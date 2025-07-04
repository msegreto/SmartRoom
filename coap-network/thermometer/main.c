#include "contiki.h"
#include "config.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "coap-block1.h"
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

//void trigger_prediction_event();
//void trigger_latest_event();

static int registered = 0;

static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("[Thermometer] === RESPONSE HANDLER CALLED ===\n");
  
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Thermometer] Registration timed out - no response received\n");
    return;
  }
  
  LOG_INFO("[Thermometer] Response received! Code: %d\n", response->code);
  
  int len = coap_get_payload(response, &chunk);
  if (len <= 0 || chunk == NULL) {
    LOG_WARN("[Thermometer] Empty or invalid payload received (len=%d)\n", len);
  } else {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[Thermometer] Response payload: '%s'\n", payload);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Thermometer] Registration successful!\n");
  } else {
    LOG_WARN("[Thermometer] Registration failed with code: %d\n", response->code);
  }
  
  LOG_INFO("[Thermometer] === RESPONSE HANDLER END ===\n");
}

PROCESS_THREAD(thermometer_process, ev, data) {
  static struct etimer timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();

  // Wait for network to be established before attempting registration
  LOG_INFO("[Thermometer] Waiting for network establishment...\n");
  etimer_set(&timer, CLOCK_SECOND * 10); // Wait 10 seconds for network
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  LOG_INFO("[Thermometer] Network wait complete, starting registration\n");

  // Registrazione diretta nel protothread
  LOG_INFO("[Thermometer] Attempting to parse endpoint: %s\n", CLOUD_SERVER_EP);
  if(coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep) == 0) {
    LOG_ERR("[Thermometer] Failed to parse server endpoint!\n");
    PROCESS_EXIT();
  }
  LOG_INFO("[Thermometer] Server endpoint parsed successfully\n");
  LOG_INFO("[Thermometer] Target server: %s:%d\n", 
           server_ep.ipaddr.u8[0] == 0xfd ? "fd00::1" : "unknown", 
           server_ep.port);
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

    cJSON_AddStringToObject(root, "s", "thermo");
    cJSON *resources = cJSON_CreateArray();
    if(resources == NULL) {
      LOG_ERR("[Thermometer] Failed to create JSON array\n");
      cJSON_Delete(root);
      PROCESS_EXIT();
    }
    cJSON_AddItemToArray(resources, cJSON_CreateString("temp"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("predt"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("ont"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("offt"));
    cJSON_AddItemToObject(root, "ss", resources);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(payload == NULL) {
      LOG_ERR("[Thermometer] Failed to create payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[Thermometer] JSON payload (len=%zu): %s\n", strlen(payload), payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[Thermometer] Sending registration request to fd00::1:5683...\n");
    LOG_INFO("[Thermometer] Request details - Type: CON, Method: POST, URI: /%s\n", REGISTRATION_RESOURCE_PATH);

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    LOG_INFO("[Thermometer] COAP_BLOCKING_REQUEST completed, checking response...\n");
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
  coap_activate_resource(&res_latest, "temp");
  coap_activate_resource(&res_prediction, "predt");
  coap_activate_resource(&res_on, "ont");
  coap_activate_resource(&res_off, "offt");

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
