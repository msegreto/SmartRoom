#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "dev/button-hal.h"
#include "leds.h"
#include "random.h"
#include "../cJSON-master/cJSON.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#define LOG_MODULE "LightSensor"
#define LOG_LEVEL LOG_LEVEL_INFO

extern coap_resource_t res_light;

int light_state = 0;
static int led_state = 0;  // 0 = OFF, 1 = ON

static int registered = 0;
static int registration_retry_count = 0;
static struct etimer debounce_timer;
static struct etimer wait_timer;

// Callback per la risposta del comando LED
static void led_command_response_handler(coap_message_t *response) {
  LOG_INFO("=== LED COMMAND RESPONSE RECEIVED ===\n");
  
  if (response == NULL) {
    LOG_ERR("LED command timeout\n");
    return;
  }
  
  LOG_INFO("LED command response code: %d\n", response->code);
  
  if (response->code == CHANGED_2_04) {
    LOG_INFO("LED command successful\n");
  } else {
    LOG_WARN("LED command failed\n");
  }
}

static void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;
  if (response == NULL) {
    LOG_ERR("Registration timed out\n");
    return;
  }
  int len = coap_get_payload(response, &chunk);
  char payload[len + 1];
  memcpy(payload, chunk, len);
  payload[len] = '\0';

  LOG_INFO("Response: %i\n", response->code);
  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("Registration successful\n");
  } else {
    LOG_WARN("Registration failed\n");
  }
}


PROCESS(light_sensor_main_process, "Light Sensor Main Process");
AUTOSTART_PROCESSES(&light_sensor_main_process);


PROCESS_THREAD(light_sensor_main_process, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_endpoint_t actuator_ep;
  static coap_message_t request[1];
  static coap_message_t led_request[1];

  static struct etimer registration_timer;

  PROCESS_BEGIN();

  coap_engine_init();

  // Wait for network 
  LOG_INFO("[LightSensor] Waiting for network establishment...\n");
  etimer_set(&registration_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&registration_timer));
  LOG_INFO("[LightSensor] Network wait complete, starting registration\n");

  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &server_ep);
  
  // Parse actuator endpoint
  coap_endpoint_parse(ACTUATOR_EP, strlen(ACTUATOR_EP), &actuator_ep);
  LOG_INFO("[LightSensor] Actuator endpoint configured\n");

  // Wait for registration
  while (registration_retry_count < MAX_REGISTRATION_RETRY && registered == 0) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *string_array = cJSON_CreateArray();
    cJSON_AddItemToArray(string_array, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", string_array);
    cJSON_AddNumberToObject(root, "t", SENSOR_SAMPLE_INTERVAL);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    coap_set_payload(request, (uint8_t *)payload, strlen(payload));
    LOG_INFO("Sending registration...\n");

    COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

    if (!registered) {
      registration_retry_count++;
      etimer_set(&wait_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    }
  }

  if (!registered) {
    LOG_ERR("Max registration attempts reached. Exiting.\n");
    PROCESS_EXIT();
  }
  // Attendi un po' prima di attivare la risorsa
  static struct etimer resource_timer;
  etimer_set(&resource_timer, CLOCK_SECOND / 2);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&resource_timer));
  coap_activate_resource(&res_light, "light");

  while (1) {
    PROCESS_YIELD();
    if (ev == button_hal_press_event) {
      LOG_INFO("=== BUTTON PRESS DETECTED ===\n");

      LOG_INFO("Button pressed: toggling LED and generating light_state\n");

      /*
      leds_on(LEDS_RED);
      
      int old_light_state = light_state;
      light_state = random_rand() % 100;
      
      LOG_INFO("Light state changed: %d -> %d\n", old_light_state, light_state);
      LOG_INFO("Triggering CoAP resource notification...\n");
      
      res_light.trigger();
      
      // Invia comando all'attuatore
      LOG_INFO("=== SENDING LED COMMAND TO ACTUATOR ===\n");
      
      // Toggle LED state
      led_state = !led_state;
      
      */
      coap_init_message(led_request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(led_request, "/led");
      LOG_INFO("Sending LED command to IP: %s\n", ACTUATOR_EP);
      
      etimer_set(&debounce_timer, CLOCK_SECOND / 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));
      // Payload: "1" per ON, "0" per OFF
      char led_payload[2];
      snprintf(led_payload, sizeof(led_payload), "%d", led_state);
      coap_set_payload(led_request, (uint8_t *)led_payload, strlen(led_payload));
      
      LOG_INFO("Sending LED command: %s (state: %s)\n", led_payload, led_state ? "ON" : "OFF");
      
      COAP_BLOCKING_REQUEST(&actuator_ep, led_request, led_command_response_handler);
      
      LOG_INFO("=== NOTIFICATION SENT ===\n");
    }
  }

  PROCESS_END();
}
