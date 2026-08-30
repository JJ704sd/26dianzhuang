#include "scope_metrics.h"

scope_metrics_t ScopeMetrics_Analyze(const uint16_t *samples,
                                     uint16_t count,
                                     uint16_t minimum,
                                     uint16_t maximum,
                                     uint16_t minimum_span)
{
    scope_metrics_t result = {0U, 0U};
    uint16_t threshold;
    uint16_t index;
    uint16_t input_high_count = 0U;

    if((samples == 0) || (count == 0U) || (maximum < minimum) ||
       ((uint16_t)(maximum - minimum) < minimum_span)){
        return result;
    }

    threshold = (uint16_t)(minimum + ((maximum - minimum) / 2U));
    for(index = 0U; index < count; index++){
        if(samples[index] <= threshold){
            input_high_count++;
        }
    }

    result.input_duty_percent =
        (uint8_t)(((uint32_t)input_high_count * 100U + (count / 2U)) / count);
    result.valid = 1U;
    return result;
}
