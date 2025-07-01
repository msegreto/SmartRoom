#include "contiki.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "sys/log.h"
#include "../cJSON-master/cJSON.h"

#define LOG_MODULE "LightClient"
#define LOG_LEVEL LOG_LEVEL_APP

#define SERVER_EP "coap://[fd00::1]"

// === Variabili globali ===
static coap_endpoint_t cloud_ep;
static bool sensor_found = false;
static char sensor_ip[64] = {0};

// === Handlers ===
static void client_chunk_handler(coap_message_t *response) {
  if (!response) {
    LOG_ERR("Timeout in risposta CoAP\n");
    return;
  }

  const uint8_t *chunk;
  int len = coap_get_payload(response, &chunk);

  char json_buf[128];
  memcpy(json_buf, chunk, len);
  json_buf[len] = '\0';

  LOG_INFO("Risposta: %s\n", json_buf);

  cJSON *root = cJSON_Parse(json_buf);
  if (!root) {
    LOG_ERR("JSON non valido\n");
    return;
  }

  cJSON *ip = cJSON_GetObjectItem(root, "ip");
  if (cJSON_IsString(ip)) {
    strncpy(sensor_ip, ip->valuestring, sizeof(sensor_ip) - 1);
    sensor_found = true;
    LOG_INFO("Indirizzo trovato: %s\n", sensor_ip);
  } else {
    LOG_WARN("Campo IP mancante\n");
  }

  cJSON_Delete(root);
}

// === Protothread di lookup ===
static PT_THREAD(lookup_sensor_address(struct pt *pt)) {
  static struct coap_blocking_request_state blocking_state;
  static coap_message_t request[1];

  PT_BEGIN(pt);

  coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(request, "/lookup");
  coap_set_header_uri_query(request, "s=light_sensor");

  PT_WAIT_THREAD(pt, coap_blocking_request(
    &blocking_state,
    PROCESS_EVENT_NONE,
    &cloud_ep,
    request,
    client_chunk_handler
  ));

  PT_END(pt);
}

// === Processo principale ===
PROCESS(main_process, "Light Sensor Client");
AUTOSTART_PROCESSES(&main_process);

PROCESS_THREAD(main_process, ev, data) {
  static struct pt lookup_pt;

  PROCESS_BEGIN();

  LOG_INFO("Avvio del processo Light Sensor\n");

  coap_engine_init();
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &cloud_ep);
  PT_INIT(&lookup_pt);

  PT_INIT(&lookup_pt);
  while(PT_SCHEDULE(lookup_sensor_address(&lookup_pt))) {
    PROCESS_PAUSE();
  }


  if (!sensor_found) {
    LOG_ERR("Sensore non trovato. Terminazione.\n");
    PROCESS_EXIT();
  }

  LOG_INFO("Setup completato. Pronto per interagire con il sensore.\n");

  while (1) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
