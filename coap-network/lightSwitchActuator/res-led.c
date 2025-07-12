#include "coap-engine.h"
#include "leds.h"
#include "sys/log.h"
#include <string.h>
#include "../cJSON-master/cJSON.h"

#define LOG_MODULE "LED_Resource"
#define LOG_LEVEL LOG_LEVEL_INFO

static void res_post_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_led,
         "title=\"LED actuator\"",
         res_get_handler,
         res_post_handler,
         NULL,
         NULL);

static void res_post_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t len = 0;
  const uint8_t *payload = NULL;
  const char *status_str = "OFF"; // default

  LOG_INFO("=== LED CONTROL REQUEST RECEIVED ===\n");

  len = coap_get_payload(request, &payload);

  if (len > 0) {
    char payload_str[len + 1];
    memcpy(payload_str, payload, len);
    payload_str[len] = '\0';
    LOG_INFO("Payload received: '%s' (length: %d)\n", payload_str, (int)len);

    if(len == 1 && payload[0] == '1') {
      leds_single_on(LEDS_YELLOW);
      LOG_INFO("LED turned ON\n");
      status_str = "ON";
    } else {
      leds_single_off(LEDS_YELLOW);
      LOG_INFO("LED turned OFF\n");
    }
  } else {
    LOG_WARN("No payload received, turning LED OFF\n");
    leds_single_off(LEDS_YELLOW);
  }

  // === JSON response ===
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "value", status_str);
  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (json_str != NULL) {
    size_t json_len = strlen(json_str);
    memcpy(buffer, json_str, json_len);
    coap_set_payload(response, buffer, json_len);
    coap_set_header_content_format(response, APPLICATION_JSON);
  }

  coap_set_status_code(response, CHANGED_2_04);
  LOG_INFO("Response sent with JSON status: %s\n", status_str);
}


// Metodo GET di debug: restituisce stato LED (ON/OFF)
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  LOG_INFO("=== LED STATUS REQUEST RECEIVED ===\n");

  const char *status_str = (leds_get() & LEDS_YELLOW) ? "ON" : "OFF";

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "value", status_str);
  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (json_str != NULL) {
    size_t json_len = strlen(json_str);
    memcpy(buffer, json_str, json_len);
    coap_set_payload(response, buffer, json_len);
    coap_set_status_code(response, CONTENT_2_05);
    coap_set_header_content_format(response, APPLICATION_JSON);
  }

  LOG_INFO("LED status: %s\n", status_str);
}