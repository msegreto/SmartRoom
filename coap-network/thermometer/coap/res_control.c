// coap/res_control.c
#include "res_control.h"

static void res_get_on(coap_message_t *request, coap_message_t *response,
                       uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();
    const char *msg = "Sensor ON";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_get_off(coap_message_t *request, coap_message_t *response,
                        uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_off();
    const char *msg = "Sensor OFF";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

RESOURCE(res_on, "title=\"Sensor ON\"", res_get_on, NULL, NULL, NULL);
RESOURCE(res_off, "title=\"Sensor OFF\"", res_get_off, NULL, NULL, NULL);
