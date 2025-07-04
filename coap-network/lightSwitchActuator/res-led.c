#include "coap-engine.h"
#include "leds.h"
#include "sys/log.h"
#include <string.h>

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
  
  LOG_INFO("=== LED CONTROL REQUEST RECEIVED ===\n");
  
  len = coap_get_payload(request, &payload);
  
  if (len > 0) {
    char payload_str[len + 1];
    memcpy(payload_str, payload, len);
    payload_str[len] = '\0';
    LOG_INFO("Payload received: '%s' (length: %d)\n", payload_str, (int)len);
    
    if(len == 1 && payload[0] == '1') {
      leds_on(LEDS_GREEN);
      LOG_INFO("LED turned ON (GREEN)\n");
    } else {
      leds_off(LEDS_GREEN);
      LOG_INFO("LED turned OFF (GREEN)\n");
    }
  } else {
    LOG_WARN("No payload received, turning LED OFF\n");
    leds_off(LEDS_GREEN);
  }
  
  coap_set_status_code(response, CHANGED_2_04);
  LOG_INFO("Response sent with status CHANGED_2_04\n");
}

// Metodo GET di debug: restituisce stato LED (ON/OFF)
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  LOG_INFO("=== LED STATUS REQUEST RECEIVED ===\n");
  
  const char *msg = (leds_get() & LEDS_GREEN) ? "ON" : "OFF";
  memcpy(buffer, msg, strlen(msg));
  coap_set_payload(response, buffer, strlen(msg));
  
  LOG_INFO("LED status: %s\n", msg);
  LOG_INFO("Response sent\n");
}