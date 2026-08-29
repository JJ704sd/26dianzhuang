#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"
#include "key.h"
#include "usart.h"
#include "ec11.h"

int main(void)
{
	//按键值
	uint8_t keyValue=0;
	//初始化滴答定时器
	systick_config();	

	//初始化LED引脚
	Init_LED_GPIO();

	//初始化按键引脚
	Init_Key_GPIO();

	//初始化串口
	Init_USART(115200);
	//初始化串口DMA
	Init_USART0RecDMA();

	//初始化EC11引脚
	Init_EC11_GPIO();
	Init_EC11_EXITGPIO();
	
	while(1)
	{
		
	}
}
