// coap/res_prediction.c
#include "res_prediction.h"
#include "sys/log.h"
#include "coap-observe.h"
#include "net/ipv6/uip.h"
#include <stdio.h>

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
    res_prediction.trigger();
}

static void res_event_handler(void) {
    LOG_INFO("[Prediction] Notifying observers...\n");

    coap_observee_t *o = coap_observees;
    if (o == NULL) {
        LOG_WARN("[Prediction] No observers found.\n");
    }

    while (o != NULL) {
        LOG_INFO("Observer: ");
        LOG_INFO_6ADDR(&o->addr);
        LOG_INFO_(" port %u\n", uip_ntohs(o->port));
        o = o->next;
    }

    coap_notify_observers(&res_prediction);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    last_prediction = 1;
    int len = snprintf((char *)buffer, preferred_size, "%.2f", last_prediction);
    LOG_INFO("[Prediction] GET request received, sending: %.2f\n", last_prediction);
    coap_set_payload(response, buffer, len);
}
