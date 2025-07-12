#include "res-control.h"
#include "coap-engine.h"
#include "../cJSON-master/cJSON.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process light_actuator_process;

static int is_on = 1;

// === Handler CoAP per accensione/spegnimento ===

static void res_post_on(coap_message_t *request, coap_message_t *response,
                        uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "value", "Light actuator ON");
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


static void res_post_off(coap_message_t *request, coap_message_t *response,
                         uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_off();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "value", "Light actuator OFF");
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


static void res_get(coap_message_t *request, coap_message_t *response,
                    uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
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


RESOURCE(res_on, "title=\"Light actuator ON\"", res_get, res_post_on, NULL, NULL);
RESOURCE(res_off, "title=\"Light actuator OFF\"", res_get, res_post_off, NULL, NULL);

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
