#ifndef TIMER_MATH_H
#define TIMER_MATH_H

#include <stdint.h>

uint32_t timer_capture_delta(uint16_t previous,
                             uint16_t current,
                             uint16_t counter_max);
uint32_t timer_capture_elapsed(uint16_t previous,
                               uint16_t current,
                               uint16_t completed_wraps,
                               uint16_t counter_max);
uint8_t timer_pending_wrap_precedes_capture(uint16_t captured,
                                             uint16_t counter_max);
uint8_t timer_measurement_is_fresh(uint32_t now_ms,
                                   uint32_t updated_ms,
                                   uint32_t timeout_ms,
                                   uint8_t valid);

#endif
