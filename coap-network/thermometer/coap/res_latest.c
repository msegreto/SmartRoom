// coap/res_latest.c
#include "res_latest.h"
#include "coap-constants.h"
#include <stdio.h>
#include "../sensor/buffer.h"

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
    if (buffer == NULL || preferred_size < 10) {
        coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
        return;
    }
    
    float latest = get_latest_value();
    int len = snprintf((char *)buffer, preferred_size, "%.2f", latest);
    
    if (len < 0 || len >= preferred_size) {
        coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
        return;
    }
    
    coap_set_payload(response, buffer, len);
}

void trigger_latest_event() {
    res_latest.trigger();
}

static void res_event_handler(void) {
    coap_notify_observers(&res_latest);
}