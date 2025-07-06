#include "contiki.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "config.h"
#include "res-control.h"
#include "leds.h"
#include "../cJSON-master/cJSON.h" 

#define LOG_MODULE "LightActuator"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(light_actuator_process, "Light Actuator");
AUTOSTART_PROCESSES(&light_actuator_process);

static int registered = 0;
static char light_ip[64];
static coap_endpoint_t light_ep;
coap_observee_t *obs_light = NULL;

static char light_service_payload[128] = "";

static void print_hex(const uint8_t *data, int len) {
  printf("[HEX] ");
  for (int i = 0; i < len; ++i) {
    printf("%02X ", data[i]);
  }
  printf("\n");
}

void discovery_response_handler(coap_message_t *response, char *buffer, size_t buffer_len) {
  if (!response || !buffer) {
    LOG_WARN("[DISCOVERY] No response or buffer null\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len <= 0 || len >= buffer_len) {
    LOG_WARN("[DISCOVERY] Invalid or empty payload\n");
    buffer[0] = '\0';
    return;
  }

  memcpy(buffer, chunk, len);
  buffer[len] = '\0';
  
  
  if (strstr(buffer, "not found") != NULL) {
    LOG_WARN("[DISCOVERY] Resource not found in response: %s\n", buffer);
    buffer[0] = '\0';  
    return;
  }

  // Stampa solo se la risposta è valida
  print_hex(chunk, len);
  LOG_INFO("[DISCOVERY] Payload (STRING): %s\n", buffer);
}

void discovery_response_handler_light(coap_message_t *response) {
  discovery_response_handler(response, light_service_payload, sizeof(light_service_payload));
}

void light_response_handler(coap_message_t *response) {
  if (!response) {
    LOG_WARN("[Light] No response received\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0) {
    LOG_INFO("[Light] Notification payload (RAW): %.*s\n", len, chunk);

    if (len == 1 && chunk[0] == '1') {
      //leds_on(LEDS_GREEN);
      LOG_INFO("[Light] LED ON (from notification)\n");
    } else {
      //leds_off(LEDS_GREEN);
      LOG_INFO("[Light] LED OFF (from notification)\n");
    }
  } else {
    LOG_WARN("[Light] Empty payload\n");
  }
}

void light_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("[Light] Notification received\n");
  if (notification) light_response_handler((coap_message_t *)notification);
}

static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("[LightSystemAct] === RESPONSE HANDLER CALLED ===\n");

  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[LightSystemAct] Registration timed out - no response received\n");
    return;
  }

  LOG_INFO("[LightSystemAct] Response received! Code: %d\n", response->code);

  int len = coap_get_payload(response, &chunk);
  if (len > 0 && chunk != NULL) {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[LightSystemAct] Response payload: '%s'\n", payload);
  } else {
    LOG_WARN("[LightSystemAct] Empty or invalid payload received (len=%d)\n", len);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[LightSystemAct] Registration successful!\n");
  } else {
    LOG_WARN("[LightSystemAct] Registration failed with code: %d\n", response->code);
  }

  LOG_INFO("[LightSystemAct] === RESPONSE HANDLER END ===\n");
}

PROCESS_THREAD(light_actuator_process, ev, data) {
  static struct etimer timer, init_timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;
  static int success;

  PROCESS_BEGIN();

  // Initialization
  LOG_INFO("Light Actuator starting...\n");

  coap_engine_init();
  
  LOG_INFO("[LightSystemAct] Waiting for network establishment...\n");
  etimer_set(&timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  LOG_INFO("[LightSystemAct] Network wait complete, starting registration\n");
  
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  registered = 0;
  retry = 0;

  while (retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "LightActuator");
    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, cJSON_CreateString("led"));
    cJSON_AddItemToArray(res, cJSON_CreateString("onlightactuator"));
    cJSON_AddItemToArray(res, cJSON_CreateString("offlightactuator"));
    cJSON_AddItemToObject(root, "ss", res);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (payload == NULL) {
      LOG_ERR("[LightSystemAct] Failed to create payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[LightSystemAct] JSON payload (len=%zu): %s\n", strlen(payload), payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[LightSystemAct] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if (!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if (!registered) {
    LOG_WARN("[LightSystemAct] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Registration successful!\n");

  // Attiva la risorsa LED
  extern coap_resource_t res_led;
  coap_activate_resource(&res_led, "led");
  coap_activate_resource(&res_on, "onlightactuator");
  coap_activate_resource(&res_off, "offlightactuator");
  
  LOG_INFO("[LightSystemAct] Starting service discovery for light\n");

  retry = 0;
  success = 0;

  do {
    static coap_endpoint_t disc_ep_light;
    static coap_message_t disc_req_light[1];
    success = 1;

    if (!coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &disc_ep_light)) {
      LOG_ERR("[DISCOVERY] Failed to parse CLOUD_SERVER_EP for light\n");
      success = 0;
    } else {
      coap_init_message(disc_req_light, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(disc_req_light, SERVICE_DISCOVERY_PATH);
      coap_set_header_uri_query(disc_req_light, QUERY_LIGHT);
      LOG_INFO("[DISCOVERY] Sending GET to %s?%s\n", SERVICE_DISCOVERY_PATH, QUERY_LIGHT);
      COAP_BLOCKING_REQUEST(&disc_ep_light, disc_req_light, discovery_response_handler_light);

      if (strlen(light_service_payload) == 0) {
        LOG_WARN("[DISCOVERY] Empty or invalid response for light\n");
        success = 0;
      } else if (!coap_endpoint_parse(light_service_payload, strlen(light_service_payload), &light_ep)) {
        LOG_WARN("[DISCOVERY] Failed to parse endpoint for light\n");
        success = 0;
      } else {
        strncpy(light_ip, light_service_payload, sizeof(light_ip) - 1);
        light_ip[sizeof(light_ip) - 1] = '\0';
        LOG_INFO("[DISCOVERY] Parsed light IP: %s\n", light_ip);
      }
    }

    if (!success) {
      retry++;
      LOG_WARN("[DISCOVERY] Failed. Retrying in 10 seconds (%d/5)...\n", retry);
      etimer_set(&init_timer, CLOCK_SECOND * 10);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));
    }

  } while (retry < 5 && !success);

  if (!success || strlen(light_ip) == 0 ) {
    LOG_ERR("Could not discover required services or IPs are empty\n");
    PROCESS_EXIT();
  }

  LOG_INFO("[LightSystemAct] Starting observation of discovered services\n");

  obs_light = coap_obs_request_registration(&light_ep, "light", light_notification_handler, NULL);
  
  if (!obs_light) {
    LOG_ERR("Failed to set up observations. Exiting process.\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Observations set up successfully.\n");
  
  LOG_INFO("Light Actuator ready\n");
  LOG_INFO("LED resource activated at /led\n");
  LOG_INFO("Waiting for CoAP requests...\n");

  while (1) {
    PROCESS_YIELD();
    
    // Log per monitorare eventi generici
    if (ev != PROCESS_EVENT_CONTINUE) {
      LOG_INFO("Event received: %d\n", ev);
    }
  }

  PROCESS_END();
}
