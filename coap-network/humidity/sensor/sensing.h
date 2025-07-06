#ifndef SENSING_H
#define SENSING_H

#include "contiki.h"
#include "random.h"
#include "config.h"
#include <stdlib.h>
#include <math.h>

// Genera una temperatura casuale con media e deviazione standard configurabili
float generate_random_humidity(void);

#endif /* SENSING_H */
