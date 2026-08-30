#ifndef __MID_TIMER_H
#define __MID_TIMER_H

#include "main.h"

#define PAUSE_MS_TIMER 0x01U
#define RUN_MS_TIMER   0x02U

typedef void (*mid_timer_callback_t)(void);

uint16_t get_key_timer_value(void);
void set_key_timer_value(uint16_t value);
void set_key_bit_value(uint8_t value);

uint16_t get_tft_timer_value(void);
void set_tft_timer_value(uint16_t value);
void set_tft_bit_value(uint8_t value);

void mid_timer_register_periodic_callback(mid_timer_callback_t callback, uint16_t period_ms);
uint32_t get_freq_value(void);

#endif
