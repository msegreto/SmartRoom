#include "res_prediction.h"
#include "../../cJSON-master/cJSON.h"

#include "sys/log.h"
#define LOG_MODULE "res_prediction"
#define LOG_LEVEL LOG_LEVEL_APP

static float last_prediction = MEAN_TEMPERATURE; 
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_prediction,
         "title=\"Prediction\";obs",
         res_get_handler,
         NULL, NULL, NULL,
         res_event_handler);

void trigger_prediction_event() {
    last_prediction = predict_temperature();
    LOG_INFO("[Prediction] Triggering prediction event, value: %.2f\n", last_prediction);
    res_prediction.trigger();
}

static void res_event_handler(void) {
    LOG_INFO("[Prediction] Notifying observers...\n");
    coap_notify_observers(&res_prediction);
}


static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

  char formatted[8]; 
  int int_part = (int)last_prediction;
  int decimal_part = (int)((last_prediction - int_part) * 100);
  if (decimal_part < 0) {
    decimal_part = -decimal_part;
  }

  snprintf(formatted, sizeof(formatted), "%d,%02d", int_part, decimal_part);

  // Creazione oggetto JSON
  cJSON *root = cJSON_CreateObject();
  if (!root) {
    LOG_ERR("[Prediction] Failed to create JSON object\n");
    coap_set_payload(response, NULL, 0);
    return;
  }

  cJSON_AddStringToObject(root, "value", formatted);

  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (json_str != NULL) {
    size_t len = strlen(json_str);
    memcpy(buffer, json_str, len);
    coap_set_payload(response, buffer, len);
    coap_set_status_code(response, CONTENT_2_05);
    coap_set_header_content_format(response, APPLICATION_JSON);
    LOG_INFO("[Prediction] JSON response: %s\n", json_str);
  } else {
    LOG_ERR("[Prediction] Failed to print JSON\n");
    coap_set_payload(response, NULL, 0);
  }

  LOG_INFO("[Prediction] GET request handled\n");
}
