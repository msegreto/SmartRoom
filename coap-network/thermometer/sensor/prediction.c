#include "prediction.h"
#include "temperature_forecast_converted.h"
#include "eml_trees.h"
#include "buffer.h"
#include <stdint.h>

float predict_temperature(void) {
    float *buffer = get_buffer();
    int start = get_buffer_start_index();
    int32_t input_fixed[BUFFER_SIZE];

    for (int i = 0; i < BUFFER_SIZE; i++) {
        int index = (start + i) % BUFFER_SIZE;
        input_fixed[i] = (int32_t)((buffer[index] - MIN_INPUT) * SCALE);
    }

    int32_t prediction = eml_trees_regress(&temperature_forecast, input_fixed, BUFFER_SIZE);
    float output = ((float)prediction / SCALE);

    return output;
}
