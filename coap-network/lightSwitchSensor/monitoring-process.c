#include "contiki.h"
#include "coap-engine.h"
#include "light-sensor.h"
#include <stdio.h>
#include <stdlib.h>

PROCESS(monitoring_process, "Monitoring Process");

PROCESS_THREAD(monitoring_process, ev, data)
{
  static struct etimer motion_timer;
  PROCESS_BEGIN();

  etimer_set(&motion_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&motion_timer));

  if(!simulate_motion_detection()) {
    light_state = false;
    res_light.trigger();
  }

  PROCESS_END();
}