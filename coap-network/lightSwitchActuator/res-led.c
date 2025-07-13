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
  const char *status_str = "OFF"; 

  LOG_INFO("=== LED CONTROL REQUEST RECEIVED ===\n");

  len = coap_get_payload(request, &payload);

  if(len > 0) {
    char payload_str[len + 1];
    memcpy(payload_str, payload, len);
    payload_str[len] = '\0';
    LOG_INFO("Payload received: '%s'\n", payload_str);

    // Parsing JSON
    cJSON *json = cJSON_Parse(payload_str);
    if(json != NULL) {
      cJSON *value = cJSON_GetObjectItemCaseSensitive(json, "value");
      if(cJSON_IsString(value) && value->valuestring != NULL) {
        // Accetta sia "1" che "ON"
        if(strcmp(value->valuestring, "1") == 0 || strcasecmp(value->valuestring, "ON") == 0) {
          leds_on(LEDS_RED);
          status_str = "ON";
          LOG_INFO("LED turned ON\n");
        } else {
          leds_off(LEDS_RED);
          status_str = "OFF";
          LOG_INFO("LED turned OFF (via JSON value)\n");
        }
      } else {
        LOG_WARN("JSON 'value' field missing or invalid, turning LED OFF\n");
        leds_off(LEDS_RED);
      }
      cJSON_Delete(json);
    } else {
      LOG_WARN("Invalid JSON format, turning LED OFF\n");
      leds_off(LEDS_RED);
    }
  } else {
    LOG_WARN("No payload received, turning LED OFF\n");
    leds_off(LEDS_RED);
  }

  // === JSON response ===
  cJSON *response_json = cJSON_CreateObject();
  cJSON_AddStringToObject(response_json, "value", status_str);
  char *json_str = cJSON_PrintUnformatted(response_json);
  cJSON_Delete(response_json);

  if(json_str != NULL) {
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
  
  LOG_INFO("Current LED state: %d\n", leds_get());
  const char *status_str = (leds_get() & LEDS_RED) ? "ON" : "OFF";

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