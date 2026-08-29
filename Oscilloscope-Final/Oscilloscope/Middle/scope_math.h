#ifndef SCOPE_MATH_H
#define SCOPE_MATH_H

#include <stdint.h>

typedef struct
{
    float minimum;
    float maximum;
    float peak_to_peak;
    uint16_t trigger_index;
} scope_frame_info_t;

uint8_t scope_analyze_frame(const float *samples,
                            uint16_t count,
                            uint16_t display_width,
                            scope_frame_info_t *info);
int16_t scope_scale_to_plot(float sample,
                            float minimum,
                            float maximum,
                            int16_t plot_height);

#endif
