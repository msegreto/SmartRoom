#include "contiki.h"
#include "button-hal.h"
#include "coap-engine.h"
#include "light-sensor.h"
#include "dev/button-sensor.h"
#define BUTTON_SENSOR &button_sensor

extern coap_resource_t res_light;


#include <stdio.h>

PROCESS(button_process, "Button Handler");
extern struct process monitoring_process;

PROCESS_THREAD(button_process, ev, data)
{
  static struct etimer debounce_timer;
  PROCESS_BEGIN();

  button_hal_init();

  while(1) {
    PROCESS_YIELD();
    if(ev == sensors_event && data == BUTTON_SENSOR) {
      etimer_set(&debounce_timer, CLOCK_SECOND / 2);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));

      if(!light_state) {
        light_state = true;
        res_light.trigger();
        process_start(&monitoring_process, NULL);
      } else {
        light_state = false;
        res_light.trigger();
        process_exit(&monitoring_process);
      }
    }
  }
  PROCESS_END();
}
