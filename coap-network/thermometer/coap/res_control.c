#include "res_control.h"
#include "coap-engine.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process thermometer_process;

static int is_on = 1; // Stato iniziale: attivo

static void res_get_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    // GET: Solo ritorna lo stato, non cambia nulla
    const char *msg = is_on ? "Sensor ON" : "Sensor OFF";
    LOG_INFO("[Control] 📖 GET /ont → %s\n", msg);
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_post_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    // POST: Comando per accendere
    LOG_INFO("[Control] 🔛 POST /ont → Turning ON\n");
    sensor_on();
    const char *msg = "Sensor ON";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_get_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    // GET: Solo ritorna lo stato, non cambia nulla
    const char *msg = is_on ? "Sensor ON" : "Sensor OFF";
    LOG_INFO("[Control] 📖 GET /offt → %s\n", msg);
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_post_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    // POST: Comando per spegnere
    LOG_INFO("[Control] ⏹️ POST /offt → Turning OFF\n");
    sensor_off();
    const char *msg = "Sensor OFF";
    coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

RESOURCE(res_on, "title=\"Sensor Control ON\"", res_get_on, res_post_on, NULL, NULL);
RESOURCE(res_off, "title=\"Sensor Control OFF\"", res_get_off, res_post_off, NULL, NULL);

void sensor_off(void) {
    if (!is_on) {
        LOG_INFO("[SensorCtrl] Already OFF\n");
        return;
    }

    LOG_INFO("[SensorCtrl] Turning OFF sensor\n");

    process_exit(&thermometer_process);
    LOG_INFO("[SensorCtrl] thermometer_process exited\n");

    is_on = 0;
}

void sensor_on(void) {
    if (is_on) {
        LOG_INFO("[SensorCtrl] Already ON\n");
        return;
    }

    LOG_INFO("[SensorCtrl] Restarting sensor process\n");

    // Riavvia solo il processo
    if (!process_is_running(&thermometer_process)) {
        process_start(&thermometer_process, NULL);
        LOG_INFO("[SensorCtrl] thermometer_process started\n");
    }

    is_on = 1;
}