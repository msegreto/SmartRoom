#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "config.h"
#include "../cJSON-master/cJSON.h"

#define LOG_MODULE "LightActuator"
#define LOG_LEVEL LOG_LEVEL_INFO

static int registered = 0;
static int registration_retry_count = 0;
static struct etimer wait_timer;

static void registration_response_handler(coap_message_t *response) {
  if (response == NULL) {
    LOG_ERR("Registration timeout\n");
    return;
  }

  if (response->code == REGISTRATION_ACK_CODE) {
    registered = 1;
    LOG_INFO("Registration successful\n");
  } else {
    LOG_WARN("Registration failed: %d\n", response->code);
  }
}

static char* create_registration_payload(void) {
  cJSON *root = cJSON_CreateObject();
  if (!root) return NULL;

  cJSON_AddStringToObject(root, "s", "light_actuator");
  
  cJSON *services = cJSON_CreateArray();
  cJSON_AddItemToArray(services, cJSON_CreateString("led"));
  cJSON_AddItemToArray(services, cJSON_CreateString("on"));
  cJSON_AddItemToArray(services, cJSON_CreateString("off"));
  cJSON_AddItemToObject(root, "ss", services);
  
  cJSON_AddNumberToObject(root, "t", 60);
  
  char *payload = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  return payload;
}

PROCESS(light_actuator_process, "Light Actuator");
AUTOSTART_PROCESSES(&light_actuator_process);

PROCESS_THREAD(light_actuator_process, ev, data) {
  static coap_endpoint_t cloud_endpoint;
  static coap_message_t request[1];
  static struct etimer network_timer;

  PROCESS_BEGIN();

  // Initialization
  LOG_INFO("Light Actuator starting...\n");

  coap_engine_init();
  
  // Wait fot network
  etimer_set(&network_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&network_timer));
  
  coap_endpoint_parse(CLOUD_SERVER_EP, strlen(CLOUD_SERVER_EP), &cloud_endpoint);
  
  // Wait for registration
  while (registration_retry_count < MAX_REGISTRATION_RETRY && !registered) {
    coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
    coap_set_header_uri_path(request, "/" REGISTRATION_RESOURCE_PATH);
    
    char *payload = create_registration_payload();
    if (payload) {
      coap_set_payload(request, (uint8_t *)payload, strlen(payload));
      LOG_INFO("Registering with cloud...\n");
      
      COAP_BLOCKING_REQUEST(&cloud_endpoint, request, registration_response_handler);
      free(payload);
    }
    
    if (!registered) {
      registration_retry_count++;
      if (registration_retry_count < MAX_REGISTRATION_RETRY) {
        etimer_set(&wait_timer, CLOCK_SECOND * REGISTRATION_WAIT_SECONDS);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
      }
    }
  }

  if (!registered) {
    LOG_ERR("Registration failed after %d attempts\n", MAX_REGISTRATION_RETRY);
    PROCESS_EXIT();
  }

  LOG_INFO("Light Actuator ready\n");

  while (1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
