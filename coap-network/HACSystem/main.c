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
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(actuator_process, "Perceived Temp Actuator");
AUTOSTART_PROCESSES(&actuator_process);

static int registered = 0;
static char temp_ip[64];
static char hum_ip[64];
static coap_endpoint_t temp_ep, hum_ep;
static coap_observee_t *obs_temp = NULL;
static coap_observee_t *obs_hum = NULL;

// === Callback risposta CoAP ===

void temp_response_handler(coap_message_t *response) {
  if (!response) {
    LOG_WARN("[Temp] No response received\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    LOG_INFO("[Temp] Notification payload: %s\n", buffer);
    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      LOG_INFO("[Temp] Parsed value: %.2f°C\n", value);
      logic_set_temp(value);
    } else {
      LOG_WARN("[Temp] Failed to parse float from: %s\n", buffer);
    }
  } else {
    LOG_WARN("[Temp] Empty payload\n");
  }
}

void hum_response_handler(coap_message_t *response) {
  if (!response) {
    LOG_WARN("[Hum] No response received\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  
  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    LOG_INFO("[Hum] Notification payload: %s\n", buffer);
    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      LOG_INFO("[Hum] Parsed value: %.2f%%\n", value * 100.0f);
      logic_set_hum(value);
    } else {
      LOG_WARN("[Hum] Failed to parse float from: %s\n", buffer);
    }
  } else {
    LOG_WARN("[Hum] Empty payload\n");
  }
}

// === Callback osservazioni ===

void temp_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("[Temp] Notification received\n");
  if (notification) {
    temp_response_handler((coap_message_t *)notification);
  }
}

void hum_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("[Hum] Notification received\n");
  if (notification) {
    hum_response_handler((coap_message_t *)notification);
  }
}

// === Handler registrazione ===

static void registration_handler(coap_message_t *response) {
  LOG_INFO("Registration response received\n");

  if (!response) {
    LOG_ERR("No response to registration request\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0 && chunk) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("Registration payload: %s\n", payload);

    cJSON *json = cJSON_Parse(payload);
    if (!json) {
      LOG_ERR("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
      return;
    }

    cJSON *temp_item = cJSON_GetObjectItemCaseSensitive(json, "temp");
    cJSON *hum_item = cJSON_GetObjectItemCaseSensitive(json, "hum");

    if (!cJSON_IsString(temp_item) || !cJSON_IsString(hum_item)) {
      LOG_WARN("Resources 'temp' and/or 'hum' not available yet. Will retry.\n");
      registered = 0;  
      cJSON_Delete(json);
      return;
    }

    snprintf(temp_ip, sizeof(temp_ip), "coap://[fe80::203:3:3:3%%iface]:5683");
    snprintf(hum_ip, sizeof(hum_ip), "coap://[fe80::204:4:4:4%%iface]:5683");

    coap_endpoint_parse(temp_ip, strlen(temp_ip), &temp_ep);
    coap_endpoint_parse(hum_ip, strlen(hum_ip), &hum_ep);

    LOG_INFO("Parsed temp endpoint: %s\n", temp_ip);
    LOG_INFO("Parsed hum endpoint: %s\n", hum_ip);
    registered = 1;

    cJSON_Delete(json);
  } else {
    LOG_WARN("Empty registration response\n");
    registered = 0;  // anche qui forza retry
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

  LOG_INFO("Actuator process started\n");

  coap_engine_init();
  button_hal_init();

  LOG_INFO("Waiting 10s for network stabilization\n");
  etimer_set(&init_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &reg_ep);

  while (!registered && retry < MAX_REGISTRATION_RETRY) {
    LOG_INFO("Attempting registration (%d/%d)\n", retry + 1, MAX_REGISTRATION_RETRY);

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
      LOG_WARN("Registration failed. Retrying in %d seconds...\n", REGISTRATION_WAIT_SECONDS);
      etimer_set(&init_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));
    }
  }

  if (!registered) {
    LOG_ERR("Registration failed after %d attempts. Exiting process.\n", MAX_REGISTRATION_RETRY);
    PROCESS_EXIT();
  }

  LOG_INFO("Registration successful! Starting resource observation\n");

  obs_temp = coap_obs_request_registration(&temp_ep, "predictionTemp", temp_notification_handler, NULL);
  obs_hum = coap_obs_request_registration(&hum_ep, "predictionHum", hum_notification_handler, NULL);


  if (!obs_temp || !obs_hum) {
    LOG_ERR("Failed to set up observations. Exiting process.\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Observations set up successfully.\n");

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == button_hal_press_event) {
      LOG_WARN("Button pressed → Shutdown initiated\n");

      if (obs_temp) {
        coap_obs_remove_observee(obs_temp);
        LOG_INFO("Temperature observation removed\n");
      }
      if (obs_hum) {
        coap_obs_remove_observee(obs_hum);
        LOG_INFO("Humidity observation removed\n");
      }

      LOG_INFO("Shutdown complete. Exiting actuator process\n");
      process_exit(&actuator_process);
      PROCESS_EXIT();
    }
  }

  PROCESS_END();
}

