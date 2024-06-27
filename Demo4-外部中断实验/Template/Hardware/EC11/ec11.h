#ifndef __EC11_H
#define __EC11_H

#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>

extern int8_t ec11value ;

void Init_EC11_GPIO(void);

void Init_EC11_EXITGPIO(void);

#endif