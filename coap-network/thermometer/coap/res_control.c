#include "res_control.h"
#include "coap-engine.h"
#include "../../cJSON-master/cJSON.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process thermometer_process;

static int is_on = 1; // Stato iniziale: attivo

static void res_post_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "value", "Sensor turned ON");
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str != NULL) {
        size_t len = strlen(json_str);
        memcpy(buffer, json_str, len);
        coap_set_payload(response, buffer, len);
        coap_set_status_code(response, CONTENT_2_05);
        coap_set_header_content_format(response, APPLICATION_JSON);
    }
}

static void res_post_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_off();
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "value", "Sensor turned OFF");
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str != NULL) {
        size_t len = strlen(json_str);
        memcpy(buffer, json_str, len);
        coap_set_payload(response, buffer, len);
        coap_set_status_code(response, CONTENT_2_05);
        coap_set_header_content_format(response, APPLICATION_JSON);
    }
}

static void res_get(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  
    const char *status_str = is_on ? "Actuator is ON" : "Actuator is OFF";
    LOG_INFO("[ActuatorCtrl] %s\n", status_str);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "value", status_str);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str != NULL) {
        size_t len = strlen(json_str);
        memcpy(buffer, json_str, len);
        coap_set_payload(response, buffer, len);
        coap_set_status_code(response, CONTENT_2_05);
        coap_set_header_content_format(response, APPLICATION_JSON);
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