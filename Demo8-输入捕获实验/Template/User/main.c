#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"
#include "key.h"
#include "timer.h"
#include "usart.h"
#include "freq.h"

int main(void)
{
    //按键值
    uint8_t keyValue=0;
    //	初始化滴答定时器
	systick_config();	
	
    //初始化LED引脚
	Init_LED_GPIO();
	
    //初始化按键引脚
    Init_Key_GPIO();
    
    //初始化串口0
    Init_USART(115200);
    
    //初始化频率测量定时器
    Init_FreqTimer();
    
	while(1)
	{
        
	}
}
