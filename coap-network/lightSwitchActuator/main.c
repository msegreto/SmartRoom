// === FILE: main.c ===
#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "leds.h"
#include "sys/log.h"
#include "random.h"
#include "cJSON.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "LedActuator"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(led_actuator_process, "LED Actuator Main Process");
AUTOSTART_PROCESSES(&led_actuator_process);

extern coap_resource_t res_led;

static int registered = 0;
static int registration_retry_count = 0;

// Handler per le notifiche observe
void observe_handler(coap_message_t *response) {
  const uint8_t *payload = NULL;
  int len = coap_get_payload(response, &payload);
  if(response == NULL || len <= 0) {
    LOG_ERR("[LedActuator] Observe timeout or no payload\n");
    return;
  }
  LOG_INFO("[LedActuator] Notification received: %.*s\n", len, (char *)payload);

  if(len == 2 && strncmp((char *)payload, "ON", 2) == 0) {
    leds_on(LEDS_GREEN);
  } else {
    leds_off(LEDS_GREEN);
  }
}

static void observe_light_sensor() {
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];

  char sensor_ip[64];
  if (!lookup_sensor_address(sensor_ip, sizeof(sensor_ip))) {
    LOG_ERR("[LedActuator] Could not find sensor IP\n");
    return;
  }

  char url[128];
  snprintf(url, sizeof(url), "coap://%s:5683", sensor_ip);
  coap_endpoint_parse(url, strlen(url), &server_ep);

  coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(request, "/light");
  coap_set_header_observe(request, 0);

  LOG_INFO("[LedActuator] Sending observe request to /light at %s\n", sensor_ip);
  COAP_BLOCKING_REQUEST(&server_ep, request, observe_handler);
}

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[LedActuator] Registration timed out\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("[LedActuator] Response: %i\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[LedActuator] Registration successful\n");
  } else {
    LOG_WARN("[LedActuator] Registration failed\n");
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
      LOG_ERR("[LedActuator] Failed to create JSON object\n");
      return;
    }

    cJSON_AddStringToObject(root, "s", "led_actuator");
    cJSON *string_array = cJSON_CreateArray();
    cJSON_AddItemToArray(string_array, cJSON_CreateString("led"));
    cJSON_AddItemToObject(root, "ss", string_array);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[LedActuator] Failed to create payload\n");
      return;
    }

    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[LedActuator] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if (!registered) {
      registration_retry_count++;
      clock_wait(CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
    }
  }

  if (!registered) {
    LOG_WARN("[LedActuator] Max registration attempts reached\n");
  }
}

static int lookup_sensor_address(char *ip_buffer, size_t buffer_len) {
  static coap_endpoint_t cloud_ep;
  static coap_message_t request[1];
  const uint8_t *chunk;

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &cloud_ep);
  coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(request, "/lookup");
  coap_set_header_uri_query(request, "s=light_sensor");

  LOG_INFO("[LedActuator] Looking up light_sensor...\n");

  coap_message_t *response = NULL;
  COAP_BLOCKING_REQUEST(&cloud_ep, request, &response);

  if (response == NULL) {
    LOG_ERR("[LedActuator] Lookup failed\n");
    return 0;
  }

  int len = coap_get_payload(response, &chunk);
  if (len <= 0) {
    LOG_ERR("[LedActuator] No payload in lookup response\n");
    return 0;
  }

  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  cJSON *json = cJSON_Parse(payload);
  if (!json) return 0;

  cJSON *ip = cJSON_GetObjectItem(json, "ip");
  if (cJSON_IsString(ip) && ip->valuestring) {
    strncpy(ip_buffer, ip->valuestring, buffer_len - 1);
    ip_buffer[buffer_len - 1] = '\0';
    cJSON_Delete(json);
    return 1;
  }

  cJSON_Delete(json);
  return 0;
}

PROCESS_THREAD(led_actuator_process, ev, data) {
  PROCESS_BEGIN();

  coap_engine_init();
  register_to_cloud();

  if (registered) {
    coap_activate_resource(&res_led, "led");
    observe_light_sensor();
  }

  PROCESS_END();
}
