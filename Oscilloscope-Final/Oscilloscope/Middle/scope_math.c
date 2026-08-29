#include "scope_math.h"

#define SCOPE_FLAT_EPSILON 0.001f

uint8_t scope_analyze_frame(const float *samples,
                            uint16_t count,
                            uint16_t display_width,
                            scope_frame_info_t *info)
{
    uint16_t i;
    uint16_t latest_start;
    float threshold;

    if ((samples == 0) || (info == 0) ||
        (display_width == 0U) || (count < display_width)) {
        return 0U;
    }

    info->minimum = samples[0];
    info->maximum = samples[0];
    info->trigger_index = 0U;

    for (i = 1U; i < count; ++i) {
        if (samples[i] < info->minimum) {
            info->minimum = samples[i];
        }
        if (samples[i] > info->maximum) {
            info->maximum = samples[i];
        }
    }

    info->peak_to_peak = info->maximum - info->minimum;
    if (info->peak_to_peak <= SCOPE_FLAT_EPSILON) {
        return 1U;
    }

    threshold = info->minimum + (info->peak_to_peak * 0.5f);
    latest_start = (uint16_t)(count - display_width);
    for (i = 0U; ((uint32_t)i + 1U) < count; ++i) {
        if ((samples[i] <= threshold) && (samples[i + 1U] > threshold)) {
            info->trigger_index = (i > latest_start) ? latest_start : i;
            break;
        }
    }

    return 1U;
}

int16_t scope_scale_to_plot(float sample,
                            float minimum,
                            float maximum,
                            int16_t plot_height)
{
    float span;
    float scaled;

    if (plot_height <= 1) {
        return 0;
    }

    span = maximum - minimum;
    if (span <= SCOPE_FLAT_EPSILON) {
        return (int16_t)(plot_height / 2);
    }

    scaled = ((sample - minimum) * (float)(plot_height - 1)) / span;
    if (scaled <= 0.0f) {
        return 0;
    }
    if (scaled >= (float)(plot_height - 1)) {
        return (int16_t)(plot_height - 1);
    }
    return (int16_t)(scaled + 0.5f);
}
