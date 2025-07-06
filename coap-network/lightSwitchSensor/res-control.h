#ifndef RES_CONTROL_H
#define RES_CONTROL_H

#include "contiki.h"
#include "coap-engine.h"

// Resource declarations
extern coap_resource_t res_on;
extern coap_resource_t res_off;

extern coap_resource_t res_light;

// Sensor control functions
void sensor_on(void);
void sensor_off(void);

#endif /* RES_CONTROL_H */

