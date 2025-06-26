#ifndef __MID_TIMER_H
#define __MID_TIMER_H

#include "main.h"

#define PAUSE_MS_TIMER 0x01
#define RUN_MS_TIMER   0x02

uint16_t get_key_timer_value(void);
void set_key_timer_value(uint16_t value);
void set_key_bit_value(uint8_t value);

uint16_t get_tft_timer_value(void);
void set_tft_timer_value(uint16_t value);
void set_tft_bit_value(uint8_t value);

uint32_t get_freq_value(void);

#endif
