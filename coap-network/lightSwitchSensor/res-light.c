#include "res-light.h"
#include "sys/log.h"

#define LOG_MODULE "Light_Resource"
#define LOG_LEVEL LOG_LEVEL_INFO

int light_state = 0;

// === Dichiarazioni handler ===
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

// === Risorsa osservabile ===
EVENT_RESOURCE(res_light,
         "title=\"Light\";rt=\"Light\";obs",
         res_get_handler,
         NULL,
         NULL,
         NULL,
         res_event_handler);

// === Trigger da chiamare nel processo principale ===
void res_light_trigger(void) {
  LOG_INFO("[Light] Triggering event for observers (state=%d)\n", light_state);
  res_light.trigger();  // Richiama l'event handler e notifica gli observer
}

// === Invocato automaticamente da Contiki quando triggerato ===
static void res_event_handler(void) {
  LOG_INFO("[Light] Notifying observers via res_event_handler()\n");
  coap_notify_observers(&res_light);
}

// === Risposta a richieste GET ===
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  LOG_INFO("[Light] GET handler called, current state = %d\n", light_state);

  int len = snprintf((char *)buffer, preferred_size, "%d", light_state);
  coap_set_status_code(response, CONTENT_2_05);  // 🔥 necessario per CoAP notify
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_payload(response, buffer, len);

  LOG_INFO("[Light] Payload sent: %s\n", buffer);
}
