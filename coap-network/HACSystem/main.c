#define LOG_MODULE "Actuator"
#define LOG_LEVEL LOG_LEVEL_APP

#include "contiki.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "../cJSON-master/cJSON.h"
#include "button-hal.h"
#include "config.h"
#include "logic.h"
#include "res-control.h"

PROCESS(actuator_process, "Perceived Temp Actuator");
AUTOSTART_PROCESSES(&actuator_process);

static int registered = 0;

/* Callback di registrazione */
static void client_chunk_handler(coap_message_t *response) {
  if (!response) {
    LOG_ERR("[Actuator] Registration timeout\n");
    return;
  }
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Actuator] Registration OK\n");
  } else {
    LOG_WARN("[Actuator] Registration failed\n");
  }
}

/* Callback di risposta per la config osservazioni */
static void obs_response_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (!response) {
    LOG_ERR("[Observer] Failed to get observation config\n");
    return;
  }

  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("[Observer] Received config: %s\n", payload);

  cJSON *root = cJSON_Parse(payload);
  if (!root) {
    LOG_ERR("[Observer] Invalid JSON\n");
    return;
  }

  const cJSON *temp_uri = cJSON_GetObjectItem(root, "obs_temp");
  const cJSON *hum_uri = cJSON_GetObjectItem(root, "obs_hum");

  if (!cJSON_IsString(temp_uri) || !cJSON_IsString(hum_uri)) {
    LOG_ERR("[Observer] Malformed config\n");
    cJSON_Delete(root);
    return;
  }

  coap_obs_request_registration(NULL, (char *)temp_uri->valuestring, NULL, NULL);
  coap_obs_request_registration(NULL, (char *)hum_uri->valuestring, NULL, NULL);

  LOG_INFO("[Observer] Subscribed to temp + hum\n");
  cJSON_Delete(root);
}

PROCESS_THREAD(actuator_process, ev, data) {
  static struct etimer retry_timer;
  static coap_endpoint_t ep, cloud_ep;
  static coap_message_t req[1];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();
  button_hal_init();
  retry = 0;
  registered = 0;

  // --- Registrazione al cloud
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &ep);

  while (!registered && retry < MAX_REGISTRATION_RETRY) {
    coap_init_message(req, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(req, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "temp_actuator");

    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, cJSON_CreateString("actuator/set_threshold"));
    cJSON_AddItemToArray(res, cJSON_CreateString("actuator/get_threshold"));
    cJSON_AddItemToArray(res, cJSON_CreateString("actuator/status"));
    cJSON_AddItemToObject(root, "ss", res);
    cJSON_AddNumberToObject(root, "t", 60); // opzionale

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    coap_set_payload(req, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[Actuator] Sending registration...\n");

    COAP_BLOCKING_REQUEST(&ep, req, client_chunk_handler);
    free(payload);

    if (!registered) {
      retry++;
      etimer_set(&retry_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }
  }

  if (!registered) {
    LOG_WARN("[Actuator] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  // --- Attivazione risorse
  coap_activate_resource(&res_set_threshold, "actuator/set_threshold");
  coap_activate_resource(&res_get_threshold, "actuator/get_threshold");
  coap_activate_resource(&res_status, "actuator/status");

  // --- Richiesta delle URI da osservare
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &cloud_ep);
  coap_init_message(req, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(req, "observeConfig");

  LOG_INFO("[Observer] Requesting observation config...\n");
  COAP_BLOCKING_REQUEST(&cloud_ep, req, obs_response_handler);

  // --- Loop
  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {
      LOG_INFO("[Actuator] Button pressed, resetting state to none\n");
      logic_reset_status();
    }
  }

  PROCESS_END();
}
