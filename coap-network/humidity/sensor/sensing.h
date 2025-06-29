#ifndef SENSING_H
#define SENSING_H

#include "contiki.h"
#include "random.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>

// Attiva il sensore
void sensor_on(void);

// Disattiva il sensore
void sensor_off(void);

// Restituisce 1 se il sensore è attivo, 0 altrimenti
int sensor_is_active(void);

// Genera una temperatura casuale con media e deviazione standard configurabili
float generate_random_humidity(void);

#endif /* SENSING_H */
