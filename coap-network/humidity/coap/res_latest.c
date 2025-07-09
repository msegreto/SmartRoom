
#include "res_prediction.h"

#include "sys/log.h"
#define LOG_MODULE "res_latest"
#define LOG_LEVEL LOG_LEVEL_APP

static float last_value = 0;
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_latest,
         "title=\"Latest value\";obs",
         res_get_handler,
         NULL, NULL, NULL,
         res_event_handler);

void trigger_latest_event(float value) {
    last_value = value;
    LOG_INFO("[Latest] Triggering latest event, value: %.2f\n", last_value);
    res_latest.trigger();
}

static void res_event_handler(void) {
    LOG_INFO("[Latest] Notifying observers...\n");
    coap_notify_observers(&res_latest);
}


static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {

  // Conversion to string with two decimal places
  int int_part = (int)last_value;
  int decimal_part = (int)((last_value - int_part) * 100);
  if (decimal_part < 0) {
    decimal_part = -decimal_part;
  }

  int len = snprintf((char *)buffer, preferred_size, "%d,%02d", int_part, decimal_part);

  if (len > 0 && len < preferred_size) {
    LOG_INFO("[Latest] Payload: %s (len=%d)\n", buffer, len);
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, buffer, len);
  } else {
    LOG_WARN("[Latest] Formatting failed or buffer overflow (len=%d)\n", len);
    coap_set_payload(response, NULL, 0);
  }

  LOG_INFO("[Latest] GET request handled\n");
}


