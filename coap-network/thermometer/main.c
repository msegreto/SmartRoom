#include "contiki.h"
#include "config.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "coap-block1.h"
#include "../cJSON-master/cJSON.h"
#include "coap/res_latest.h"
#include "coap/res_prediction.h"
#include "sensor/sensing.h"
#include "sensor/buffer.h"
#include "sensor/prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "SmartThermometer"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(thermometer_process, "Smart Thermometer");
AUTOSTART_PROCESSES(&thermometer_process);

extern coap_resource_t res_latest;
extern coap_resource_t res_prediction;
extern coap_resource_t res_on;
extern coap_resource_t res_off;

static int registered = 0;
static char hac_ip[64];
static coap_endpoint_t hac_ep;
coap_observee_t *obs_hac = NULL;

static char hac_service_payload[128] = "";

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

void discovery_response_handler_hac(coap_message_t *response) {
  discovery_response_handler(response, hac_service_payload, sizeof(hac_service_payload));
}

void hac_response_handler(coap_message_t *response) {
  if (!response) {
    LOG_WARN("[HAC] No response received\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);
  if (len > 0) {
    char buffer[64];
    memcpy(buffer, chunk, len);
    buffer[len] = '\0';
    LOG_INFO("[HAC] Notification payload: %s\n", buffer);
    float value;
    if (sscanf(buffer, "%f", &value) == 1) {
      LOG_INFO("[HAC] Parsed value: %.2f°C\n", value);
      //HERE SOME LOGIC TO HANDLE THE TEMPERATURE VALUE
    } else {
      LOG_WARN("[HAC] Failed to parse float from: %s\n", buffer);
    }
  } else {
    LOG_WARN("[HAC] Empty payload\n");
  }
}

void hac_notification_handler(struct coap_observee_s *obs, void *notification, coap_notification_flag_t flag) {
  LOG_INFO("[HAC] Notification received\n");
  if (notification) hac_response_handler((coap_message_t *)notification);
}

static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("[Thermometer] === RESPONSE HANDLER CALLED ===\n");
  
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Thermometer] Registration timed out - no response received\n");
    return;
  }
  
  LOG_INFO("[Thermometer] Response received! Code: %d\n", response->code);
  
  int len = coap_get_payload(response, &chunk);
  if (len <= 0 || chunk == NULL) {
    LOG_WARN("[Thermometer] Empty or invalid payload received (len=%d)\n", len);
  } else {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[Thermometer] Response payload: '%s'\n", payload);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Thermometer] Registration successful!\n");
  } else {
    LOG_WARN("[Thermometer] Registration failed with code: %d\n", response->code);
  }
  
  LOG_INFO("[Thermometer] === RESPONSE HANDLER END ===\n");
}

PROCESS_THREAD(thermometer_process, ev, data) {
  static struct etimer timer, init_timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;
  static int success;

  PROCESS_BEGIN();

  coap_engine_init();

  // Wait for network to be established before attempting registration
  LOG_INFO("[Thermometer] Waiting for network establishment...\n");
  etimer_set(&timer, CLOCK_SECOND * 10); // Wait 10 seconds for network
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  LOG_INFO("[Thermometer] Network wait complete, starting registration\n");

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  registered = 0;
  retry = 0;

  while(retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if(root == NULL) {
      LOG_ERR("[Thermometer] Failed to create JSON object\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "thermo");
    cJSON *resources = cJSON_CreateArray();
    if(resources == NULL) {
      LOG_ERR("[Thermometer] Failed to create JSON array\n");
      cJSON_Delete(root);
      PROCESS_EXIT();
    }
    cJSON_AddItemToArray(resources, cJSON_CreateString("temp"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("predt"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("ont"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("offt"));
    cJSON_AddItemToObject(root, "ss", resources);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(payload == NULL) {
      LOG_ERR("[Thermometer] Failed to create payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[Thermometer] JSON payload (len=%zu): %s\n", strlen(payload), payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[Thermometer] Sending registration request ...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    free(payload);

    if(!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if(!registered) {
    LOG_WARN("[Thermometer] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Registration successful!\n");

  // Attiva risorse e avvia sensing
  coap_activate_resource(&res_latest, "temp");
  coap_activate_resource(&res_prediction, "predt");
  coap_activate_resource(&res_on, "ont");
  coap_activate_resource(&res_off, "offt");

  LOG_INFO("[Thermometer] Starting service discovery for HACstatus\n");

  retry = 0;
  success = 0;

  do {
    static coap_endpoint_t disc_ep_hac;
    static coap_message_t disc_req_hac[1];
    success = 1;

    if (!coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &disc_ep_hac)) {
      LOG_ERR("[DISCOVERY] Failed to parse CLOUD_SERVER_EP for HAC status\n");
      success = 0;
    } else {
      coap_init_message(disc_req_hac, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(disc_req_hac, SERVICE_DISCOVERY_PATH);
      coap_set_header_uri_query(disc_req_hac, QUERY_HAC);
      LOG_INFO("[DISCOVERY] Sending GET to %s?%s\n", SERVICE_DISCOVERY_PATH, QUERY_HAC);
      COAP_BLOCKING_REQUEST(&disc_ep_hac, disc_req_hac, discovery_response_handler_hac);

      if (strlen(hac_service_payload) == 0) {
        LOG_WARN("[DISCOVERY] Empty or invalid response for HAC status\n");
        success = 0;
      } else if (!coap_endpoint_parse(hac_service_payload, strlen(hac_service_payload), &hac_ep)) {
        LOG_WARN("[DISCOVERY] Failed to parse endpoint for  HAC status\n");
        success = 0;
      } else {
        strncpy(hac_ip, hac_service_payload, sizeof(hac_ip) - 1);
        hac_ip[sizeof(hac_ip) - 1] = '\0';
        LOG_INFO("[DISCOVERY] Parsed hac IP: %s\n", hac_ip);
      }
    }

    if (!success) {
      retry++;
      LOG_WARN("[DISCOVERY] Failed. Retrying in 10 seconds (%d/5)...\n", retry);
      etimer_set(&init_timer, CLOCK_SECOND * 10);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&init_timer));
    }

  } while (retry < 5 && !success);

  if (!success || strlen(hac_ip) == 0 ) {
    LOG_ERR("Could not discover required services or IPs are empty\n");
    //NO PROCESS_EXIT() HERE BECAUSE IT CAN STILL WORK WITHOUT HAC
  }
  
  LOG_INFO("[LightSystemAct] Starting observation of discovered services\n");

  obs_hac = coap_obs_request_registration(&hac_ep, "status", hac_notification_handler, NULL);
  
  if (!obs_hac) {
    LOG_ERR("Failed to set up observations. Exiting process.\n");
    //NO PROCESS_EXIT() HERE BECAUSE IT CAN STILL WORK WITHOUT HAC
  }

  if (success && strlen(hac_ip) > 0 && obs_hac){
    LOG_INFO("Observations set up successfully.\n");
  }

  etimer_set(&timer, CLOCK_SECOND * SENSING_PERIOD_SECONDS);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    
    float temp = generate_random_temperature();
    LOG_INFO("Generated temperature: %.2f\n", temp);
    update_buffer(temp);
    trigger_latest_event(temp);

    if(buffer_is_full()) {
      LOG_INFO("Buffer is full, triggering prediction event.\n");
      trigger_prediction_event();
    }

    etimer_reset(&timer);
  }

  PROCESS_END();
}
