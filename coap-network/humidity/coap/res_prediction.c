// coap/res_prediction.c
#include "res_prediction.h"

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
    // last_prediction = predict_humidity();
    res_prediction.trigger();
}

static void res_event_handler(void) {
    coap_notify_observers(&res_prediction);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    int len = snprintf((char *)buffer, preferred_size, "%.2f", last_prediction);
    coap_set_payload(response, buffer, len);
}
