// coap/res_prediction.c
#include "res_prediction.h"

#include "sys/log.h"
#define LOG_MODULE "res_prediction"
#define LOG_LEVEL LOG_LEVEL_APP

static float last_prediction = 0;
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_prediction,
         "title=\"Prediction\";obs",
         res_get_handler,
         NULL, NULL, NULL,
         res_event_handler);

void trigger_prediction_event() {
    last_prediction = 1;
    LOG_INFO("[Prediction] Triggering prediction event, value: %.2f\n", last_prediction);
    // last_prediction = predict_humidity();
    res_prediction.trigger();
}

static void res_event_handler(void) {
    LOG_INFO("[Prediction] Notifying observers...\n");
    int notified = coap_notify_observers(&res_prediction);
    LOG_INFO("[Prediction] Notification result: %d\n", notified);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    last_prediction = 1;
    int len = snprintf((char *)buffer, preferred_size, "%.2f", last_prediction);
    LOG_INFO("[Prediction] GET request received, sending: %.2f\n", last_prediction);
    coap_set_payload(response, buffer, len);
}

