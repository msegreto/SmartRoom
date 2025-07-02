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
    LOG_ERR("[HACSystem] Registration timeout\n");
    return;
  }
  LOG_INFO("[HACSystem] RESPONSE CODE %d\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[HACSystem] Registration OK\n");
  } else {
    LOG_WARN("[HACSystem] Registration failed\n");
  }
}

/* Callback di risposta per la config temperature */
static void temp_response_handler(coap_message_t *response) {
  if (!response) {
    LOG_ERR("[Observer] Failed to get temp config - no response\n");
    return;
  }

  LOG_INFO("[Observer] Temp response code: %d\n", response->code);
  
  if (response->code != CONTENT_2_05) {
    LOG_ERR("[Observer] Temp config failed with code: %d\n", response->code);
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  
  if (len <= 0 || !chunk) {
    LOG_ERR("[Observer] Empty temp config payload\n");
    return;
  }

  char temp_uri[len + 1];
  memcpy(temp_uri, chunk, len);
  temp_uri[len] = '\0';

  LOG_INFO("[Observer] Received temp URI (len=%d): %s\n", len, temp_uri);
  
  // Verifica che l'URI sia valido
  if (strlen(temp_uri) < 10) {
    LOG_ERR("[Observer] Invalid temp URI length\n");
    return;
  }
  
  coap_obs_request_registration(NULL, temp_uri, NULL, NULL);
  LOG_INFO("[Observer] Subscribed to temp URI\n");
}

/* Callback di risposta per la config humidity */
static void hum_response_handler(coap_message_t *response) {
  if (!response) {
    LOG_ERR("[Observer] Failed to get hum config - no response\n");
    return;
  }

  LOG_INFO("[Observer] Hum response code: %d\n", response->code);
  
  if (response->code != CONTENT_2_05) {
    LOG_ERR("[Observer] Hum config failed with code: %d\n", response->code);
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  
  if (len <= 0 || !chunk) {
    LOG_ERR("[Observer] Empty hum config payload\n");
    return;
  }

  char hum_uri[len + 1];
  memcpy(hum_uri, chunk, len);
  hum_uri[len] = '\0';

  LOG_INFO("[Observer] Received hum URI (len=%d): %s\n", len, hum_uri);
  
  // Verifica che l'URI sia valido
  if (strlen(hum_uri) < 10) {
    LOG_ERR("[Observer] Invalid hum URI length\n");
    return;
  }
  
  coap_obs_request_registration(NULL, hum_uri, NULL, NULL);
  LOG_INFO("[Observer] Subscribed to hum URI\n");
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

  // Aspetta che la rete si stabilizzi prima di tentare la registrazione
  LOG_INFO("[HACSystem] Waiting for network initialization...\n");
  etimer_set(&retry_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
  LOG_INFO("[HACSystem] Network initialization complete, starting registration...\n");

  // --- Registrazione al cloud
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &ep);

  while (!registered && retry < MAX_REGISTRATION_RETRY) {
    coap_init_message(req, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(req, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "HACSys");

    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, cJSON_CreateString("set_limit"));
    cJSON_AddItemToArray(res, cJSON_CreateString("get_limit"));
    cJSON_AddItemToArray(res, cJSON_CreateString("sts"));
    cJSON_AddItemToObject(root, "ss", res);
    cJSON_AddNumberToObject(root, "t", 60);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    coap_set_payload(req, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[HACSystem] Sending registration...\n");

    COAP_BLOCKING_REQUEST(&ep, req, client_chunk_handler);
    free(payload);

    if (!registered) {
      retry++;
      etimer_set(&retry_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }
  }

  if (!registered) {
    LOG_WARN("[HACSystem] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  // --- Attivazione risorse
  coap_activate_resource(&res_set_threshold, "actuator/set_limit");
  coap_activate_resource(&res_get_threshold, "actuator/get_limit");
  coap_activate_resource(&res_status, "actuator/sts");

  // --- Richiesta degli URI da osservare (due richieste separate)
  LOG_INFO("[Observer] Configuring cloud endpoint...\n");
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &cloud_ep);
  
  // Aspetta un po' prima di fare le richieste
  etimer_set(&retry_timer, CLOCK_SECOND * 2);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
  
  // Richiesta per temperatura
  LOG_INFO("[Observer] Preparing temp config request...\n");
  coap_init_message(req, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(req, "tempConfig");
  LOG_INFO("[Observer] Requesting temp config...\n");
  COAP_BLOCKING_REQUEST(&cloud_ep, req, temp_response_handler);
  LOG_INFO("[Observer] Temp config request completed\n");

  // Aspetta prima della seconda richiesta
  etimer_set(&retry_timer, CLOCK_SECOND * 1);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));

  // Richiesta per umidità
  LOG_INFO("[Observer] Preparing hum config request...\n");
  coap_init_message(req, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(req, "humConfig");
  LOG_INFO("[Observer] Requesting hum config...\n");
  COAP_BLOCKING_REQUEST(&cloud_ep, req, hum_response_handler);
  LOG_INFO("[Observer] Hum config request completed\n");

  // --- Loop
  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {
      LOG_INFO("[HACSystem] Button pressed, resetting state to none\n");
      logic_reset_status();
    }
  }

  PROCESS_END();
}
