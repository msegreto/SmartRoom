#include "res-control.h"
#include "coap-engine.h"
#include "logic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

static void get_status_handler(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  const char *status = logic_get_status();
  coap_set_payload(response, (uint8_t *)status, strlen(status));
}

RESOURCE(res_set_threshold,
         "title=\"Set thresholds\"",
         NULL, post_handler, NULL, NULL);

RESOURCE(res_get_threshold,
         "title=\"Get thresholds\"",
         get_threshold_handler, NULL, NULL, NULL);

RESOURCE(res_status,
         "title=\"Actuator status\"",
         get_status_handler, NULL, NULL, NULL);