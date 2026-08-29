#include "timer_math.h"

uint32_t timer_capture_delta(uint16_t previous,
                             uint16_t current,
                             uint16_t counter_max)
{
    return timer_capture_elapsed(previous, current, 0U, counter_max);
}

uint32_t timer_capture_elapsed(uint16_t previous,
                               uint16_t current,
                               uint16_t completed_wraps,
                               uint16_t counter_max)
{
    const uint32_t counter_period = (uint32_t)counter_max + 1U;
    uint32_t elapsed;

    if (completed_wraps == 0U)
    {
        if (current > previous)
        {
            return (uint32_t)current - previous;
        }
        return (counter_period - previous) + current;
    }

    elapsed = (uint32_t)completed_wraps * counter_period;
    if (current > previous)
    {
        elapsed += (uint32_t)current - previous;
    }
    else
    {
        elapsed -= (uint32_t)previous - current;
    }
    return elapsed;
}

uint8_t timer_pending_wrap_precedes_capture(uint16_t captured,
                                             uint16_t counter_max)
{
    return (captured <= (uint16_t)(counter_max / 2U)) ? 1U : 0U;
}

uint8_t timer_measurement_is_fresh(uint32_t now_ms,
                                   uint32_t updated_ms,
                                   uint32_t timeout_ms,
                                   uint8_t valid)
{
    if (valid == 0U)
    {
        return 0U;
    }
    return ((uint32_t)(now_ms - updated_ms) <= timeout_ms) ? 1U : 0U;
}
