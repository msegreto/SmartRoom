#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "button-hal.h"
#include "light-sensor.h"
#include "sys/log.h"
#include "leds.h"
#include "random.h"
#include "cJSON.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(light_sensor_main_process, "Light Sensor Main");
AUTOSTART_PROCESSES(&light_sensor_main_process);

extern coap_resource_t res_light;
extern struct process button_process;

static int registered = 0;
static int registration_retry_count = 0;

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[LightSensor] Registration timed out\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("[LightSensor] Response: %i\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[LightSensor] Registration successful\n");
  } else {
    LOG_WARN("[LightSensor] Registration failed\n");
  }
}

static void register_to_cloud() {
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);

  while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
      LOG_ERR("[LightSensor] Failed to create JSON object\n");
      return;
    }

    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *string_array = cJSON_CreateArray();
    cJSON_AddItemToArray(string_array, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", string_array);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[LightSensor] Failed to create payload\n");
      return;
    }

    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[LightSensor] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if (!registered) {
      registration_retry_count++;
      clock_wait(CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
    }
  }

  if (!registered) {
    LOG_WARN("[LightSensor] Max registration attempts reached\n");
  }
}

PROCESS_THREAD(light_sensor_main_process, ev, data) {
  PROCESS_BEGIN();

  coap_engine_init();
  register_to_cloud();

  if (registered) {
    coap_activate_resource(&res_light, "light");
    process_start(&button_process, NULL);
  }

  PROCESS_END();
}