#include "sensing.h"

static float current_humidity = MEAN_HUMIDITY;

float generate_random_humidity() {
    // Direzione casuale: -1 o +1
    int direction = (random_rand() % 2 == 0) ? 1 : -1;

    // Step casuale tra 0.0 e MAX_STEP
    float step = ((float)random_rand() / 32767.0f) * MAX_STEP;

    // Applica lo step alla temperatura corrente
    float new_temp = current_humidity + direction * step;

    // Limita all'intervallo consentito
    float min_temp = MEAN_HUMIDITY - 3.0f * STD_HUMIDITY;
    float max_temp = MEAN_HUMIDITY + 3.0f * STD_HUMIDITY;

    if(new_temp < min_temp) new_temp = min_temp;
    if(new_temp > max_temp) new_temp = max_temp;

    current_humidity = new_temp;
    return current_humidity;
}
