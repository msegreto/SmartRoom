#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "button-hal.h"
#include "light-sensor.h"
#include "sys/log.h"
#include "leds.h"
#include "random.h"
#include "../cJSON-master/cJSON.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(light_sensor_main_process, "Light Sensor Main");
AUTOSTART_PROCESSES(&light_sensor_main_process);

extern coap_resource_t res_light;

static int registered = 0;
static int registration_retry_count = 0;
static struct etimer debounce_timer, motion_timer;
static int monitoring_active = 0;

PROCESS_THREAD(light_sensor_main_process, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static cJSON *root = NULL;
  static char *payload = NULL;

  PROCESS_BEGIN();

  coap_engine_init();
  button_hal_init();

  // --- REGISTRAZIONE INLINE ---
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);

  while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    root = cJSON_CreateObject();
    if (root == NULL) {
      LOG_ERR("[LightSensor] Failed to create JSON object\n");
      break;
    }

    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *string_array = cJSON_CreateArray();
    cJSON_AddItemToArray(string_array, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", string_array);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);

    payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[LightSensor] Failed to create payload\n");
      break;
    }

    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[LightSensor] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, 
      (void (*)(coap_message_t *response)) 
      [](coap_message_t *response) {
        const uint8_t *chunk;
        if (response == NULL) {
          LOG_ERR("[LightSensor] Registration timed out\n");
          return;
        }
        int len = coap_get_payload(response, &chunk);
        char resp_payload[len + 1];
        memcpy(resp_payload, chunk, len);
        resp_payload[len] = '\0';

        LOG_INFO("[LightSensor] Response: %i\n", response->code);
        if (response->code == REGISTRATION_ACK_CODE) {
          registered = 1;
          LOG_INFO("[LightSensor] Registration successful\n");
        } else {
          LOG_WARN("[LightSensor] Registration failed\n");
        }
      }
    );

    free(payload);

    if (!registered) {
      registration_retry_count++;
      clock_wait(CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
    }
  }

  if (!registered) {
    LOG_WARN("[LightSensor] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  // --- ATTIVA RISORSA ---
  coap_activate_resource(&res_light, "light");

  // --- LOOP PRINCIPALE ---
  while (1) {
    PROCESS_YIELD();

    // BUTTON EVENT
    if (ev == sensors_event && data == &button_hal_sensor) {
      etimer_set(&debounce_timer, CLOCK_SECOND / 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));

      light_state = !light_state;
      res_light.trigger();

      if (light_state) {
        monitoring_active = 1;
        etimer_set(&motion_timer, CLOCK_SECOND * 10);
      } else {
        monitoring_active = 0;
      }
    }

    // MOTION MONITORING
    if (monitoring_active && ev == PROCESS_EVENT_TIMER && data == &motion_timer) {
      if (!simulate_motion_detection()) {
        light_state = false;
        res_light.trigger();
        monitoring_active = 0;
      } else {
        etimer_reset(&motion_timer);
      }
    }
  }

  PROCESS_END();
}


PROCESS_THREAD(light_sensor_main_process, ev, data) {
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static coap_blocking_request_state_t blocking_state;
  static struct etimer retry_timer;

  PROCESS_BEGIN();

  coap_engine_init();

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);

  while (registration_retry_count < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
      LOG_ERR("Failed to create JSON\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", arr);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);

    char *json_payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_payload == NULL) {
      LOG_ERR("Failed to create JSON payload\n");
      PROCESS_EXIT();
    }

    coap_set_payload(request, (uint8_t *)json_payload, strlen(json_payload));
    LOG_INFO("Sending registration attempt #%d...\n", registration_retry_count + 1);

    PROCESS_WAIT_EVENT_UNTIL(COAP_BLOCKING_REQUEST(&blocking_state, &server_ep, request, client_chunk_handler));

    free(json_payload);

    if (!registered) {
      registration_retry_count++;
      etimer_set(&retry_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }
  }

  if (!registered) {
    LOG_ERR("Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  coap_activate_resource(&res_light, "light");
  process_start(&button_process, NULL);
  LOG_INFO("Light sensor is up and running\n");

  PROCESS_END();
}
