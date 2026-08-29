#ifndef __ADC_H
#define __ADC_H

#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>

void ADC_Config(void);
uint16_t Get_ADC_Value(uint8_t  ADC_CHANNEL_x);

#endif