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

#define LOG_MODULE "SmartHumidity"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(humidity_process, "Smart Humidity");
AUTOSTART_PROCESSES(&humidity_process);

extern coap_resource_t res_latest;
extern coap_resource_t res_prediction;
extern coap_resource_t res_on;
extern coap_resource_t res_off;

//void trigger_prediction_event();
//void trigger_latest_event();

static int registered = 0;

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Humidity] Registration timed out\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  if (len <= 0 || chunk == NULL) {
    LOG_WARN("[Humidity] Empty or invalid payload received\n");
    return;
  }
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("[Humidity] Response: %i\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Humidity] Registration successful\n");
  } else {
    LOG_WARN("[Humidity] Registration failed\n");
  }
}

PROCESS_THREAD(humidity_process, ev, data) {
  static struct etimer timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();

  // Wait for network to be established before attempting registration
  LOG_INFO("[Humidity] Waiting for network establishment...\n");
  etimer_set(&timer, CLOCK_SECOND * 10); // Wait 10 seconds for network
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  LOG_INFO("[Humidity] Network wait complete, starting registration\n");

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  registered = 0;
  retry = 0;

  while(retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if(root == NULL) {
      LOG_ERR("[Humidity] Failed to create JSON object\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "humid");
    cJSON *resources = cJSON_CreateArray();
    if(resources == NULL) {
      LOG_ERR("[Humidity] Failed to create JSON array\n");
      cJSON_Delete(root);
      PROCESS_EXIT();
    }
    cJSON_AddItemToArray(resources, cJSON_CreateString("hum"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("pred"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("on"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("off"));
    cJSON_AddItemToObject(root, "ss", resources);
    cJSON_AddNumberToObject(root, "t", SENSING_PERIOD_SECONDS);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(payload == NULL) {
      LOG_ERR("[Humidity] Failed to create payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[Humidity] JSON payload (len=%zu): %s\n", strlen(payload), payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[Humidity] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if(!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if(!registered) {
    LOG_WARN("[Humidity] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  coap_activate_resource(&res_latest, "latestHum");
  coap_activate_resource(&res_prediction, "predictionHum");
  coap_activate_resource(&res_on, "sensorHum/on");
  coap_activate_resource(&res_off, "sensorHum/off");

  sensor_on();
  etimer_set(&timer, CLOCK_SECOND * SENSING_PERIOD_SECONDS);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    if(sensor_is_active()) {
      float hum = generate_random_humidity(); // <-- implementa se non esiste
      LOG_INFO("Generated humidity: %.2f\n", hum);
      update_buffer(hum);
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


