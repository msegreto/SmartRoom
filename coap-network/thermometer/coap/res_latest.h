#ifndef RES_LATEST_H
#define RES_LATEST_H

#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
#include "buffer.h"

extern coap_resource_t res_latest;
void trigger_latest_event(void);

#endif /* RES_LATEST_H */
