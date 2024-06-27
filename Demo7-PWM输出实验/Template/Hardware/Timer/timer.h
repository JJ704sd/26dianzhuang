#ifndef __TIMER_H
#define __TIMER_H

#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>

void Init_PWM_Output(uint32_t period,uint32_t pulse);
void Set_Output_PWMComparex(uint16_t value);
void Set_Output_Freq(uint32_t value);

#endif