// sensor/buffer.c
#include "buffer.h"

static float temperature_buffer[BUFFER_SIZE];
static int buffer_index = 0;
static int buffer_full = 0;

void update_buffer(float new_value) {
    temperature_buffer[buffer_index] = new_value;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
    if (buffer_index == 0) buffer_full = 1;
}

int buffer_is_full(void) { return buffer_full; }

float *get_buffer(void) { return temperature_buffer; }

float get_latest_value(void) {
    int last_index = (buffer_index + BUFFER_SIZE - 1) % BUFFER_SIZE;
    return temperature_buffer[last_index];
}

int get_buffer_start_index(void) {
    if (!buffer_full) {
        return 0; 
    }
    return buffer_index; 
}