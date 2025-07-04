#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Enable button HAL for Cooja simulation */
#ifdef CONTIKI_TARGET_COOJA
#define BUTTON_HAL_CONF_ENABLE 1
#define COOJA_CONF_SIMULATE_TURNAROUND 0
#endif

#endif /* PROJECT_CONF_H_ */