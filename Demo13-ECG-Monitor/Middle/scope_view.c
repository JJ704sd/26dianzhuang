#include "scope_view.h"

#include <stddef.h>

static uint16_t history_sample(const uint16_t *history,
                               uint16_t history_size,
                               uint16_t oldest,
                               uint16_t offset)
{
    return history[(uint16_t)((oldest + offset) % history_size)];
}

uint16_t ScopeView_CopyWindow(const uint16_t *history,
                              uint16_t history_size,
                              uint16_t history_count,
                              uint16_t write_index,
                              uint16_t span_samples,
                              uint16_t free_phase,
                              scope_view_mode_t mode,
                              uint16_t *output,
                              uint16_t output_capacity,
                              scope_view_info_t *info)
{
    uint16_t source_count;
    uint16_t output_count;
    uint16_t oldest;
    uint16_t start = 0U;
    uint16_t midpoint;
    uint16_t i;

    if ((history == NULL) || (output == NULL) || (info == NULL) ||
        (history_size == 0U) || (history_count == 0U) ||
        (span_samples == 0U) || (output_capacity == 0U))
    {
        return 0U;
    }
    if (history_count > history_size) { history_count = history_size; }
    source_count = (history_count < span_samples) ? history_count : span_samples;
    output_count = (source_count < output_capacity) ? source_count : output_capacity;
    oldest = (uint16_t)((write_index + history_size - source_count) % history_size);

    info->minimum = history_sample(history, history_size, oldest, 0U);
    info->maximum = info->minimum;
    info->source_count = source_count;
    info->trigger_index = 0U;
    info->trigger_found = 0U;
    for (i = 1U; i < source_count; ++i)
    {
        const uint16_t sample = history_sample(history, history_size, oldest, i);
        if (sample < info->minimum) { info->minimum = sample; }
        if (sample > info->maximum) { info->maximum = sample; }
    }

    if (mode == SCOPE_VIEW_FREE)
    {
        start = (uint16_t)(free_phase % source_count);
    }
    else if (info->maximum > info->minimum)
    {
        midpoint = (uint16_t)(((uint32_t)info->minimum + info->maximum) / 2U);
        for (i = 1U; i < source_count; ++i)
        {
            const uint16_t previous =
                history_sample(history, history_size, oldest, (uint16_t)(i - 1U));
            const uint16_t current =
                history_sample(history, history_size, oldest, i);
            const uint8_t crossing = (mode == SCOPE_VIEW_RISING) ?
                ((previous <= midpoint) && (current > midpoint)) :
                ((previous > midpoint) && (current <= midpoint));

            if (crossing != 0U)
            {
                info->trigger_index = i;
                info->trigger_found = 1U;
                start = (uint16_t)((i + source_count - (source_count / 20U)) %
                                   source_count);
                break;
            }
        }
    }

    for (i = 0U; i < output_count; ++i)
    {
        const uint16_t offset = (output_count == 1U) ? 0U :
            (uint16_t)(((uint32_t)i * source_count) / output_count);
        output[i] = history_sample(history, history_size, oldest,
                                   (uint16_t)((start + offset) % source_count));
    }
    return output_count;
}
