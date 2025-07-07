#include "res-control.h"
#include "coap-engine.h"
#include "logic.h"
#include "contiki.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    float th_min = 0, th_max = 0;
    sscanf((const char *)payload, "%f,%f", &th_min, &th_max);
    logic_set_thresholds(th_min, th_max);
  }
  const char *msg = "Thresholds updated";
  coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void get_threshold_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
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

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
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

// === ON / OFF COAP HANDLERS ===

static void res_post_on(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  sensor_on();
  const char *msg = "Actuator ON";
  coap_set_payload(response, (uint8_t *)msg, strlen(msg));
}

static void res_post_off(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  sensor_off();
  const char *msg = "Actuator OFF";
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