#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "sys/etimer.h"
#include "net/linkaddr.h"
#include "../cJSON-master/cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_MODULE "CloudServer"
#define LOG_LEVEL LOG_LEVEL_INFO


// Max number of devices that can register
#define MAX_DEVICES 10

typedef struct {
  char id[32];  // type of device: e.g., light_sensor, led_actuator
  char services[7][32];
  int num_services;
  int interval;
} device_entry_t;

device_entry_t devices[MAX_DEVICES];
static int device_count = 0;

static void res_post_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static void res_temp_config_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static void res_hum_config_handler(coap_message_t *request, coap_message_t *response,
                                  uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

RESOURCE(res_registration_sensor,
         "title=\"Registration Resource\";ct=0",
         NULL,
         res_post_handler,
         NULL,
         NULL);

RESOURCE(res_temp_config,
         "title=\"Temperature Config\";rt=\"config\";ct=0",
         res_temp_config_handler,
         NULL,
         NULL,
         NULL);

RESOURCE(res_hum_config,
         "title=\"Humidity Config\";rt=\"config\";ct=0",
         res_hum_config_handler,
         NULL,
         NULL,
         NULL);

static void res_post_handler(coap_message_t *request, coap_message_t *response,
                             uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  LOG_INFO("[Cloud] ======== POST HANDLER INVOKED ========\n");
  LOG_INFO("[Cloud] Request received at %lu\n", clock_time());
  LOG_INFO("[Cloud] Request pointer: %p\n", (void*)request);
  LOG_INFO("[Cloud] Response pointer: %p\n", (void*)response);
  
  // Log delle informazioni di rete del mittente
  LOG_INFO("[Cloud] Request URI: %.*s\n", (int)request->uri_path_len, request->uri_path);
  LOG_INFO("[Cloud] Request method: %d\n", request->code);
  LOG_INFO("[Cloud] Content format: %d\n", request->content_format);

  size_t len = 0;
  const uint8_t *payload = NULL;
  len = coap_get_payload(request, &payload);

  LOG_INFO("[Cloud] Payload length: %zu\n", len);
  LOG_INFO("[Cloud] Payload pointer: %p\n", (void*)payload);

  if (len == 0 || payload == NULL) {
    LOG_ERR("[Cloud] Empty or NULL payload\n");
    coap_set_status_code(response, BAD_REQUEST_4_00);
    LOG_INFO("[Cloud] === POST HANDLER END (BAD REQUEST) ===\n");
    return;
  }

  // Log del payload raw prima di copiarlo
  LOG_INFO("[Cloud] Raw payload first 10 bytes: ");
  for(size_t i = 0; i < len && i < 10; i++) {
    printf("%02x ", payload[i]);
  }
  printf("\n");

  // Crea una copia null-terminated del payload per sicurezza
  LOG_INFO("[Cloud] Allocating %zu bytes for payload copy\n", len + 1);
  char *payload_str = malloc(len + 1);
  if (!payload_str) {
    LOG_ERR("[Cloud] Memory allocation failed for %zu bytes\n", len + 1);
    coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
    LOG_INFO("[Cloud] === POST HANDLER END (MALLOC FAIL) ===\n");
    return;
  }
  LOG_INFO("[Cloud] Allocated payload_str at %p\n", (void*)payload_str);
  
  memcpy(payload_str, payload, len);
  payload_str[len] = '\0';
  
  LOG_INFO("[Cloud] Received payload (len=%zu): '%s'\n", len, payload_str);
  LOG_INFO("[Cloud] About to parse JSON...\n");

  cJSON *root = cJSON_Parse(payload_str);
  LOG_INFO("[Cloud] JSON parse result: %p\n", (void*)root);
  
  LOG_INFO("[Cloud] Freeing payload_str at %p\n", (void*)payload_str);
  free(payload_str);
  LOG_INFO("[Cloud] payload_str freed successfully\n");
  
  if (!root) {
    const char *error_ptr = cJSON_GetErrorPtr();
    LOG_ERR("[Cloud] Failed to parse JSON (len=%zu): %s\n", len, error_ptr ? error_ptr : "unknown error");
    coap_set_status_code(response, BAD_REQUEST_4_00);
    LOG_INFO("[Cloud] === POST HANDLER END (JSON PARSE FAIL) ===\n");
    return;
  }

  LOG_INFO("[Cloud] JSON parsed successfully\n");

  if (device_count >= MAX_DEVICES) {
    LOG_WARN("[Cloud] Device registry full (current count: %d)\n", device_count);
    LOG_INFO("[Cloud] About to delete JSON root...\n");
    cJSON_Delete(root);
    LOG_INFO("[Cloud] JSON root deleted\n");
    coap_set_status_code(response, SERVICE_UNAVAILABLE_5_03);
    LOG_INFO("[Cloud] === POST HANDLER END (REGISTRY FULL) ===\n");
    return;
  }

  LOG_INFO("[Cloud] Current device count: %d\n", device_count);

  const cJSON *s = cJSON_GetObjectItem(root, "s");
  const cJSON *ss = cJSON_GetObjectItem(root, "ss");
  const cJSON *t = cJSON_GetObjectItem(root, "t");

  LOG_INFO("[Cloud] JSON parsing - s: %p, ss: %p, t: %p\n", (void*)s, (void*)ss, (void*)t);

  if (s == NULL || ss == NULL || t == NULL) {
    LOG_ERR("[Cloud] Missing required JSON fields\n");
    LOG_INFO("[Cloud] About to delete JSON root (missing fields)...\n");
    cJSON_Delete(root);
    LOG_INFO("[Cloud] JSON root deleted (missing fields)\n");
    coap_set_status_code(response, BAD_REQUEST_4_00);
    LOG_INFO("[Cloud] === POST HANDLER END (MISSING FIELDS) ===\n");
    return;
  }

  if (!cJSON_IsString(s) || !cJSON_IsArray(ss) || !cJSON_IsNumber(t)) {
    LOG_ERR("[Cloud] Invalid JSON structure - s:%s ss:%s t:%s\n",
            cJSON_IsString(s) ? "OK" : "FAIL",
            cJSON_IsArray(ss) ? "OK" : "FAIL",
            cJSON_IsNumber(t) ? "OK" : "FAIL");
    LOG_INFO("[Cloud] About to delete JSON root (invalid structure)...\n");
    cJSON_Delete(root);
    LOG_INFO("[Cloud] JSON root deleted (invalid structure)\n");
    coap_set_status_code(response, BAD_REQUEST_4_00);
    LOG_INFO("[Cloud] === POST HANDLER END (INVALID STRUCTURE) ===\n");
    return;
  }

  // Controlla se il dispositivo è già registrato
  const char *new_device_id = s->valuestring;
  for (int i = 0; i < device_count; i++) {
    if (strcmp(devices[i].id, new_device_id) == 0) {
      LOG_WARN("[Cloud] Device '%s' already registered at index %d\n", new_device_id, i);
      cJSON_Delete(root);
      coap_set_status_code(response, CONTENT_2_05); // Already exists, but OK
      const char *response_payload = "already_registered";
      coap_set_payload(response, (uint8_t *)response_payload, strlen(response_payload));
      coap_set_header_content_format(response, TEXT_PLAIN);
      LOG_INFO("[Cloud] === POST HANDLER END (ALREADY REGISTERED) ===\n");
      return;
    }
  }

  LOG_INFO("[Cloud] Accessing device entry at index %d\n", device_count);
  device_entry_t *entry = &devices[device_count];
  
  // Controllo sicurezza per evitare buffer overflow
  if (s->valuestring == NULL) {
    LOG_ERR("[Cloud] Device ID string is NULL\n");
    LOG_INFO("[Cloud] About to delete JSON root (null string)...\n");
    cJSON_Delete(root);
    LOG_INFO("[Cloud] JSON root deleted (null string)\n");
    coap_set_status_code(response, BAD_REQUEST_4_00);
    LOG_INFO("[Cloud] === POST HANDLER END (NULL STRING) ===\n");
    return;
  }
  
  size_t id_len = strlen(s->valuestring);
  LOG_INFO("[Cloud] Device ID length: %zu, max allowed: %zu\n", id_len, sizeof(entry->id) - 1);
  if (id_len >= sizeof(entry->id)) {
    LOG_ERR("[Cloud] Device ID too long: %zu >= %zu\n", id_len, sizeof(entry->id));
    LOG_INFO("[Cloud] About to delete JSON root (id too long)...\n");
    cJSON_Delete(root);
    LOG_INFO("[Cloud] JSON root deleted (id too long)\n");
    coap_set_status_code(response, BAD_REQUEST_4_00);
    LOG_INFO("[Cloud] === POST HANDLER END (ID TOO LONG) ===\n");
    return;
  }
  
  LOG_INFO("[Cloud] Copying device ID: %s\n", s->valuestring);
  strncpy(entry->id, s->valuestring, sizeof(entry->id) - 1);
  entry->id[sizeof(entry->id) - 1] = '\0';
  entry->num_services = 0;

  LOG_INFO("[Cloud] Processing device ID: %s\n", entry->id);

  cJSON *item = NULL;
  int service_index = 0;
  LOG_INFO("[Cloud] Starting to process services array\n");
  cJSON_ArrayForEach(item, ss) {
    LOG_INFO("[Cloud] Processing service %d, item: %p\n", service_index, (void*)item);
    if (cJSON_IsString(item) && service_index < 7) {
      if (item->valuestring == NULL) {
        LOG_WARN("[Cloud] Service string is NULL, skipping\n");
        continue;
      }
      size_t service_len = strlen(item->valuestring);
      LOG_INFO("[Cloud] Service %d length: %zu, value: %s\n", service_index, service_len, item->valuestring);
      if (service_len >= 32) {
        LOG_WARN("[Cloud] Service name too long, truncating: %s\n", item->valuestring);
      }
      strncpy(entry->services[service_index], item->valuestring, 31);
      entry->services[service_index][31] = '\0';
      LOG_INFO("[Cloud] Added service[%d]: %s\n", service_index, entry->services[service_index]);
      service_index++;
    } else if (!cJSON_IsString(item)) {
      LOG_WARN("[Cloud] Skipping non-string service item\n");
    } else {
      LOG_WARN("[Cloud] Too many services, maximum is 7\n");
      break;
    }
  }
  entry->num_services = service_index;

  entry->interval = t->valueint;
  
  LOG_INFO("[Cloud] Device interval: %d\n", entry->interval);
  
  device_count++;
  LOG_INFO("[Cloud] About to delete JSON root (success path)...\n");
  cJSON_Delete(root);
  LOG_INFO("[Cloud] JSON root deleted (success path)\n");

  LOG_INFO("[Cloud] Successfully registered device: %s with %d service(s), interval %d (total devices: %d)\n",
           entry->id, entry->num_services, entry->interval, device_count);

  // Log di tutti i servizi registrati
  for (int i = 0; i < entry->num_services; i++) {
    LOG_INFO("[Cloud] Service %d: %s\n", i, entry->services[i]);
  }

  coap_set_status_code(response, CREATED_2_01);
  
  // Aggiungi un payload di risposta per confermare la registrazione
  const char *response_payload = "registered";
  coap_set_payload(response, (uint8_t *)response_payload, strlen(response_payload));
  coap_set_header_content_format(response, TEXT_PLAIN);
  
  LOG_INFO("[Cloud] === POST HANDLER END (SUCCESS) ===\n");
}

static void res_temp_config_handler(coap_message_t *request, coap_message_t *response,
                                   uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  LOG_INFO("[Cloud] === TEMP CONFIG HANDLER INVOKED ===\n");
  LOG_INFO("[Cloud] Request method: %d\n", request->code);
  LOG_INFO("[Cloud] Request URI: %.*s\n", (int)request->uri_path_len, request->uri_path);
  LOG_INFO("[Cloud] Buffer size: %d\n", preferred_size);
  
  const char *temp_uri = "coap://[fd00::100]:5683/predictionTemp";
  int len = strlen(temp_uri);
  
  LOG_INFO("[Cloud] Temp URI length: %d, buffer size: %d\n", len, preferred_size);
  
  if (len > preferred_size) {
    LOG_ERR("[Cloud] Temp URI too large: %d > %d\n", len, preferred_size);
    coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
    return;
  }

  LOG_INFO("[Cloud] Copying URI to buffer...\n");
  memcpy(buffer, temp_uri, len);
  coap_set_payload(response, buffer, len);
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_status_code(response, CONTENT_2_05);
  
  LOG_INFO("[Cloud] Sent temp URI: %s\n", temp_uri);
  LOG_INFO("[Cloud] === TEMP CONFIG HANDLER COMPLETED ===\n");
}

static void res_hum_config_handler(coap_message_t *request, coap_message_t *response,
                                  uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
  LOG_INFO("[Cloud] === HUM CONFIG HANDLER INVOKED ===\n");
  LOG_INFO("[Cloud] Request method: %d\n", request->code);
  LOG_INFO("[Cloud] Request URI: %.*s\n", (int)request->uri_path_len, request->uri_path);
  LOG_INFO("[Cloud] Buffer size: %d\n", preferred_size);
  
  const char *hum_uri = "coap://[fd00::101]:5683/predictionHum";
  int len = strlen(hum_uri);
  
  LOG_INFO("[Cloud] Hum URI length: %d, buffer size: %d\n", len, preferred_size);
  
  if (len > preferred_size) {
    LOG_ERR("[Cloud] Hum URI too large: %d > %d\n", len, preferred_size);
    coap_set_status_code(response, INTERNAL_SERVER_ERROR_5_00);
    return;
  }

  LOG_INFO("[Cloud] Copying URI to buffer...\n");
  memcpy(buffer, hum_uri, len);
  coap_set_payload(response, buffer, len);
  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_status_code(response, CONTENT_2_05);
  
  LOG_INFO("[Cloud] Sent hum URI: %s\n", hum_uri);
  LOG_INFO("[Cloud] === HUM CONFIG HANDLER COMPLETED ===\n");
}

PROCESS(cloud_server_process, "Cloud Server Process");
// PROCESS(monitor_process, "Monitor Process"); // Disabilitato temporaneamente
AUTOSTART_PROCESSES(&cloud_server_process);

PROCESS_THREAD(cloud_server_process, ev, data) {
  static struct etimer heartbeat_timer;
  static struct etimer network_check_timer;
  
  PROCESS_BEGIN();

  LOG_INFO("[Cloud] === CLOUD SERVER STARTING ===\n");
  LOG_INFO("[Cloud] Node ID: %d\n", linkaddr_node_addr.u8[0]);
  
  // Wait a bit for network initialization
  LOG_INFO("[Cloud] Waiting for network initialization...\n");
  etimer_set(&network_check_timer, CLOCK_SECOND * 5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&network_check_timer));
  
  LOG_INFO("[Cloud] Network initialization complete.\n");
  LOG_INFO("[Cloud] Initializing CoAP engine...\n");
  coap_engine_init();
  
  LOG_INFO("[Cloud] Activating registration resource...\n");
  coap_activate_resource(&res_registration_sensor, "registration");
  
  LOG_INFO("[Cloud] Activating temp config resource...\n");
  coap_activate_resource(&res_temp_config, "tempConfig");
  
  LOG_INFO("[Cloud] Activating hum config resource...\n");
  coap_activate_resource(&res_hum_config, "humConfig");

  LOG_INFO("[Cloud] Server started successfully. Waiting for registrations...\n");
  LOG_INFO("[Cloud] Max devices: %d\n", MAX_DEVICES);
  LOG_INFO("[Cloud] Listening on resources:\n");
  LOG_INFO("[Cloud]   - /registration\n");
  LOG_INFO("[Cloud]   - /tempConfig\n");
  LOG_INFO("[Cloud]   - /humConfig\n");
  LOG_INFO("[Cloud] Full URIs:\n");
  LOG_INFO("[Cloud]   - coap://[fe80::202:2:2:2]/registration\n");
  LOG_INFO("[Cloud]   - coap://[fe80::202:2:2:2]/tempConfig\n");
  LOG_INFO("[Cloud]   - coap://[fe80::202:2:2:2]/humConfig\n");
  
  // Heartbeat ogni 30 secondi
  etimer_set(&heartbeat_timer, CLOCK_SECOND * 30);

  while(1) {
    PROCESS_WAIT_EVENT();
    
    if(ev == PROCESS_EVENT_TIMER && data == &heartbeat_timer) {
      LOG_INFO("[Cloud] === HEARTBEAT === Devices registered: %d/%d\n", device_count, MAX_DEVICES);
      LOG_INFO("[Cloud] Server is running and listening for registrations\n");
      etimer_reset(&heartbeat_timer);
    } else if(ev == PROCESS_EVENT_INIT) {
      LOG_INFO("[Cloud] Process initialized\n");
    }
  }

  PROCESS_END();
}

/*
PROCESS_THREAD(monitor_process, ev, data) {
  static struct etimer monitor_timer;
  
  PROCESS_BEGIN();
  
  etimer_set(&monitor_timer, CLOCK_SECOND * 60); // Log ogni 60 secondi invece di 30
  
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&monitor_timer));
    
    LOG_INFO("[Monitor] Server status - Registered devices: %d/%d\n", device_count, MAX_DEVICES);
    
    for(int i = 0; i < device_count; i++) {
      LOG_INFO("[Monitor] Device %d: %s (%d services, interval: %d)\n", 
               i, devices[i].id, devices[i].num_services, devices[i].interval);
    }
    
    etimer_reset(&monitor_timer);
  }
  
  PROCESS_END();
}
*/