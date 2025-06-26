#ifndef __OSC_TASK_H
#define __OSC_TASK_H

#include "main.h"

#define OSC_PAUSE 	0x01
#define OSC_RUN		0x02

void TFT_StaticUI(void);
void TFT_ShowUI(void);
void osc_waveShow(uint16_t vref_value);
void key_scanf_handle(const uint16_t key_pin,const uint8_t key_state);
void Register_oscShowData(void);
void clear_adc_value(void);
void set_step_value(uint8_t value);
uint8_t get_step_value(void);
void set_osc_stop_bit(uint8_t value);
uint8_t get_osc_stop_bit(void);
#endif

