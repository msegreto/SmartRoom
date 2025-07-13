#include "res-control.h"
#include "coap-engine.h"
#include "logic.h"
#include "contiki.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../cJSON-master/cJSON.h"

#define LOG_MODULE "ResControl"
#define LOG_LEVEL LOG_LEVEL_APP
#include "sys/log.h"

extern struct process actuator_process;
extern coap_observee_t *obs_temp;
extern coap_observee_t *obs_hum;

static int is_on = 1; // inizialmente il processo è attivo

// === HANDLERS COAP ===

static void res_get_handler(coap_message_t *request, coap_message_t *response,  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_event_handler(void);

static void post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  const uint8_t *payload = NULL;
  int len = coap_get_payload(request, &payload);
  if (payload && len > 0) {
    char json_payload[len + 1];
    memcpy(json_payload, payload, len);
    json_payload[len] = '\0';

    cJSON *root = cJSON_Parse(json_payload);
    if (!root) {
      LOG_WARN("[Thresholds] Invalid JSON format: %s\n", json_payload);
    } else {
      cJSON *value_item = cJSON_GetObjectItem(root, "value");
      if (value_item && cJSON_IsString(value_item)) {
        float th_min = 0, th_max = 0;
        if (sscanf(value_item->valuestring, "%f,%f", &th_min, &th_max) == 2) {
          logic_set_thresholds(th_min, th_max);
          LOG_INFO("[Thresholds] Set to: %.2f - %.2f\n", th_min, th_max);
        } else {
          LOG_WARN("[Thresholds] Invalid value format: %s\n", value_item->valuestring);
        }
      } else {
        LOG_WARN("[Thresholds] Missing 'value' string in JSON\n");
      }
      cJSON_Delete(root);
    }
  } else {
    LOG_WARN("[Thresholds] Empty payload in POST\n");
  }

  // Risposta: JSON { "value": "Thresholds updated" }
  cJSON *resp = cJSON_CreateObject();
  cJSON_AddStringToObject(resp, "value", "Thresholds updated");
  char *json_str = cJSON_PrintUnformatted(resp);
  cJSON_Delete(resp);

  if (json_str != NULL) {
    size_t json_len = strlen(json_str);
    memcpy(buffer, json_str, json_len);
    coap_set_payload(response, buffer, json_len);
    coap_set_status_code(response, CONTENT_2_05);
    coap_set_header_content_format(response, APPLICATION_JSON);
  }
}


static void get_threshold_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  float th_min, th_max;
  logic_get_thresholds(&th_min, &th_max);

  char formatted[32];
  snprintf(formatted, sizeof(formatted), "%.2f,%.2f", th_min, th_max);

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "value", formatted);
  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (json_str != NULL) {
    size_t json_len = strlen(json_str);
    memcpy(buffer, json_str, json_len);
    coap_set_payload(response, buffer, json_len);
    coap_set_status_code(response, CONTENT_2_05);
    coap_set_header_content_format(response, APPLICATION_JSON);
  }
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

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  warming_state_t current = logic_get_state();
  const char *status_str = NULL;

  switch (current) {
    case WARMING_COOLING: status_str = "cooling"; break;
    case WARMING_HEATING: status_str = "heating"; break;
    default: status_str = "none"; break;
  }

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "value", status_str);
  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (json_str != NULL) {
    size_t json_len = strlen(json_str);
    memcpy(buffer, json_str, json_len);
    coap_set_payload(response, buffer, json_len);
    coap_set_status_code(response, CONTENT_2_05);
    coap_set_header_content_format(response, APPLICATION_JSON);
    LOG_INFO("[Status] JSON Payload: %s\n", json_str);
  } else {
    LOG_WARN("[Status] Failed to format JSON response\n");
  }

  LOG_INFO("[Status] GET handled\n");
}

// === ON / OFF COAP HANDLERS ===

static void res_post_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
    sensor_on();

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "value", "HAC system ON");
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
    cJSON_AddStringToObject(root, "value", "HAC system OFF");
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

RESOURCE(res_on, "title=\"Actuator ON\"", res_get, res_post_on, NULL, NULL);
RESOURCE(res_off, "title=\"Actuator OFF\"", res_get, res_post_off, NULL, NULL);

// === SENSOR CONTROL FUNCTIONS ===

void sensor_off(void) {
  if (!is_on) {
    LOG_INFO("[ActuatorCtrl] Already OFF\n");
    return;
  }

  LOG_INFO("[ActuatorCtrl] Turning OFF actuator process\n");

  // Rimuove osservazioni CoAP se presenti
  if (obs_temp) {
    coap_obs_remove_observee(obs_temp);
    obs_temp = NULL;
    LOG_INFO("[ActuatorCtrl] Temperature observer removed\n");
  }

  if (obs_hum) {
    coap_obs_remove_observee(obs_hum);
    obs_hum = NULL;
    LOG_INFO("[ActuatorCtrl] Humidity observer removed\n");
  }

  // Termina il processo principale
  process_exit(&actuator_process);
  LOG_INFO("[ActuatorCtrl] actuator_process exited\n");

  is_on = 0;
}

void sensor_on(void) {
  if (is_on) {
    LOG_INFO("[ActuatorCtrl] Already ON\n");
    return;
  }

  LOG_INFO("[ActuatorCtrl] Restarting actuator process\n");

  // Riavvia il processo se non è attivo
  if (!process_is_running(&actuator_process)) {
    process_start(&actuator_process, NULL);
    LOG_INFO("[ActuatorCtrl] actuator_process started\n");
  }

  is_on = 1;
}