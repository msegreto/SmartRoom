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
  if (!response) return;
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
  if (!response) return;
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

void temp_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  if (notification) temp_response_handler((coap_message_t *)notification);
}

void hum_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  if (notification) hum_response_handler((coap_message_t *)notification);
}

static void registration_handler(coap_message_t *response) {
  registered = 0;
  if (!response) return;
  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0 && chunk) {
    registered = 1;
  }
}

PROCESS_THREAD(actuator_process, ev, data)
{
  static struct etimer init_timer;
  static coap_endpoint_t reg_ep;
  static coap_message_t req[1];
  static int retry = 0;

  PROCESS_BEGIN();

  coap_engine_init();
  button_hal_init();

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

  // Inline service discovery
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

      COAP_BLOCKING_REQUEST(&disc_ep, disc_req, registration_handler);

      // Usa chunk direttamente
      len = coap_get_payload(disc_req, &chunk);
      if (len <= 0 || len >= sizeof(payload)) {
        success = 0;
        break;
      }

      memcpy(payload, chunk, len);
      payload[len] = '\0';

      // ✅ Safe strncpy
      strncpy(ip_targets[i], payload, sizeof(ip_targets[i]) - 1);
      ip_targets[i][sizeof(ip_targets[i]) - 1] = '\0';
      coap_endpoint_parse(ip_targets[i], strlen(ip_targets[i]), ep_targets[i]);
    }

    if (success) break;

    retry++;
    etimer_set(&init_timer, CLOCK_SECOND * 10);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));
  }

  if (retry >= 5) {
    PROCESS_EXIT();
  }

  obs_temp = coap_obs_request_registration(&temp_ep, "predt", temp_notification_handler, NULL);
  obs_hum = coap_obs_request_registration(&hum_ep, "predh", hum_notification_handler, NULL);

  if (!obs_temp || !obs_hum) {
    PROCESS_EXIT();
  }

  while (1) {
    PROCESS_WAIT_EVENT();
    if (ev == button_hal_press_event) {
      if (obs_temp) coap_obs_remove_observee(obs_temp);
      if (obs_hum) coap_obs_remove_observee(obs_hum);
      process_exit(&actuator_process);
      PROCESS_EXIT();
    }
  }

  PROCESS_END();
}
