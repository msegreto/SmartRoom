#include "contiki.h"
#include "config.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "../cJSON-master/cJSON.h"
#include "coap/res_latest.h"
#include "coap/res_prediction.h"
#include "sensor/sensing.h"
#include "sensor/buffer.h"
#include "sensor/prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "SmartThermometer"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(thermometer_process, "Smart Thermometer");
AUTOSTART_PROCESSES(&thermometer_process);

extern coap_resource_t res_latest;
extern coap_resource_t res_prediction;
extern coap_resource_t res_on;
extern coap_resource_t res_off;

void trigger_prediction_event();
void trigger_latest_event();

static int registered = 0;

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Thermometer] Registration timed out\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("[Thermometer] Response: %i\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Thermometer] Registration successful\n");
  } else {
    LOG_WARN("[Thermometer] Registration failed\n");
  }
}

PROCESS_THREAD(thermometer_process, ev, data) {
  static struct etimer timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();

  // Registrazione diretta nel protothread
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  registered = 0;
  retry = 0;

  while(retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if(root == NULL) {
      LOG_ERR("[Thermometer] Failed to create JSON object\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "smart_thermometer");
    cJSON *resources = cJSON_CreateArray();
    cJSON_AddItemToArray(resources, cJSON_CreateString("latestTemp"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("predictionTemp"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("sensor/onTemp"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("sensor/offTemp"));
    cJSON_AddItemToObject(root, "ss", resources);
    cJSON_AddNumberToObject(root, "t", SENSING_PERIOD_SECONDS);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(payload == NULL) {
      LOG_ERR("[Thermometer] Failed to create payload\n");
      PROCESS_EXIT();
    }

    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[Thermometer] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if(!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if(!registered) {
    LOG_WARN("[Thermometer] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  // Attiva risorse e avvia sensing
  coap_activate_resource(&res_latest, "latest");
  coap_activate_resource(&res_prediction, "prediction");
  coap_activate_resource(&res_on, "sensor/on");
  coap_activate_resource(&res_off, "sensor/off");

  sensor_on();
  etimer_set(&timer, CLOCK_SECOND * SENSING_PERIOD_SECONDS);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    if(sensor_is_active()) {
      float temp = generate_random_temperature();
      LOG_INFO("Generated temperature: %.2f\n", temp);
      update_buffer(temp);
      trigger_latest_event();

      if(buffer_is_full()) {
        LOG_INFO("Buffer is full, triggering prediction event.\n");
        trigger_prediction_event();
      }
    }

    etimer_reset(&timer);
  }

  PROCESS_END();
}
