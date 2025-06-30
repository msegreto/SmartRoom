#include "coap-engine.h"
#include "leds.h"
#include <string.h>

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
  len = coap_get_payload(request, &payload);

  if(len == 1 && payload[0] == '1') {
    leds_on(LEDS_GREEN);
  } else {
    leds_off(LEDS_GREEN);
  }
  coap_set_status_code(response, CHANGED_2_04);
}

// Metodo GET di debug: restituisce stato LED (ON/OFF)
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *msg = (leds_get() & LEDS_GREEN) ? "ON" : "OFF";
  memcpy(buffer, msg, strlen(msg));
  coap_set_payload(response, buffer, strlen(msg));
}