
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
    // last_prediction = predict_temperature();
    res_prediction.trigger();
}

static void res_event_handler(void) {
    LOG_INFO("[Prediction] Notifying observers...\n");
    coap_notify_observers(&res_prediction);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    LOG_INFO("[Prediction] GET request received\n");
    
    // Check if it's an OBSERVE request
    if(coap_is_option(request, COAP_OPTION_OBSERVE)) {
        LOG_INFO("[Prediction] 🔍 OBSERVE request detected!\n");
        LOG_INFO("[Prediction] Adding observer to list\n");
    } else {
        LOG_INFO("[Prediction] Regular GET request (no observe)\n");
    }
    
    int len = snprintf((char *)buffer, preferred_size, "%.2f", last_prediction);

    if (len > 0) {
        LOG_INFO("[Prediction] Formatted payload: %s (len=%d)\n", buffer, len);
        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_payload(response, buffer, len);
    } else {
        LOG_WARN("[Prediction] Failed to format payload\n");
    }

    LOG_INFO("[Prediction] GET request handled, content sent\n");
}

