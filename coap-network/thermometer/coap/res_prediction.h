#ifndef RES_PREDICTION_H
#define RES_PREDICTION_H

#include "contiki.h"
#include "coap-engine.h"
#include <stdio.h>
#include "prediction.h"
#include "config.h"

extern coap_resource_t res_prediction;
void trigger_prediction_event(void);

#endif /* RES_PREDICTION_H */
