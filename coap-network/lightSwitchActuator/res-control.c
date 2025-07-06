#include "res-control.h"
#include "coap-engine.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process light_actuator_process;

static int is_on = 1;

// === Handler CoAP per accensione/spegnimento ===

static void res_get_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();
    const char *msg = "Light actuator ON";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_get_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_off();
    const char *msg = "Light actuator OFF";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

RESOURCE(res_on, "title=\"Light actuator ON\"", res_get_on, NULL, NULL, NULL);
RESOURCE(res_off, "title=\"Light actuator OFF\"", res_get_off, NULL, NULL, NULL);

// === Funzioni di gestione processo e CoAP ===

void sensor_off(void) {
    if (!is_on) {
        LOG_INFO("[ActuatorCtrl] Already OFF\n");
        return;
    }

    LOG_INFO("[ActuatorCtrl] Shutting down Light Actuator\n");

    if (obs_light) {
    coap_obs_remove_observee(obs_light);
    obs_light = NULL;
    LOG_INFO("[ActuatorCtrl] Light observer removed\n");
  }

    process_exit(&light_actuator_process);
    LOG_INFO("[ActuatorCtrl] light_actuator_process exited\n");

    is_on = 0;
}

void sensor_on(void) {
    if (is_on) {
        LOG_INFO("[ActuatorCtrl] Already ON\n");
        return;
    }

    LOG_INFO("[ActuatorCtrl] Restarting Light Actuator\n");

    if (!process_is_running(&light_actuator_process)) {
        process_start(&light_actuator_process, NULL);
        LOG_INFO("[ActuatorCtrl] light_actuator_process started\n");
    }

    is_on = 1;
}
