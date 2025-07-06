#include "sensing.h"

static float current_temperature = MEAN_TEMPERATURE;
static TemperatureTrend current_trend = TREND_NONE;

void set_temperature_trend(TemperatureTrend trend) {
    current_trend = trend;
}

float generate_random_temperature() {
    // Determina la direzione in base al trend
    int direction;
    switch(current_trend) {
        case TREND_HEATING:
            direction = (random_rand() % 10 < 7) ? 1 : -1; // 70% in salita
            break;
        case TREND_COOLING:
            direction = (random_rand() % 10 < 7) ? -1 : 1; // 70% in discesa
            break;
        case TREND_NONE:
        default:
            direction = (random_rand() % 2 == 0) ? 1 : -1; // 50/50
            break;
    }

    // Step casuale tra 0.0 e MAX_STEP
    float step = ((float)random_rand() / 32767.0f) * MAX_STEP;

    // Calcola nuova temperatura
    float new_temp = current_temperature + direction * step;

    // Limita la temperatura all'intervallo [media ± 3σ]
    float min_temp = MEAN_TEMPERATURE - 3.0f * STD_TEMPERATURE;
    float max_temp = MEAN_TEMPERATURE + 3.0f * STD_TEMPERATURE;

    if(new_temp < min_temp) new_temp = min_temp;
    if(new_temp > max_temp) new_temp = max_temp;

    current_temperature = new_temp;
    return current_temperature;
}
