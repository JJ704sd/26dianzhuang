#ifndef __TIMER_H
#define __TIMER_H

#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>

void Init_GeneralTimer15(uint32_t period);
void startTimer15(void);

#endif