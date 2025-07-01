#include "contiki.h"
#include "sys/log.h"
#include "sys/etimer.h"
#include "../cJSON-master/cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_MODULE "LightSensorTest"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
PROCESS(light_sensor_test_process, "Light Sensor Memory Test Process");
AUTOSTART_PROCESSES(&light_sensor_test_process);
/*---------------------------------------------------------------------------*/

/* Test the exact JSON creation pattern from your original lightSwitchSensor */
static void test_light_sensor_json_pattern(void) {
  LOG_INFO("Testing Light Sensor JSON memory pattern...\n");
  
  for(int i = 0; i < 5; i++) {
    LOG_INFO("Test iteration %d\n", i);
    
    // Exact same pattern as in your original main.c
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "s", "light_sensor");
    cJSON *string_array = cJSON_CreateArray();
    cJSON_AddItemToArray(string_array, cJSON_CreateString("light"));
    cJSON_AddItemToObject(root, "ss", string_array);
    cJSON_AddNumberToObject(root, "t", 30);
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if(payload == NULL) {
      LOG_ERR("[LightSensor] Failed to create payload\n");
      return;
    }

    LOG_INFO("Iteration %d: JSON payload created (len: %zu): %s\n", i, strlen(payload), payload);
    free(payload);
  }
  
  LOG_INFO("Light Sensor JSON memory pattern test completed\n");
}

PROCESS_THREAD(light_sensor_test_process, ev, data)
{
  static struct etimer et;
  
  PROCESS_BEGIN();

  LOG_INFO("Starting Light Sensor Memory Test...\n");
  
  // Test the JSON memory pattern used in your original code
  test_light_sensor_json_pattern();
  
  // Keep process alive for a bit
  etimer_set(&et, CLOCK_SECOND * 5);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  
  LOG_INFO("Light Sensor Memory Test completed\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
