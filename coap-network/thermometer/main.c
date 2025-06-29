// main.c
#include "contiki.h"
#include "config.h"
#include "lib/random.h"
#include "sys/etimer.h"
#include "coap-engine.h"
#include "coap/res_latest.h"
#include "coap/res_prediction.h"
#include "sensor/sensing.h"
#include "sensor/buffer.h"
#include "sensor/prediction.h"
#include <stdio.h>

PROCESS(thermometer_process, "Smart Thermometer");
AUTOSTART_PROCESSES(&thermometer_process);

extern coap_resource_t res_latest;
extern coap_resource_t res_prediction;
extern coap_resource_t res_on;
extern coap_resource_t res_off;

void trigger_prediction_event();
void trigger_latest_event();

PROCESS_THREAD(thermometer_process, ev, data) {
    static struct etimer timer;
    PROCESS_BEGIN();

    coap_engine_init();
    coap_activate_resource(&res_latest, "latest");
    coap_activate_resource(&res_prediction, "prediction");
    coap_activate_resource(&res_on, "sensor/on");
    coap_activate_resource(&res_off, "sensor/off");

    sensor_on();
    etimer_set(&timer, CLOCK_SECOND * SENSING_PERIOD_SECONDS);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

        if (sensor_is_active()) {
            float temp = generate_random_temperature();
            printf("Generated temperature: %.2f\n", temp);
            update_buffer(temp);
            trigger_latest_event();

            if (buffer_is_full()) {
                printf("Buffer is full, triggering prediction event.\n");
                trigger_prediction_event();
            }
        }

        etimer_reset(&timer);
    }

    PROCESS_END();
}
