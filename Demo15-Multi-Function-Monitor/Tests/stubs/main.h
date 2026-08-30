#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#define TIMER14 14U
#define TIMER_CH_0 0U
#define TIMER_EVENT_SRC_UPG 1U
void timer_channel_output_pulse_value_config(uint32_t timer, uint16_t channel,
                                              uint16_t pulse);
void timer_autoreload_value_config(uint32_t timer, uint16_t reload);
void timer_counter_value_config(uint32_t timer, uint16_t counter);
void timer_event_software_generate(uint32_t timer, uint16_t event);
void timer_enable(uint32_t timer);
void timer_disable(uint32_t timer);
#endif
