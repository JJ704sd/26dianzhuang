#ifndef SCOPE_METRICS_H
#define SCOPE_METRICS_H

#include <stdint.h>

typedef struct
{
    uint8_t valid;
    uint8_t input_duty_percent;
} scope_metrics_t;

/*
 * Calculate the duty cycle of the external input.  The Demo15 analog front
 * end is inverting, so an input-high sample has a lower ADC code.
 */
scope_metrics_t ScopeMetrics_Analyze(const uint16_t *samples,
                                     uint16_t count,
                                     uint16_t minimum,
                                     uint16_t maximum,
                                     uint16_t minimum_span);

#endif
