#include "contiki.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "../cJSON-master/cJSON.h"
#include "button-hal.h"
#include "config.h"
#include "logic.h"
#include "res-control.h"

#define LOG_MODULE "Actuator"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(actuator_process, "Perceived Temp Actuator");
AUTOSTART_PROCESSES(&actuator_process);

static int registered = 0;
static char temp_ip[64];
static char hum_ip[64];
static coap_endpoint_t temp_ep, hum_ep;
static coap_observee_t *obs_temp = NULL;
static coap_observee_t *obs_hum = NULL;

// === Callback per risposta CoAP ===

void temp_response_handler(coap_message_t *response) {
  if (response == NULL) return;

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      logic_set_temp(value);
    }
  }
}

void hum_response_handler(coap_message_t *response) {
  if (response == NULL) return;

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      logic_set_hum(value);
    }
  }
}

// === Callback per osservazione ===

void temp_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  if (notification) {
    temp_response_handler((coap_message_t *)notification);
  }
}

void hum_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  if (notification) {
    hum_response_handler((coap_message_t *)notification);
  }
}

// === Handler risposta registrazione ===

static void registration_handler(coap_message_t *response) {
  if (!response) return;

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0 && chunk) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';

    cJSON *json = cJSON_Parse(payload);
    if (!json) return;

    cJSON *temp_item = cJSON_GetObjectItemCaseSensitive(json, "temp");
    cJSON *hum_item = cJSON_GetObjectItemCaseSensitive(json, "hum");

    if (cJSON_IsString(temp_item) && cJSON_IsString(hum_item)) {
      snprintf(temp_ip, sizeof(temp_ip), "coap://[%s]:5683", temp_item->valuestring);
      snprintf(hum_ip, sizeof(hum_ip), "coap://[%s]:5683", hum_item->valuestring);

      coap_endpoint_parse(temp_ip, strlen(temp_ip), &temp_ep);
      coap_endpoint_parse(hum_ip, strlen(hum_ip), &hum_ep);
      registered = 1;
    }

    cJSON_Delete(json);
  }
}

// === Processo principale ===

PROCESS_THREAD(actuator_process, ev, data)
{
  static struct etimer init_timer;
  static coap_endpoint_t reg_ep;
  static coap_message_t req[1];
  static int retry = 0;

  PROCESS_BEGIN();

  coap_engine_init();
  button_hal_init();

  // Delay iniziale
  etimer_set(&init_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &reg_ep);

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
    coap_set_payload(req, (uint8_t *)payload, strlen(payload));
    cJSON_Delete(root);

    COAP_BLOCKING_REQUEST(&reg_ep, req, registration_handler);
    free(payload);

    if (!registered) {
      retry++;
      etimer_set(&init_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));
    }
  }

  if (!registered) {
    PROCESS_EXIT();
  }

  // === Avvio osservazione ===
  obs_temp = coap_obs_request_registration(&temp_ep, "/predictionTemp", temp_notification_handler, NULL);
  obs_hum  = coap_obs_request_registration(&hum_ep, "/predictionHum", hum_notification_handler, NULL);

  LOG_INFO("Observation set on temp and hum\n");

  // === Main loop: gestisce il bottone ===
  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == button_hal_press_event) {
      LOG_INFO("Shutdown button pressed!\n");

      // Rimuove osservazione
      if (obs_temp) coap_obs_remove_observee(obs_temp);
      if (obs_hum)  coap_obs_remove_observee(obs_hum);

      LOG_INFO("Observations removed. Exiting process.\n");
      process_exit(&actuator_process);
      PROCESS_EXIT();
    }
  }

  PROCESS_END();
}
