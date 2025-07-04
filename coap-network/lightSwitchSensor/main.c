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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_INFO

extern coap_resource_t res_light;

static int light = 0;
static int registered = 0;

PROCESS(light_sensor_main_process, "Light Sensor Main Process");
AUTOSTART_PROCESSES(&light_sensor_main_process);

// === Callback per risposta registrazione ===
static void registration_response_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Light] Registration timed out - no response\n");
    return;
  }

  LOG_INFO("[Light] Registration response code: %d\n", response->code);

  int len = coap_get_payload(response, &chunk);
  if (len > 0 && chunk != NULL) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[Light] Payload: %s\n", payload);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Light] Registration successful!\n");
  } else {
    LOG_WARN("[Light] Registration failed with code: %d\n", response->code);
  }
}

PROCESS_THREAD(light_sensor_main_process, ev, data) {
  static struct etimer timer;
  static coap_endpoint_t server_ep, actuator_ep;
  static coap_message_t request[1], led_request[1];
  static char led_payload[2];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();

  LOG_INFO("[Light] Waiting for network...\n");
  etimer_set(&timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  coap_endpoint_parse(ACTUATOR_EP, strlen(ACTUATOR_EP), &actuator_ep);

  registered = 0;
  retry = 0;

  while (retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
      LOG_ERR("[Light] Failed to create JSON object\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *resources = cJSON_CreateArray();
    if (resources == NULL) {
      LOG_ERR("[Light] Failed to create JSON array\n");
      cJSON_Delete(root);
      PROCESS_EXIT();
    }
    cJSON_AddItemToArray(resources, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", resources);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[Light] Failed to generate payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[Light] Registration payload: %s\n", payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    COAP_BLOCKING_REQUEST(&server_ep, request, registration_response_handler);
    free(payload);

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

  // Attiva risorsa
  coap_activate_resource(&res_light, "light");

  LOG_INFO("[Light] System ready\n");

  // Inizio ciclo principale
  while (1) {
    PROCESS_YIELD();

    if (ev == button_hal_press_event && light == 0) {
      LOG_INFO("[Light] Button pressed: activating light\n");
      light = 1;

      coap_init_message(led_request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(led_request, "/led");
      snprintf(led_payload, sizeof(led_payload), "%d", light);
      coap_set_payload(led_request, (uint8_t *)led_payload, strlen(led_payload));
      COAP_BLOCKING_REQUEST(&actuator_ep, led_request, NULL);

      res_light.trigger();

      etimer_set(&timer, CLOCK_SECOND * 10);
      int exit_monitor = 0;

      while (!exit_monitor) {
        PROCESS_YIELD();

        if (ev == button_hal_press_event) {
          LOG_INFO("[Light] Button pressed again: deactivating light\n");
          light = 0;

          coap_init_message(led_request, COAP_TYPE_CON, COAP_POST, 0);
          coap_set_header_uri_path(led_request, "/led");
          snprintf(led_payload, sizeof(led_payload), "%d", light);
          coap_set_payload(led_request, (uint8_t *)led_payload, strlen(led_payload));
          COAP_BLOCKING_REQUEST(&actuator_ep, led_request, NULL);

          res_light.trigger();
          exit_monitor = 1;

        } else if (etimer_expired(&timer)) {
          int decision = random_rand() % 2;
          LOG_INFO("[Light] Timeout expired, random decision: %d\n", decision);

          if (decision == 1) {
            LOG_INFO("[Light] Deactivating after timeout\n");
            light = 0;

            coap_init_message(led_request, COAP_TYPE_CON, COAP_POST, 0);
            coap_set_header_uri_path(led_request, "/led");
            snprintf(led_payload, sizeof(led_payload), "%d", light);
            coap_set_payload(led_request, (uint8_t *)led_payload, strlen(led_payload));
            COAP_BLOCKING_REQUEST(&actuator_ep, led_request, NULL);

            res_light.trigger();
            exit_monitor = 1;

          } else {
            LOG_INFO("[Light] Keeping light active\n");
            etimer_reset(&timer);
          }
        }
      }
    }
  }

  PROCESS_END();
}
