#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "dev/button-hal.h"
#include "leds.h"
#include "random.h"
#include "../cJSON-master/cJSON.h"

#include <stdlib.h>
#include <string.h>
#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_INFO

// CONFIG
#define CLOUD_SERVER_EP "coap://[fd00::1]:5683"
#define REGISTRATION_RESOURCE_PATH "registration"
#define MAX_REGISTRATION_RETRY 5
#define REGISTRATION_ACK_CODE CONTENT_2_05
#define REGISTRATION_WAIT_SECONDS 5
#define SENSOR_SAMPLE_INTERVAL 30

extern coap_resource_t res_light;

int light_state = 0;

static int registered = 0;
static int registration_retry_count = 0;
static struct etimer debounce_timer;
static struct etimer wait_timer;

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("Registration timed out\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("Response: %i\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("Registration successful\n");
  } else {
    LOG_WARN("Registration failed\n");
  }
}

PROCESS(light_sensor_main_process, "Light Sensor Main Process");
AUTOSTART_PROCESSES(&light_sensor_main_process);


PROCESS_THREAD(light_sensor_main_process, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];

  PROCESS_BEGIN();

  coap_engine_init();

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);

  while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *string_array = cJSON_CreateArray();
    cJSON_AddItemToArray(string_array, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", string_array);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("Sending registration...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if (!registered) {
      registration_retry_count++;
      etimer_set(&wait_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    }
  }

  if (!registered) {
    LOG_ERR("Max registration attempts reached. Exiting.\n");
    PROCESS_EXIT();
  }

  coap_activate_resource(&res_light, "light");

  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {

      etimer_set(&debounce_timer, CLOCK_SECOND / 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));

      LOG_INFO("Button pressed: toggling LED and generating light_state\n");
      leds_toggle(LEDS_RED);
      light_state = random_rand() % 100;
      res_light.trigger();
    }
  }

  PROCESS_END();
}
