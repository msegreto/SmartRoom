#include "logic.h"
#include "config.h"
#include "leds.h"
#include "sys/log.h"
#include "res-control.h" 
#include <stdio.h>

#define LOG_MODULE "Logic"
#define LOG_LEVEL LOG_LEVEL_APP

static float last_temp = DEFAULT_TEMPERATURE;
static float last_hum = DEFAULT_HUMIDITY;
static float threshold_min = DEFAULT_THRESHOLD_MIN;
static float threshold_max = DEFAULT_THRESHOLD_MAX;
static warming_state_t state = WARMING_NONE;
extern void trigger_status_change(void);

void logic_set_temp(float t) {
  last_temp = t;
  logic_check();
}

void logic_set_hum(float h) {
  last_hum = h;
  logic_check();
}

void logic_set_thresholds(float th_min, float th_max) {
  threshold_min = th_min;
  threshold_max = th_max;
}

void logic_get_thresholds(float *th_min, float *th_max) {
  *th_min = threshold_min;
  *th_max = threshold_max;
}

void logic_reset_status(void) {
  state = WARMING_NONE;
  leds_off(LEDS_ALL);
}

void logic_check() {
  float hum = last_hum / 100;
  float perceived = last_temp - 0.55f * (1 - hum) * (last_temp - 14.5f);
  LOG_INFO("Perceived temp: %.2f\n", perceived);

  warming_state_t old_state = state;

  if (perceived > threshold_max) {
    state = WARMING_COOLING;
    leds_on(LEDS_GREEN);
    leds_off(LEDS_RED);
  } else if (perceived < threshold_min) {
    state = WARMING_HEATING;
    leds_on(LEDS_RED);
    leds_off(LEDS_GREEN);
  } else {
    state = WARMING_NONE;
    leds_off(LEDS_ALL);
  }

  if (state != old_state) {
    LOG_INFO("State changed from %d to %d\n", old_state, state);
    trigger_status_change();
  }
}

const char *logic_get_status() {
  switch (state) {
    case WARMING_COOLING: return "cooling";
    case WARMING_HEATING: return "heating";
    default: return "none";
  }
}

warming_state_t logic_get_state(void) {
  return state;
}