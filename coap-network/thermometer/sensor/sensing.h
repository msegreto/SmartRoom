#ifndef SENSING_H
#define SENSING_H

#include "contiki.h"
#include "random.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>

// Tipologia di andamento della temperatura
typedef enum {
    TREND_NONE,
    TREND_COOLING,
    TREND_HEATING
} TemperatureTrend;

// Imposta la direzione del trend
void set_temperature_trend(TemperatureTrend trend);

// Genera una temperatura casuale con media e deviazione standard configurabili
float generate_random_temperature(void);

#endif /* SENSING_H */
