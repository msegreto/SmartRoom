#define LOG_MODULE "Actuator"
#define LOG_LEVEL LOG_LEVEL_APP

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

PROCESS(actuator_process, "Perceived Temp Actuator");
AUTOSTART_PROCESSES(&actuator_process);

static int registered = 0;


static void temp_notification_handler(struct coap_observee *obs, void *data, coap_notification_flag_t flag) {
  const uint8_t *payload = coap_observee_notification_payload(obs);
  int len = coap_observee_notification_payload_length(obs);

  if(payload && len > 0) {
    char buffer[32];
    memcpy(buffer, payload, len);
    buffer[len] = '\0';
    float temp_val = 0;
    if(sscanf(buffer, "%f", &temp_val) == 1) {
      LOG_INFO("[HACSystem] Temp notify: %.2f°C\n", temp_val);
      logic_set_temp(temp_val);
    } else {
      LOG_WARN("[HACSystem] Temp notify: invalid payload '%s'\n", buffer);
    }
  } else {
    LOG_WARN("[HACSystem] Temp notify: empty or null payload\n");
  }
}

static void hum_notification_handler(struct coap_observee *obs, void *data, coap_notification_flag_t flag) {
  const uint8_t *payload = coap_observee_notification_payload(obs);
  int len = coap_observee_notification_payload_length(obs);

  if(payload && len > 0) {
    char buffer[32];
    memcpy(buffer, payload, len);
    buffer[len] = '\0';
    float hum_val = 0;
    if(sscanf(buffer, "%f", &hum_val) == 1) {
      LOG_INFO("[HACSystem] Hum notify: %.2f%%\n", hum_val * 100.0f);
      logic_set_hum(hum_val);
    } else {
      LOG_WARN("[HACSystem] Hum notify: invalid payload '%s'\n", buffer);
    }
  } else {
    LOG_WARN("[HACSystem] Hum notify: empty or null payload\n");
  }
}



static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("[HACSystem] === REGISTRATION CALLBACK INVOKED ===\n");
  
  if (!response) {
    LOG_ERR("[HACSystem] Registration timeout - no response\n");
    return;
  }
  
  LOG_INFO("[HACSystem] RESPONSE CODE %d\n", response->code);
  
  if (response->code == REGISTRATION_ACK_CODE) {
    const uint8_t *chunk;
    int len = coap_get_payload(response, &chunk);
    
    if (len > 0 && chunk) {
      char payload[len + 1];
      memcpy(payload, chunk, len);
      payload[len] = '\0';

      cJSON *json = cJSON_Parse(payload);
      if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        LOG_ERR("[HACSystem] JSON parsing error: %s\n", error_ptr ? error_ptr : "unknown");
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

        coap_obs_request_registration(&temp_ep, "/predictionTemp", temp_notification_handler, NULL);
        coap_obs_request_registration(&hum_ep, "/predictionHum", hum_notification_handler, NULL);

        registered = 1;
      } else {
        LOG_ERR("[HACSystem] Invalid JSON format or missing keys\n");
      }

      cJSON_Delete(json);
    } else {
      registered = 1; // fallback
    }
  } else {
    LOG_WARN("[HACSystem] Registration failed with code: %d\n", response->code);
  }

  LOG_INFO("[HACSystem] === REGISTRATION CALLBACK COMPLETED ===\n");
}



PROCESS_THREAD(actuator_process, ev, data) {
  static struct etimer retry_timer;
  static coap_endpoint_t ep;
  static coap_message_t req[1];
  static int retry;

  PROCESS_BEGIN();

  coap_engine_init();
  button_hal_init();
  retry = 0;
  registered = 0;

  LOG_INFO("[HACSystem] Waiting for network initialization...\n");
  etimer_set(&retry_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&retry_timer));
  LOG_INFO("[HACSystem] Starting registration...\n");

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

  LOG_INFO("[HACSystem] Activating resources\n");

  coap_activate_resource(&res_set_threshold, "actuator/set_limit");
  coap_activate_resource(&res_get_threshold, "actuator/get_limit");
  coap_activate_resource(&res_status, "actuator/sts");

  LOG_INFO("[HACSystem] All resources activated, entering main loop\n");

  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {
      LOG_INFO("[HACSystem] Button pressed, resetting state to none\n");
      logic_reset_status();
    }
  }

  PROCESS_END();
}
