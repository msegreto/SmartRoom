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

void temp_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("[Temp] Notification received\n");
  if (notification) temp_response_handler((coap_message_t *)notification);
}

void hum_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("[Hum] Notification received\n");
  if (notification) hum_response_handler((coap_message_t *)notification);
}

static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("[HACSystem] === RESPONSE HANDLER CALLED ===\n");

  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[HACSystem] Registration timed out - no response received\n");
    return;
  }

  LOG_INFO("[HACSystem] Response received! Code: %d\n", response->code);

  int len = coap_get_payload(response, &chunk);
  if (len <= 0 || chunk == NULL) {
    LOG_WARN("[HACSystem] Empty or invalid payload received (len=%d)\n", len);
  } else {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[HACSystem] Response payload: '%s'\n", payload);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[HACSystem] Registration successful!\n");
  } else {
    LOG_WARN("[HACSystem] Registration failed with code: %d\n", response->code);
  }

  LOG_INFO("[HACSystem] === RESPONSE HANDLER END ===\n");
}

PROCESS_THREAD(actuator_process, ev, data)
{
  static struct etimer timer, init_timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;

  PROCESS_BEGIN();

  LOG_INFO("Actuator process started\n");

  coap_engine_init();
  button_hal_init();

  LOG_INFO("[HACSystem] Waiting for network establishment...\n");
  etimer_set(&timer, CLOCK_SECOND * 10); // Wait 10 seconds for network
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  LOG_INFO("[HACSystem] Network wait complete, starting registration\n");

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  registered = 0;
  retry = 0;

  while (retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "HACSys");
    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, cJSON_CreateString("set_limit"));
    cJSON_AddItemToArray(res, cJSON_CreateString("get_limit"));
    cJSON_AddItemToArray(res, cJSON_CreateString("sts"));
    cJSON_AddItemToObject(root, "ss", res);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[HACSystem] Failed to create payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[HACSystem] JSON payload (len=%zu): %s\n", strlen(payload), payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[HACSystem] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if (!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if (!registered) {
    LOG_WARN("[HACSystem] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Registration successful!\n");


  retry = 0;
  while (retry < 5) {
    static coap_endpoint_t disc_ep;
    static coap_message_t disc_req[1];
    const uint8_t *chunk;
    int len;
    char payload[128];

    const char *services[] = {"predt", "predh"};
    char *ip_targets[] = {temp_ip, hum_ip};
    coap_endpoint_t *ep_targets[] = {&temp_ep, &hum_ep};

    coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &disc_ep);
    int success = 1;

    for (int i = 0; i < 2; i++) {
      char uri[32];
      snprintf(uri, sizeof(uri), "/services/%s", services[i]);

      coap_init_message(disc_req, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(disc_req, uri);

      LOG_INFO("Sending GET request to %s\n", uri);
      COAP_BLOCKING_REQUEST(&disc_ep, disc_req, registration_handler);

      len = coap_get_payload(disc_req, &chunk);
      if (len <= 0 || len >= sizeof(payload)) {
        LOG_WARN("Invalid or empty response for %s\n", services[i]);
        success = 0;
        break;
      }

      memcpy(payload, chunk, len);
      payload[len] = '\0';

      LOG_INFO("Received IP for %s: %s\n", services[i], payload);

      strncpy(ip_targets[i], payload, sizeof(ip_targets[i]) - 1);
      ip_targets[i][sizeof(ip_targets[i]) - 1] = '\0';
      coap_endpoint_parse(ip_targets[i], strlen(ip_targets[i]), ep_targets[i]);
    }

    if (success) break;

    retry++;
    LOG_WARN("Discovery failed. Retrying in 10 seconds (%d/5)...\n", retry);
    etimer_set(&init_timer, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));
  }

  if (retry >= 5) {
    LOG_ERR("Could not discover required services after multiple attempts\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Starting observation of discovered services\n");

  obs_temp = coap_obs_request_registration(&temp_ep, "predt", temp_notification_handler, NULL);
  obs_hum = coap_obs_request_registration(&hum_ep, "predh", hum_notification_handler, NULL);

  if (!obs_temp || !obs_hum) {
    LOG_ERR("Failed to set up observations. Exiting process.\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Observations set up successfully.\n");

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == button_hal_press_event) {
      LOG_WARN("Button pressed: Shutdown initiated\n");

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
