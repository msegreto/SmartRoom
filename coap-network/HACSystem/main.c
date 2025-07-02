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

// === Callback per temperatura ===
void temp_response_handler(coap_message_t *response) {
  LOG_INFO("[Temp] Callback invoked\n");

  if (response == NULL) {
    LOG_ERR("[Temp] No response received\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  LOG_INFO("[Temp] Payload length: %d\n", len);

  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    LOG_INFO("[Temp] Raw payload: %s\n", buffer);

    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      LOG_INFO("[Temp] Parsed value: %.2f°C\n", value);
      logic_set_temp(value);
    } else {
      LOG_WARN("[Temp] Failed to parse payload: %s\n", buffer);
    }
  } else {
    LOG_WARN("[Temp] Empty payload\n");
  }
}

// === Callback per umidità ===
void hum_response_handler(coap_message_t *response) {
  LOG_INFO("[Hum] Callback invoked\n");

  if (response == NULL) {
    LOG_ERR("[Hum] No response received\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  LOG_INFO("[Hum] Payload length: %d\n", len);

  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    LOG_INFO("[Hum] Raw payload: %s\n", buffer);

    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      LOG_INFO("[Hum] Parsed value: %.2f%%\n", value * 100.0f);
      logic_set_hum(value);
    } else {
      LOG_WARN("[Hum] Failed to parse payload: %s\n", buffer);
    }
  } else {
    LOG_WARN("[Hum] Empty payload\n");
  }
}

// === Handler risposta registrazione ===
static void registration_handler(coap_message_t *response) {
  LOG_INFO("Registration response handler triggered\n");

  if (!response) {
    LOG_ERR("No response to registration\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);

  if (len > 0 && chunk) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';

    LOG_INFO("Registration response: %s\n", payload);

    cJSON *json = cJSON_Parse(payload);
    if (!json) {
      LOG_ERR("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
      return;
    }

    cJSON *temp_item = cJSON_GetObjectItemCaseSensitive(json, "temp");
    cJSON *hum_item = cJSON_GetObjectItemCaseSensitive(json, "hum");

    if (cJSON_IsString(temp_item) && cJSON_IsString(hum_item)) {
      snprintf(temp_ip, sizeof(temp_ip), "coap://[%s]:5683", temp_item->valuestring);
      snprintf(hum_ip, sizeof(hum_ip), "coap://[%s]:5683", hum_item->valuestring);

      coap_endpoint_parse(temp_ip, strlen(temp_ip), &temp_ep);
      coap_endpoint_parse(hum_ip, strlen(hum_ip), &hum_ep);

      LOG_INFO("Temp endpoint parsed: %s\n", temp_ip);
      LOG_INFO("Hum endpoint parsed: %s\n", hum_ip);
      registered = 1;
    } else {
      LOG_ERR("Missing or invalid 'temp' or 'hum' fields in JSON\n");
    }

    cJSON_Delete(json);
  } else {
    LOG_WARN("Empty registration response payload\n");
  }
}

// === Processo principale ===

PROCESS_THREAD(actuator_process, ev, data)
{
  static struct etimer timer;
  static coap_endpoint_t reg_ep;
  static coap_message_t req[1];
  static int retry = 0;

  PROCESS_BEGIN();

  LOG_INFO("Starting actuator process\n");

  coap_engine_init();
  LOG_INFO("CoAP engine initialized\n");

  button_hal_init();
  LOG_INFO("Button HAL initialized\n");

  // Delay iniziale per assicurarsi che la rete sia pronta
  LOG_INFO("Waiting 10 seconds for network setup...\n");
  etimer_set(&timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &reg_ep);

  while (!registered && retry < MAX_REGISTRATION_RETRY) {
    LOG_INFO("Attempting registration: try #%d\n", retry + 1);

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
    LOG_INFO("Sending registration payload: %s\n", payload);
    coap_set_payload(req, (uint8_t *)payload, strlen(payload));
    cJSON_Delete(root);

    COAP_BLOCKING_REQUEST(&reg_ep, req, registration_handler);
    free(payload);

    if (!registered) {
      retry++;
      LOG_WARN("Registration attempt failed, retrying in %d seconds\n", REGISTRATION_WAIT_SECONDS);
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if (!registered) {
    LOG_ERR("Registration failed after %d attempts. Exiting...\n", MAX_REGISTRATION_RETRY);
    PROCESS_EXIT();
  }

  LOG_INFO("Registration successful! Starting data polling every 30 seconds\n");

  etimer_set(&timer, CLOCK_SECOND * 30);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    // === Richiesta temperatura ===
    LOG_INFO("[Temp] Sending GET request to /predictionTemp\n");
    LOG_INFO("[Temp] Endpoint: %s\n", temp_ip);
    coap_init_message(req, COAP_TYPE_CON, COAP_GET, 0);
    coap_set_header_uri_path(req, "/predictionTemp");
    COAP_BLOCKING_REQUEST(&temp_ep, req, temp_response_handler);

    // === Richiesta umidità ===
    LOG_INFO("[Hum] Sending GET request to /predictionHum\n");
    LOG_INFO("[Hum] Endpoint: %s\n", hum_ip);
    coap_init_message(req, COAP_TYPE_CON, COAP_GET, 0);
    coap_set_header_uri_path(req, "/predictionHum");
    COAP_BLOCKING_REQUEST(&hum_ep, req, hum_response_handler);

    etimer_reset(&timer);
  }

  PROCESS_END();
}
