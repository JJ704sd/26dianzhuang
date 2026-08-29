#include "osc_window.h"

uint8_t osc_window_find(const int16_t *samples,
                        uint16_t count,
                        int16_t threshold,
                        uint16_t width,
                        uint16_t *start)
{
    uint16_t i;
    uint16_t latest_start;

    if ((samples == 0) || (start == 0) || (width == 0U) || (count < width)) {
        return 0U;
    }

    *start = 0U;
    latest_start = (uint16_t)(count - width);

    for (i = 0U; ((uint32_t)i + 2U) < count; ++i) {
        if ((samples[i] < threshold) && (samples[i + 2U] > threshold)) {
            *start = (i > latest_start) ? latest_start : i;
            break;
        }
    }

    return 1U;
}

uint8_t osc_window_find_auto(const int16_t *samples,
                             uint16_t count,
                             uint16_t width,
                             uint16_t pretrigger,
                             uint16_t min_span,
                             uint16_t *start)
{
    uint16_t i;
    uint16_t latest_start;
    int16_t minimum;
    int16_t maximum;
    int32_t span;
    int32_t low_level;
    int32_t high_level;
    uint8_t armed = 0U;

    if ((samples == 0) || (start == 0) || (width == 0U) ||
        (count < width) || (pretrigger >= width)) {
        return 0U;
    }

    *start = 0U;
    latest_start = (uint16_t)(count - width);
    minimum = samples[0];
    maximum = samples[0];

    for (i = 1U; i < count; ++i) {
        if (samples[i] < minimum) {
            minimum = samples[i];
        }
        if (samples[i] > maximum) {
            maximum = samples[i];
        }
    }

    span = (int32_t)maximum - (int32_t)minimum;
    if (span < (int32_t)min_span) {
        return 1U;
    }

    low_level = (int32_t)minimum + ((span * 3) / 8);
    high_level = (int32_t)minimum + ((span * 5) / 8);

    for (i = 0U; i < count; ++i) {
        if ((int32_t)samples[i] <= low_level) {
            armed = 1U;
        } else if ((armed != 0U) && ((int32_t)samples[i] >= high_level)) {
            uint16_t candidate = (i > pretrigger) ?
                                 (uint16_t)(i - pretrigger) : 0U;
            *start = (candidate > latest_start) ? latest_start : candidate;
            break;
        }
    }

    return 1U;
}
