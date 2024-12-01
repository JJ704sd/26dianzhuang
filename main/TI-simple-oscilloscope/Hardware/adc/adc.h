#ifndef _ADC_H
#define _ADC_H

#include "ti_msp_dl_config.h"
#include "main.h"

extern volatile struct Oscilloscope oscilloscope;


uint16_t Get_ADC_Value(uint16_t value);
void Init_ADC(void);
void ADC_DMA_Init(void);
void ADC_Config(void);
unsigned int Get_ADC_Value2(uint8_t  ADC_CHANNEL_x);
#endif