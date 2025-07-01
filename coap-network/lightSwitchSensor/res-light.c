#include <stdio.h>
#include "coap-engine.h"
#include <string.h>

extern int light_state;

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  char msg[4];
  snprintf(msg, sizeof(msg), "%d", light_state);
  memcpy(buffer, msg, strlen(msg));
  coap_set_payload(response, buffer, strlen(msg));
}

RESOURCE(res_light,
         "title=\"Light status\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);
