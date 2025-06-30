#ifndef LOGIC_H
#define LOGIC_H

typedef enum {
  WARMING_NONE,
  WARMING_COOLING,
  WARMING_HEATING
} warming_state_t;

void logic_set_temp(float t);
void logic_set_hum(float h);
void logic_check(void);
void logic_set_thresholds(float th_min, float th_max);
void logic_get_thresholds(float *th_min, float *th_max);
const char *logic_get_status();
void logic_reset_status(void);

#endif
