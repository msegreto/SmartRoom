#include "contiki.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "coap-observe-client.h"
#include "../cJSON-master/cJSON.h"
#include "button-hal.h"
#include "config.h"
#include "logic.h"
#include "res-control.h"

#define LOG_MODULE "Actuator"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(actuator_process, "Perceived Temp Actuator");
AUTOSTART_PROCESSES(&actuator_process);

static coap_observee_t *obs_temp = NULL;
static coap_observee_t *obs_hum = NULL;
static int registered = 0;

// ==== Notification Handlers ====

static void temp_notification_handler(coap_observee_t *obs, void *data, coap_notification_flag_t flag) {
  const uint8_t *payload = coap_observee_notification_payload(obs);
  int len = coap_observee_notification_payload_length(obs);

  if(payload && len > 0) {
    char buffer[32];
    memcpy(buffer, payload, len);
    buffer[len] = '\0';
    float temp_val;
    if(sscanf(buffer, "%f", &temp_val) == 1) {
      LOG_INFO("Temp notify: %.2f°C\n", temp_val);
      logic_set_temp(temp_val);
      process_poll(&actuator_process);
    } else {
      LOG_WARN("Temp notify invalid payload: %s\n", buffer);
    }
  }
}

static void hum_notification_handler(coap_observee_t *obs, void *data, coap_notification_flag_t flag) {
  const uint8_t *payload = coap_observee_notification_payload(obs);
  int len = coap_observee_notification_payload_length(obs);

  if(payload && len > 0) {
    char buffer[32];
    memcpy(buffer, payload, len);
    buffer[len] = '\0';
    float hum_val;
    if(sscanf(buffer, "%f", &hum_val) == 1) {
      LOG_INFO("Hum notify: %.2f%%\n", hum_val * 100.0f);
      logic_set_hum(hum_val);
      process_poll(&actuator_process);
    } else {
      LOG_WARN("Hum notify invalid payload: %s\n", buffer);
    }
  }
}

// ==== Registration Response Handler ====

static void client_chunk_handler(coap_message_t *response) {
  if (!response) {
    LOG_ERR("Registration timeout - no response\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);

  if (len > 0 && chunk) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';

    cJSON *json = cJSON_Parse(payload);
    if (!json) {
      LOG_ERR("JSON parsing error: %s\n", cJSON_GetErrorPtr());
      return;
    }

    cJSON *temp_item = cJSON_GetObjectItemCaseSensitive(json, "temp");
    cJSON *hum_item = cJSON_GetObjectItemCaseSensitive(json, "hum");

    if (cJSON_IsString(temp_item) && cJSON_IsString(hum_item)) {
      static char temp_uri[100], hum_uri[100];
      snprintf(temp_uri, sizeof(temp_uri), "coap://[%s]:5683/predictionTemp", temp_item->valuestring);
      snprintf(hum_uri, sizeof(hum_uri), "coap://[%s]:5683/predictionHum", hum_item->valuestring);

      static coap_endpoint_t temp_ep, hum_ep;
      coap_endpoint_parse(temp_uri, strlen(temp_uri), &temp_ep);
      coap_endpoint_parse(hum_uri, strlen(hum_uri), &hum_ep);

      obs_temp = coap_obs_request_registration(&temp_ep, "/predictionTemp", temp_notification_handler, NULL);
      obs_hum = coap_obs_request_registration(&hum_ep, "/predictionHum", hum_notification_handler, NULL);

      registered = 1;
    } else {
      LOG_ERR("Invalid JSON or missing fields\n");
    }

    cJSON_Delete(json);
  }
}

// ==== Main Process Thread ====

PROCESS_THREAD(actuator_process, ev, data)
{
  static struct etimer retry_timer;
  static coap_endpoint_t ep;
  static coap_message_t req[1];
  static int retry = 0;

  PROCESS_BEGIN();

  coap_engine_init();
  button_hal_init();

  LOG_INFO("Waiting for network...\n");
  etimer_set(&retry_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));

  LOG_INFO("Starting registration...\n");
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
    coap_set_payload(req, (uint8_t *)payload, strlen(payload));
    cJSON_Delete(root);

    COAP_BLOCKING_REQUEST(&ep, req, client_chunk_handler);
    free(payload);

    if (!registered) {
      retry++;
      etimer_set(&retry_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }
  }

  if (!registered) {
    LOG_WARN("Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Activating resources...\n");
  coap_activate_resource(&res_set_threshold, "actuator/set_limit");
  coap_activate_resource(&res_get_threshold, "actuator/get_limit");
  coap_activate_resource(&res_status, "actuator/sts");

  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {
      LOG_INFO("Button pressed - resetting logic state\n");
      logic_reset_status();
    }
  }

  PROCESS_END();
}
