#include "scope_view.h"

#include <stddef.h>

static uint16_t history_sample(const uint16_t *history,
                               uint16_t history_size,
                               uint16_t oldest,
                               uint16_t offset)
{
    return history[(uint16_t)((oldest + offset) % history_size)];
}

static uint16_t packed12_history_sample(const uint8_t *history,
                                        uint16_t history_size,
                                        uint16_t oldest,
                                        uint16_t offset)
{
    return (uint16_t)((uint16_t)history[(uint16_t)((oldest + offset) %
                                                   history_size)] << 4U);
}

static uint8_t median5(uint8_t value0, uint8_t value1, uint8_t value2,
                       uint8_t value3, uint8_t value4)
{
    uint8_t values[5] = {value0, value1, value2, value3, value4};
    uint8_t i;

    for(i = 1U; i < 5U; ++i)
    {
        uint8_t j = i;
        while((j > 0U) && (values[j] < values[j - 1U]))
        {
            const uint8_t temporary = values[j];
            values[j] = values[j - 1U];
            values[j - 1U] = temporary;
            --j;
        }
    }
    return values[2];
}

static uint16_t packed12_filtered_sample(const uint8_t *history,
                                         uint16_t history_size,
                                         uint16_t oldest,
                                         uint16_t source_count,
                                         uint16_t offset)
{
    const uint16_t offset0 = (offset > 1U) ? (uint16_t)(offset - 2U) : 0U;
    const uint16_t offset1 = (offset > 0U) ? (uint16_t)(offset - 1U) : 0U;
    const uint16_t offset3 = ((uint16_t)(offset + 1U) < source_count) ?
                             (uint16_t)(offset + 1U) :
                             (uint16_t)(source_count - 1U);
    const uint16_t offset4 = ((uint16_t)(offset + 2U) < source_count) ?
                             (uint16_t)(offset + 2U) :
                             (uint16_t)(source_count - 1U);

    return (uint16_t)((uint16_t)median5(
        (uint8_t)(packed12_history_sample(history, history_size, oldest,
                                           offset0) >> 4U),
        (uint8_t)(packed12_history_sample(history, history_size, oldest,
                                           offset1) >> 4U),
        (uint8_t)(packed12_history_sample(history, history_size, oldest,
                                           offset) >> 4U),
        (uint8_t)(packed12_history_sample(history, history_size, oldest,
                                           offset3) >> 4U),
        (uint8_t)(packed12_history_sample(history, history_size, oldest,
                                           offset4) >> 4U)) << 4U);
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
    uint16_t lag = 0U;
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
    if ((mode == SCOPE_VIEW_FREE) && (history_count > source_count))
    {
        const uint16_t max_lag = (uint16_t)(history_count - source_count);
        lag = (free_phase < max_lag) ? free_phase : max_lag;
    }
    oldest = (uint16_t)((write_index + history_size - source_count - lag) %
                        history_size);

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
        /* write_index already advances the newest chronological window.
         * Rotating that window by free_phase splices newest data back to its
         * oldest sample and creates a non-physical display edge. */
        (void)free_phase;
        start = 0U;
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

    if (source_count <= output_count)
    {
        for (i = 0U; i < output_count; ++i)
        {
            output[i] = history_sample(
                history, history_size, oldest,
                (uint16_t)((start + i) % source_count));
        }
    }
    else
    {
        const uint16_t pair_count = (uint16_t)((output_count + 1U) / 2U);
        uint16_t pair;

        /* Preserve both extrema from each chronological source bucket. This
         * is a peak-detect display, not an average: narrow noise spikes remain
         * visible when 200-800 raw samples are compressed to 120 LCD points.
         */
        for (pair = 0U; pair < pair_count; ++pair)
        {
            const uint16_t bucket_begin =
                (uint16_t)(((uint32_t)pair * source_count) / pair_count);
            uint16_t bucket_end =
                (uint16_t)(((uint32_t)(pair + 1U) * source_count) / pair_count);
            uint16_t bucket_offset = bucket_begin;
            uint16_t minimum_offset = bucket_begin;
            uint16_t maximum_offset = bucket_begin;
            uint16_t minimum_value = history_sample(
                history, history_size, oldest,
                (uint16_t)((start + bucket_begin) % source_count));
            uint16_t maximum_value = minimum_value;
            const uint16_t output_index = (uint16_t)(pair * 2U);

            if (bucket_end <= bucket_begin) { bucket_end = bucket_begin + 1U; }
            for (bucket_offset = (uint16_t)(bucket_begin + 1U);
                 bucket_offset < bucket_end; ++bucket_offset)
            {
                const uint16_t sample = history_sample(
                    history, history_size, oldest,
                    (uint16_t)((start + bucket_offset) % source_count));
                if (sample < minimum_value)
                {
                    minimum_value = sample;
                    minimum_offset = bucket_offset;
                }
                if (sample > maximum_value)
                {
                    maximum_value = sample;
                    maximum_offset = bucket_offset;
                }
            }

            if (minimum_offset <= maximum_offset)
            {
                output[output_index] = minimum_value;
                if ((uint16_t)(output_index + 1U) < output_count)
                {
                    output[output_index + 1U] = maximum_value;
                }
            }
            else
            {
                output[output_index] = maximum_value;
                if ((uint16_t)(output_index + 1U) < output_count)
                {
                    output[output_index + 1U] = minimum_value;
                }
            }
        }
    }
    return output_count;
}

uint16_t ScopeView_CopyUniformWindow(const uint16_t *history,
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
    uint16_t lag = 0U;
    uint16_t i;

    if (mode != SCOPE_VIEW_FREE)
    {
        return ScopeView_CopyWindow(history, history_size, history_count,
                                    write_index, span_samples, free_phase,
                                    mode, output, output_capacity, info);
    }
    (void)free_phase;
    if ((history == NULL) || (output == NULL) || (info == NULL) ||
        (history_size == 0U) || (history_count == 0U) ||
        (span_samples == 0U) || (output_capacity == 0U))
    {
        return 0U;
    }
    if (history_count > history_size) { history_count = history_size; }
    source_count = (history_count < span_samples) ? history_count : span_samples;
    output_count = (source_count < output_capacity) ? source_count : output_capacity;
    if (history_count > source_count)
    {
        const uint16_t max_lag = (uint16_t)(history_count - source_count);
        lag = (free_phase < max_lag) ? free_phase : max_lag;
    }
    oldest = (uint16_t)((write_index + history_size - source_count - lag) %
                        history_size);

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
    for (i = 0U; i < output_count; ++i)
    {
        const uint16_t offset = (output_count == 1U) ? 0U :
            (uint16_t)(((uint32_t)i * (source_count - 1U)) /
                       (output_count - 1U));
        output[i] = history_sample(history, history_size, oldest, offset);
    }
    return output_count;
}

uint16_t ScopeView_CopyPacked12Window(const uint8_t *history,
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
    uint16_t lag = 0U;
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
    if ((mode == SCOPE_VIEW_FREE) && (history_count > source_count))
    {
        const uint16_t max_lag = (uint16_t)(history_count - source_count);
        lag = (free_phase < max_lag) ? free_phase : max_lag;
    }
    oldest = (uint16_t)((write_index + history_size - source_count - lag) %
                        history_size);

    info->minimum = packed12_filtered_sample(history, history_size, oldest,
                                             source_count, 0U);
    info->maximum = info->minimum;
    info->source_count = source_count;
    info->trigger_index = 0U;
    info->trigger_found = 0U;
    for (i = 1U; i < source_count; ++i)
    {
        const uint16_t sample =
            packed12_filtered_sample(history, history_size, oldest,
                                     source_count, i);
        if (sample < info->minimum) { info->minimum = sample; }
        if (sample > info->maximum) { info->maximum = sample; }
    }

    if (mode == SCOPE_VIEW_FREE)
    {
        (void)free_phase;
        start = 0U;
    }
    else if (info->maximum > info->minimum)
    {
        midpoint = (uint16_t)(((uint32_t)info->minimum + info->maximum) / 2U);
        for (i = 1U; i < source_count; ++i)
        {
            const uint16_t previous = packed12_filtered_sample(
                history, history_size, oldest, source_count,
                (uint16_t)(i - 1U));
            const uint16_t current = packed12_filtered_sample(
                history, history_size, oldest, source_count, i);
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
            (uint16_t)(((uint32_t)i * (source_count - 1U)) /
                       (output_count - 1U));
        output[i] = packed12_filtered_sample(
            history, history_size, oldest, source_count,
            (uint16_t)((start + offset) % source_count));
    }
    return output_count;
}
