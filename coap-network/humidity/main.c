#include "contiki.h"
#include "config.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "sys/log.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "../cJSON-master/cJSON.h"
#include "os/dev/button-hal.h"
#include "leds.h"
#include "coap/res_latest.h"
#include "coap/res_prediction.h"
#include "sensor/sensing.h"
#include "sensor/buffer.h"
#include "sensor/prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_MODULE "SmartHumidity"
#define LOG_LEVEL LOG_LEVEL_APP

PROCESS(humidity_process, "Smart Humidity");
AUTOSTART_PROCESSES(&humidity_process);

extern coap_resource_t res_latest;
extern coap_resource_t res_prediction;
extern coap_resource_t res_on;
extern coap_resource_t res_off;

//void trigger_prediction_event();
//void trigger_latest_event();

static int registered = 0;

static void client_chunk_handler(coap_message_t *response) {
  LOG_INFO("[Humidity] === RESPONSE HANDLER CALLED ===\n");
  
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("[Humidity] Registration timed out - no response received\n");
    return;
  }
  
  LOG_INFO("[Humidity] Response received! Code: %d\n", response->code);
  
  int len = coap_get_payload(response, &chunk);
  if (len <= 0 || chunk == NULL) {
    LOG_WARN("[Humidity] Empty or invalid payload received (len=%d)\n", len);
  } else {
    char payload[len + 1];
    memcpy(payload, chunk, len);
    payload[len] = '\0';
    LOG_INFO("[Humidity] Response payload: '%s'\n", payload);
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("[Humidity] Registration successful!\n");
  } else {
    LOG_WARN("[Humidity] Registration failed with code: %d\n", response->code);
  }
  
  LOG_INFO("[Humidity] === RESPONSE HANDLER END ===\n");
}

PROCESS_THREAD(humidity_process, ev, data) {
  static struct etimer timer;
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];
  static int retry;
  static int pressed = 0;

  PROCESS_BEGIN();
  coap_engine_init();

  // Waiting for button press to start registration
  while(1) {
    PROCESS_YIELD();
    if(ev == button_hal_press_event || pressed == 1) {
      pressed = 1;
      break;
    }
  }
  LOG_INFO("[Humidity] Starting registration\n");
  leds_on(LEDS_RED);

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  registered = 0;
  retry = 0;

  while(retry < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    if(root == NULL) {
      LOG_ERR("[Humidity] Failed to create JSON object\n");
      PROCESS_EXIT();
    }

    cJSON_AddStringToObject(root, "s", "humid");
    cJSON *resources = cJSON_CreateArray();
    if(resources == NULL) {
      LOG_ERR("[Humidity] Failed to create JSON array\n");
      cJSON_Delete(root);
      PROCESS_EXIT();
    }
    cJSON_AddItemToArray(resources, cJSON_CreateString("hum"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("predh"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("onh"));
    cJSON_AddItemToArray(resources, cJSON_CreateString("offh"));
    cJSON_AddItemToObject(root, "ss", resources);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(payload == NULL) {
      LOG_ERR("[Humidity] Failed to create payload\n");
      PROCESS_EXIT();
    }

    LOG_INFO("[Humidity] JSON payload (len=%zu): %s\n", strlen(payload), payload);
    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("[Humidity] Sending registration request...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);
    
    if(!registered) {
      retry++;
      etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }

  if(!registered) {
    LOG_WARN("[Humidity] Max registration attempts reached\n");
    PROCESS_EXIT();
  }

  coap_activate_resource(&res_latest, "hum");
  coap_activate_resource(&res_prediction, "predh");
  coap_activate_resource(&res_on, "onh");
  coap_activate_resource(&res_off, "offh");

  LOG_INFO("Registration successful!\n");
  etimer_set(&timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
  leds_off(LEDS_RED);
  leds_on(LEDS_GREEN);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    leds_off(LEDS_GREEN);

    float hum = generate_random_humidity(); // <-- implementa se non esiste
    LOG_INFO("Generated humidity: %.2f\n", hum);
    update_buffer(hum);
    trigger_latest_event(hum);

    if(buffer_is_full()) {
      LOG_INFO("Buffer is full, triggering prediction event.\n");
      trigger_prediction_event();
    }   

    etimer_reset(&timer);
    leds_on(LEDS_GREEN);
  }

  PROCESS_END();
}


