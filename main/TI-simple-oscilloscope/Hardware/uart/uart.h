#ifndef __USART_H
#define __USART_H

#include "ti_msp_dl_config.h"
#include <stdio.h>

void Init_USART(uint32_t bound);
void uart0_send_char(char ch);
#endif