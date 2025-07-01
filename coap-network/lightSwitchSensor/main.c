#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "button-sensor.h"
#include "sys/log.h"
#include "random.h"
#include "leds.h"
#include "../cJSON-master/cJSON.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(light_sensor_main_process, "Light Sensor Main Process");
AUTOSTART_PROCESSES(&light_sensor_main_process);

// dichiarazioni esterne
extern coap_resource_t res_light;
extern struct process button_process;

// stato registrazione
static int registered = 0;
static int registration_retry_count = 0;

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;

  if (response == NULL) {
    LOG_ERR("Registration timeout\n");
    return;
  }

  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("Response code: %u\n", response->code);

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("Registration successful\n");
  } else {
    LOG_WARN("Registration failed\n");
  }
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
