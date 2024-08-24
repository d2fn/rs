#include "rs_math.h"

float rs_remap(float value, float input_min, float input_max, float output_min, float output_max) {
    // Handle cases where input_max < input_min by swapping the input range
    if (input_max < input_min) {
        float temp = input_max;
        input_max = input_min;
        input_min = temp;
    }

    // Handle cases where output_max < output_min by swapping the output range
    if (output_max < output_min) {
        float temp = output_max;
        output_max = output_min;
        output_min = temp;
    }

    // Apply the standard mapping formula
    return output_min + (value - input_min) * (output_max - output_min) / (input_max - input_min);
}
