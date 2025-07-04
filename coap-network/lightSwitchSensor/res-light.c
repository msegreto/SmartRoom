#include "res-light.h"
#include "sys/log.h"

#define LOG_MODULE "Light_Resource"
#define LOG_LEVEL LOG_LEVEL_INFO

int light_state = 0;

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  LOG_INFO("=== LIGHT SENSOR VALUE REQUESTED ===\n");
  LOG_INFO("[Light] GET handler called, current state = %d\n", light_state);
  
  char msg[4];
  snprintf(msg, sizeof(msg), "%d", light_state);
  memcpy(buffer, msg, strlen(msg));
  coap_set_payload(response, buffer, strlen(msg));
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  LOG_INFO("Light value sent: %s\n", msg);
}

RESOURCE(res_light,
         "title=\"Light\";rt=\"Light\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL);

// === NUOVA FUNZIONE PER NOTIFICARE OSSERVATORI ===
void res_light_trigger(void) {
  LOG_INFO("[Light] Notifying observers of light state change\n");
  coap_notify_observers(&res_light);
}
