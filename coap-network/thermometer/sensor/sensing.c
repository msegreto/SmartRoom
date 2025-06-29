// sensor/sensing.c
#include "sensing.h"

static int sensor_active = 0;

float generate_random_temperature() {
    float u1 = (float)random_rand() / RANDOM_RAND_MAX;
    float u2 = (float)random_rand() / RANDOM_RAND_MAX;
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(2 * M_PI * u2);
    return MEAN_TEMPERATURE + z0 * STD_TEMPERATURE;
}

void sensor_on() { sensor_active = 1; }
void sensor_off() { sensor_active = 0; }
int sensor_is_active() { return sensor_active; }