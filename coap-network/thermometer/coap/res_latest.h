#ifndef RES_LATEST_H
#define RES_LATEST_H

#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
#include "prediction.h"

extern coap_resource_t res_latest;
void trigger_latest_event(float value) ;

#endif /* RES_LATEST_H */
