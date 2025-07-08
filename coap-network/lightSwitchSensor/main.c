#include "contiki.h"
#include "config.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "../cJSON-master/cJSON.h"
#include "dev/button-hal.h"
#include "res-light.h"
#include "res-control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(light_sensor_main_process, "Light Sensor Main Process");
AUTOSTART_PROCESSES(&light_sensor_main_process);

extern coap_resource_t res_light;

static int registered = 0;

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Light] Registration timeout\n");
    return;
  }

  int len = coap_get_payload(response, &chunk);
  if (len > 0 && chunk != NULL) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[Light] Response: %s\n", payload);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Light] Registration successful\n");
  } else {
    LOG_WARN("[Light] Registration failed: %d\n", response->code);
  }
}

PROCESS_THREAD(light_sensor_main_process, ev, data) {
  static struct etimer timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();

  LOG_INFO("[Light] Waiting for network establishment...\n");
  etimer_set(&timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  LOG_INFO("[Light] Starting registration\n");

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);

  registered = 0;
  retry = 0;

  while (retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
      LOG_ERR("[Light] JSON creation failed\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *resources = cJSON_CreateArray();
    if (resources == NULL) {
      LOG_ERR("[Light] JSON array creation failed\n");
      cJSON_Delete(root);
      PROCESS_EXIT();
    }
    cJSON_AddItemToArray(resources, cJSON_CreateString("light"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("onlightsensor"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("offlightsensor"));
    cJSON_AddItemToObject(root, "ss", resources);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[Light] Payload generation failed\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[Light] Attempt %d: %s\n", retry + 1, payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

    if (!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if (!registered) {
    LOG_ERR("[Light] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  coap_activate_resource(&res_light, "light");
  coap_activate_resource(&res_on, "onlightsensor");
  coap_activate_resource(&res_off, "offlightsensor");

  LOG_INFO("[Light] System ready\n");

  while (1) {
    PROCESS_YIELD();

    if (ev == button_hal_press_event && light_state == 0) {
      LOG_INFO("[Light] Button pressed: activating light\n");
      light_state = 1;
      res_light_trigger();
      etimer_set(&timer, CLOCK_SECOND * 10);
    } 
    else if (ev == button_hal_press_event && light_state == 1) {
      LOG_INFO("[Light] Button pressed: deactivating light\n");
      light_state = 0;
      res_light_trigger();
    }
    else if (etimer_expired(&timer) && light_state == 1) {
      int decision = random_rand() % 2;
      LOG_INFO("[Light] Timeout expired, random decision: %d\n", decision);

      if (decision == 1) {
        LOG_INFO("[Light] Deactivating after timeout\n");
        light_state = 0;
        res_light_trigger();
      } else {
        LOG_INFO("[Light] Keeping light active\n");
        etimer_reset(&timer);
      }
    }
  }

  PROCESS_END();
}
