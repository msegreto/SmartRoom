#include "res-light.h"
#include "sys/log.h"
#include "../cJSON-master/cJSON.h" 

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

  cJSON *root = cJSON_CreateObject();
  if (root == NULL) {
    LOG_ERR("[Light] Failed to create JSON object\n");
    coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
    return;
  }

  // Aggiungi il campo "value" con il valore numerico come stringa
  char value_str[8];
  snprintf(value_str, sizeof(value_str), "%d", light_state);
  cJSON_AddStringToObject(root, "value", value_str);

  char *json_str = cJSON_PrintUnformatted(root);  // genera stringa JSON
  cJSON_Delete(root);

  if (json_str == NULL) {
    LOG_ERR("[Light] Failed to print JSON string\n");
    coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
    return;
  }

  // Copia nel buffer di risposta CoAP
  size_t json_len = strlen(json_str);

  memcpy(buffer, json_str, json_len);
  coap_set_payload(response, buffer, json_len);
  coap_set_status_code(response, CONTENT_2_05);
  coap_set_header_content_format(response, APPLICATION_JSON);

  LOG_INFO("[Light] JSON payload sent: %.*s\n", (int)json_len, json_str);

}
