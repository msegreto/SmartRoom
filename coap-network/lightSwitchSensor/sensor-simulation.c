// sensor-simulation.c
#include "light-sensor.h"
#include "lib/random.h"
#include <stdlib.h>

bool simulate_motion_detection(void)
{
  return (random_rand() % 2) == 1;
}
