#include "res-control.h"
#include "coap-engine.h"
#include "logic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "logic.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

static float status = 0;
static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

static void post_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  const uint8_t *payload = NULL;
  int len = coap_get_payload(request, &payload);
  if (payload && len > 0) {
    float th_min = 0, th_max = 0;
    sscanf((const char *)payload, "%f,%f", &th_min, &th_max);
    logic_set_thresholds(th_min, th_max);
  }
  const char *msg = "Thresholds updated";
  coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void get_threshold_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  float th_min, th_max;
  logic_get_thresholds(&th_min, &th_max);
  int len = snprintf((char *)buffer, preferred_size, "%.2f,%.2f", th_min, th_max);
  coap_set_payload(response, buffer, len);
}

RESOURCE(res_set_threshold,
         "title=\"Set thresholds\"",
         NULL, post_handler, NULL, NULL);

RESOURCE(res_get_threshold,
         "title=\"Get thresholds\"",
         get_threshold_handler, NULL, NULL, NULL);


EVENT_RESOURCE(res_status,
         "title=\"Actuator status\";obs",
         res_get_handler,
         NULL, NULL, NULL,
         res_event_handler);

void trigger_status_change(void) {
  LOG_INFO("[Status] Triggering Status event\n");
  res_status.trigger();
}

static void res_event_handler(void) {
    LOG_INFO("[Status] Notifying observers...\n");
    coap_notify_observers(&res_status);
}

static void res_get_handler(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    warming_state_t current = logic_get_state();
    const char *status_str = NULL;

    switch (current) {
      case WARMING_COOLING: status_str = "cooling"; break;
      case WARMING_HEATING: status_str = "heating"; break;
      default: status_str = "none"; break;
    }

    int len = snprintf((char *)buffer, preferred_size, "%s", status_str);
    if (len > 0) {
        LOG_INFO("[Status] Payload: %s\n", status_str);
        coap_set_header_content_format(response, TEXT_PLAIN);
        coap_set_payload(response, buffer, len);
    } else {
        LOG_WARN("[Status] Failed to format payload\n");
    }

    LOG_INFO("[Status] GET handled\n");
}