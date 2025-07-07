#ifndef BUFFER_H
#define BUFFER_H

#include "config.h"

float *get_buffer(void);
float get_latest_value(void);
void update_buffer(float new_value);
int buffer_is_full(void);
int get_buffer_start_index(void);

#endif /* BUFFER_H */
