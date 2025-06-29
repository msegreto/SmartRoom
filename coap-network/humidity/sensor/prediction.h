#ifndef PREDICTION_H
#define PREDICTION_H


#include "config.h"
#include "buffer.h"

void sensor_on(void);
void sensor_off(void);
int sensor_is_active(void);
float generate_random_humidity(void);
void update_buffer(float value);
int buffer_is_full(void);

#endif /* PREDICTION_H */
