#ifndef RES_LIGHT_H
#define RES_LIGHT_H

#include "coap-engine.h"
#include <string.h>
#include <stdio.h>

extern coap_resource_t res_light;
extern int light_state;

void res_light_trigger(void);

#endif
