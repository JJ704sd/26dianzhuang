#ifndef __KEY_H
#define __KEY_H

#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>

enum
{
	KEY1 = 1,
	KEY2 = 2,
	KEY3 = 3,
};

void Init_Key_GPIO(void);

uint8_t Scanf_Key_GPIO(void);

void Key_Handle(uint8_t keyValue);

#endif