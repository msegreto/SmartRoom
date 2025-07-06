#ifndef RES_CONTROL_H
#define RES_CONTROL_H

#include "coap-engine.h"

extern coap_resource_t res_set_threshold;
extern coap_resource_t res_get_threshold;
extern coap_resource_t res_status;
extern coap_resource_t res_on;
extern coap_resource_t res_off;

extern coap_observee_t *obs_temp;
extern coap_observee_t *obs_hum;

void trigger_status_change(void);
void sensor_on(void);
void sensor_off(void);

#endif /* RES_CONTROL_H */
