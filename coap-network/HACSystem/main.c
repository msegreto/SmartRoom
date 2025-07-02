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

void temp_notification_handler(struct coap_observee_s *observee, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("Temperature notification handler triggered\n");

  switch(flag) {
    case NOTIFICATION_OK:
      LOG_INFO("Notification flag: OK\n");
      break;
    case OBSERVE_OK:
      LOG_INFO("Notification flag: OBSERVE_OK\n");
      break;
    case NOTIFICATION_NON:
      LOG_INFO("Notification flag: NON\n");
      break;
    case OBSERVE_NOT_SUPPORTED:
      LOG_WARN("Observation not supported by server\n");
      break;
    case OBSERVE_TERMINATED:
      LOG_WARN("Observation terminated by server\n");
      break;
    default:
      LOG_WARN("Unknown notification flag: %d\n", flag);
  }

  coap_message_t *msg = (coap_message_t *)notification;
  if (msg) {
    LOG_INFO("Received temperature notification\n");

    const uint8_t *payload;
    int len = coap_get_payload(msg, &payload);
    LOG_INFO("Payload length: %d\n", len);

    if (len > 0) {
      char buffer[32];
      memcpy(buffer, payload, len);
      buffer[len] = '\0';
      float temp_val;
      if (sscanf(buffer, "%f", &temp_val) == 1) {
        LOG_INFO("Parsed temperature: %.2f°C\n", temp_val);
        logic_set_temp(temp_val);
        process_poll(&actuator_process);
      } else {
        LOG_WARN("Temp payload invalid: %s\n", buffer);
      }
    } else {
      LOG_WARN("Empty temperature payload\n");
    }
  } else {
    LOG_WARN("No temperature notification received\n");
  }
}

void hum_notification_handler(struct coap_observee_s *observee, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("Humidity notification handler triggered\n");

  switch(flag) {
    case NOTIFICATION_OK:
      LOG_INFO("Notification flag: OK\n");
      break;
    case OBSERVE_OK:
      LOG_INFO("Notification flag: OBSERVE_OK\n");
      break;
    case NOTIFICATION_NON:
      LOG_INFO("Notification flag: NON\n");
      break;
    case OBSERVE_NOT_SUPPORTED:
      LOG_WARN("Observation not supported by server\n");
      break;
    case OBSERVE_TERMINATED:
      LOG_WARN("Observation terminated by server\n");
      break;
    default:
      LOG_WARN("Unknown notification flag: %d\n", flag);
  }

  coap_message_t *msg = (coap_message_t *)notification;
  if (msg) {
    LOG_INFO("Received humidity notification\n");

    const uint8_t *payload;
    int len = coap_get_payload(msg, &payload);
    LOG_INFO("Payload length: %d\n", len);

    if (len > 0) {
      char buffer[32];
      memcpy(buffer, payload, len);
      buffer[len] = '\0';
      float hum_val;
      if (sscanf(buffer, "%f", &hum_val) == 1) {
        LOG_INFO("Parsed humidity: %.2f%%\n", hum_val * 100.0f);
        logic_set_hum(hum_val);
        process_poll(&actuator_process);
      } else {
        LOG_WARN("Hum payload invalid: %s\n", buffer);
      }
    } else {
      LOG_WARN("Empty humidity payload\n");
    }
  } else {
    LOG_WARN("No humidity notification received\n");
  }
}

// ==== Registration Response Handler ====

static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("Registration response handler triggered\n");

  if (!response) {
    LOG_ERR("Registration timeout - no response\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  LOG_INFO("Received response payload of length: %d\n", len);

  if (len > 0 && chunk) {
    LOG_INFO("Received registration payload: %.*s\n", len, chunk);
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
      LOG_INFO("Parsed temp URI: %s\n", temp_uri);
      snprintf(hum_uri, sizeof(hum_uri), "coap://[%s]:5683/predictionHum", hum_item->valuestring);
      LOG_INFO("Parsed hum URI: %s\n", hum_uri);

      static coap_endpoint_t temp_ep, hum_ep;
      coap_endpoint_parse(temp_uri, strlen(temp_uri), &temp_ep);
      coap_endpoint_parse(hum_uri, strlen(hum_uri), &hum_ep);

      obs_temp = coap_obs_request_registration(&temp_ep, "/predictionTemp", temp_notification_handler, NULL);
      if (obs_temp != NULL) LOG_INFO("Observation to /predictionTemp registered successfully\n");
      else LOG_WARN("Observation to /predictionTemp failed\n");

      obs_hum = coap_obs_request_registration(&hum_ep, "/predictionHum", hum_notification_handler, NULL);
      if (obs_hum != NULL) LOG_INFO("Observation to /predictionHum registered successfully\n");
      else LOG_WARN("Observation to /predictionHum failed\n");

      registered = 1;
    } else {
      LOG_ERR("Invalid JSON or missing fields\n");
    }

    cJSON_Delete(json);
  } else {
    LOG_WARN("No payload received in registration response\n");
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

  LOG_INFO("Actuator process started\n");

  coap_engine_init();
  LOG_INFO("CoAP engine initialized\n");

  button_hal_init();
  LOG_INFO("Button HAL initialized\n");

  LOG_INFO("Waiting for network...\n");
  etimer_set(&retry_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));

  LOG_INFO("Starting registration process...\n");
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &ep);

  while (!registered && retry < MAX_REGISTRATION_RETRY) {
    LOG_INFO("Registration attempt #%d\n", retry + 1);
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

    COAP_BLOCKING_REQUEST(&ep, req, client_chunk_handler);
    free(payload);

    if (!registered) {
      LOG_WARN("Registration attempt %d failed, retrying...\n", retry + 1);
      retry++;
      etimer_set(&retry_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
    }
  }

  if (!registered) {
    LOG_ERR("Max registration attempts reached, exiting process\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Actuator process completed and registered\n");
  PROCESS_END();
}
