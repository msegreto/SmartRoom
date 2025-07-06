#include "res-control.h"
#include "coap-engine.h"
#include "res-light.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process light_sensor_main_process;

static int is_on = 1; // Stato iniziale: attivo

// === CoAP Handlers ===

static void res_get_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();
    const char *msg = "Sensor ON";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_get_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_off();
    const char *msg = "Sensor OFF";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

RESOURCE(res_on, "title=\"Sensor ON\"", res_get_on, NULL, NULL, NULL);
RESOURCE(res_off, "title=\"Sensor OFF\"", res_get_off, NULL, NULL, NULL);

// === Sensor Control ===

void sensor_off(void) {
    if (!is_on) {
        LOG_INFO("[SensorCtrl] Already OFF\n");
        return;
    }

    LOG_INFO("[SensorCtrl] Turning OFF sensor\n");

    process_exit(&light_sensor_main_process);
    LOG_INFO("[SensorCtrl] light_sensor_main_process exited\n");

    is_on = 0;
}

void sensor_on(void) {
    if (is_on) {
        LOG_INFO("[SensorCtrl] Already ON\n");
        return;
    }

    LOG_INFO("[SensorCtrl] Restarting sensor process\n");

    // Riavvia solo il processo
    if (!process_is_running(&light_sensor_main_process)) {
        process_start(&light_sensor_main_process, NULL);
        LOG_INFO("[SensorCtrl] light_sensor_main_process started\n");
    }

    is_on = 1;
}
