// sensor/sensing.c
#include "sensing.h"

static int sensor_active = 0;

static float current_temperature = MEAN_TEMPERATURE;

float generate_random_temperature() {
    // Direzione casuale: -1 o +1
    int direction = (random_rand() % 2 == 0) ? 1 : -1;

    // Step casuale tra 0.0 e MAX_STEP
    float step = ((float)random_rand() / 32767.0f) * MAX_STEP;

    // Applica lo step alla temperatura corrente
    float new_temp = current_temperature + direction * step;

    // Limita all'intervallo consentito
    float min_temp = MEAN_TEMPERATURE - 3.0f * STD_TEMPERATURE;
    float max_temp = MEAN_TEMPERATURE + 3.0f * STD_TEMPERATURE;

    if(new_temp < min_temp) new_temp = min_temp;
    if(new_temp > max_temp) new_temp = max_temp;

    current_temperature = new_temp;
    return current_temperature;
}

void sensor_on() { sensor_active = 1; }
void sensor_off() { sensor_active = 0; }
int sensor_is_active() { return sensor_active; }