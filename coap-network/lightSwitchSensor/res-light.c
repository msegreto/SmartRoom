#include "res-light.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_light,
               "title=\"Light resource\";obs",
               res_get_handler,
               NULL, NULL, NULL,
               res_event_handler);

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const char *msg = light_state ? "ON" : "OFF";
  memcpy(buffer, msg, strlen(msg));
  coap_set_payload(response, buffer, strlen(msg));
}

static void res_event_handler(void)
{
  printf("[LightSensor] Notifying observers...\n");
  coap_notify_observers(&res_light);
}