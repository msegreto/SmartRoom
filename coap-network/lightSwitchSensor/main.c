#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "dev/button-hal.h"
#include "leds.h"
#include "random.h"
#include "../cJSON-master/cJSON.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_INFO

extern coap_resource_t res_light;

static int light = 0; // false = 0, true = 1

static struct etimer debounce_timer;
static struct etimer monitoring_timer;

static int registered = 0;
static int registration_retry_count = 0;

PROCESS(light_sensor_main_process, "Light Sensor Main Process");
AUTOSTART_PROCESSES(&light_sensor_main_process);

// === Funzione per invio CoAP all'attuatore ===
static void send_light_state_to_actuator(int state) {
  static coap_endpoint_t actuator_ep;
  static coap_message_t request[1];

  coap_endpoint_parse(ACTUATOR_EP, strlen(ACTUATOR_EP), &actuator_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(request, "/led");

  char payload[2];
  snprintf(payload, sizeof(payload), "%d", state);
  coap_set_payload(request, (uint8_t *)payload, strlen(payload));

  LOG_INFO("Sending light state to actuator: %s\n", state ? "true" : "false");

  COAP_BLOCKING_REQUEST(&actuator_ep, request, NULL);
}

// === Callback per la registrazione ===
static void registration_response_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("Registration timeout\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("Registration successful\n");
  } else {
    LOG_WARN("Registration failed\n");
  }
}

PROCESS_THREAD(light_sensor_main_process, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_message_t registration_request[1];
  static struct etimer registration_timer;

  PROCESS_BEGIN();
  coap_engine_init();

  // Aspetta rete
  LOG_INFO("[LightSensor] Waiting for network...\n");
  etimer_set(&registration_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));

  // Registrazione
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  while (!registered && registration_retry_count < MAX_REGISTRATION_RETRY) {
    coap_init_message(registration_request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(registration_request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *ss_array = cJSON_CreateArray();
    cJSON_AddItemToArray(ss_array, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", ss_array);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    coap_set_payload(registration_request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("Sending registration...\n");

    COAP_BLOCKING_REQUEST(&server_ep, registration_request, registration_response_handler);

    if (!registered) {
      registration_retry_count++;
      etimer_set(&registration_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
    }
  }

  if (!registered) {
    LOG_ERR("Max registration attempts reached. Exiting.\n");
    PROCESS_EXIT();
  }

  // Attiva risorsa CoAP
  static struct etimer resource_timer;
  etimer_set(&resource_timer, CLOCK_SECOND / 2);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&resource_timer));
  coap_activate_resource(&res_light, "light");

  LOG_INFO("System ready. Waiting for button events...\n");

  while (1) {
    PROCESS_YIELD();

    if (ev == button_hal_press_event && light == 0) {
      LOG_INFO("Button pressed: activating light\n");
      light = 1;
      send_light_state_to_actuator(light);
      res_light.trigger();

      etimer_set(&monitoring_timer, CLOCK_SECOND * 10);
      int exit_monitor = 0;

      while (!exit_monitor) {
        PROCESS_YIELD();

        if (ev == button_hal_press_event) {
          LOG_INFO("Button pressed again: deactivating light\n");
          light = 0;
          send_light_state_to_actuator(light);
          res_light.trigger();
          exit_monitor = 1;
        } else if (etimer_expired(&monitoring_timer)) {
          int decision = random_rand() % 2;
          LOG_INFO("Timeout expired. Random decision: %d\n", decision);
          if (decision == 1) {
            LOG_INFO("Deactivating light after timeout\n");
            light = 0;
            send_light_state_to_actuator(light);
            res_light.trigger();
            exit_monitor = 1;
          } else {
            LOG_INFO("Keeping light active after timeout\n");
            etimer_reset(&monitoring_timer);
          }
        }
      }
    }
  }

  PROCESS_END();
}
