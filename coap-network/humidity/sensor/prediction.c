#include "prediction.h"
#include "humidity_forecast_converted.h"
#include "buffer.h"
#include <stdint.h>

float predict_humidity(void) {
    float *buffer = get_buffer();
    int start = get_buffer_start_index();
    int16_t input_fixed[BUFFER_SIZE];

    for (int i = 0; i < BUFFER_SIZE; i++) {
        int index = (start + i) % BUFFER_SIZE;
        input_fixed[i] = (int16_t)((buffer[index] - MIN_INPUT) * SCALE);
    }

    // Uso della funzione generata da emlearn per DecisionTreeRegressor
    int32_t prediction = humidity_forecast_predict(input_fixed, BUFFER_SIZE);

    float output = ((float)prediction / SCALE);
    return output;
}
