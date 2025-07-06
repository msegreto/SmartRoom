#ifndef RES_CONTROL_H
#define RES_CONTROL_H

#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>

// Resource declarations for sensor control (on/off)
extern coap_resource_t res_on;
extern coap_resource_t res_off;

// Sensor control functions
void sensor_on(void);
void sensor_off(void);

#endif /* RES_CONTROL_H */
