#include "res_control.h"
#include "coap-engine.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process humidity_process;

static int is_on = 1; // Stato iniziale: attivo

static void res_post_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();
    const char *msg = "Sensor turned ON";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_post_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_off();
    const char *msg = "Sensor OFF";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_get(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  if(is_on) {
    LOG_INFO("[ActuatorCtrl] Actuator is  ON\n");
    const char *msg = "Actuator is  ON";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
    return;
  }
  else {
    LOG_INFO("[ActuatorCtrl] Actuator is  OFF\n");
    const char *msg = "Actuator is  OFF";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
    return;
  }
}

RESOURCE(res_on, "title=\"Sensor ON\"", res_get, res_post_on, NULL, NULL);
RESOURCE(res_off, "title=\"Sensor OFF\"", res_get, res_post_off, NULL, NULL);

void sensor_off(void) {
    if (!is_on) {
        LOG_INFO("[SensorCtrl] Already OFF\n");
        return;
    }

    LOG_INFO("[SensorCtrl] Turning OFF sensor\n");

    process_exit(&humidity_process);
    LOG_INFO("[SensorCtrl] humidity_process exited\n");

    is_on = 0;
}

void sensor_on(void) {
    if (is_on) {
        LOG_INFO("[SensorCtrl] Already ON\n");
        return;
    }

    LOG_INFO("[SensorCtrl] Restarting sensor process\n");

    // Riavvia solo il processo
    if (!process_is_running(&humidity_process)) {
        process_start(&humidity_process, NULL);
        LOG_INFO("[SensorCtrl] humidity_process started\n");
    }

    is_on = 1;
}