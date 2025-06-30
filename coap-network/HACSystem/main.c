#define LOG_MODULE "Actuator"
#define LOG_LEVEL LOG_LEVEL_APP
#include "contiki.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "cJSON.h"
#include "button-hal.h"
#include "config.h"
#include "observer.h"
#include "logic.h"
#include "res_control.h"

PROCESS(actuator_process, "Perceived Temp Actuator");
AUTOSTART_PROCESSES(&actuator_process);

static int registered = 0;
static int retry_count = 0;

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

static void register_to_cloud() {
  static coap_endpoint_t ep;
  static coap_message_t req[1];
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &ep);

  while (!registered && retry_count < MAX_REGISTRATION_RETRY) {
    coap_init_message(req, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(req, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "temp_actuator");
    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, cJSON_CreateString("actuator/set_threshold"));
    cJSON_AddItemToArray(res, cJSON_CreateString("actuator/get_threshold"));
    cJSON_AddItemToArray(res, cJSON_CreateString("actuator/status"));
    cJSON_AddItemToObject(root, "ss", res);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    coap_set_payload(req, (uint8_t *)payload, strlen(payload));

    LOG_INFO("[Actuator] Sending registration...\n");
    COAP_BLOCKING_REQUEST(&ep, req, client_chunk_handler);
    free(payload);

    if (!registered) {
      retry_count++;
      clock_wait(CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
    }
  }
}

PROCESS_THREAD(actuator_process, ev, data) {
  PROCESS_BEGIN();

  coap_engine_init();
  register_to_cloud();
  button_hal_init();

  if (registered) {
    coap_activate_resource(&res_set_threshold, "actuator/set_threshold");
    coap_activate_resource(&res_get_threshold, "actuator/get_threshold");
    coap_activate_resource(&res_status, "actuator/status");
    observer_start();
  }

  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {
      LOG_INFO("[Actuator] Button pressed, resetting state to none\n");
      logic_reset_status();
    }
  }

  PROCESS_END();
}