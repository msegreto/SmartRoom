#ifndef RES_CONTROL_H
#define RES_CONTROL_H

#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
#include "sensing.h"


// Resource declarations for sensor control (on/off)
extern coap_resource_t res_on;
extern coap_resource_t res_off;

#endif /* RES_CONTROL_H */
