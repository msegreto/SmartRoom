// coap/res_latest.c
#include "res_latest.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

EVENT_RESOURCE(res_latest,
         "title=\"Latest value\";obs",
         res_get_handler,
         NULL, NULL, NULL,
         res_event_handler);

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    float latest = get_latest_value();
    int len = snprintf((char *)buffer, preferred_size, "%.2f", latest);
    coap_set_payload(response, buffer, len);
}

void trigger_latest_event() {
    res_latest.trigger();
}

static void res_event_handler(void) {
    coap_notify_observers(&res_latest);
}