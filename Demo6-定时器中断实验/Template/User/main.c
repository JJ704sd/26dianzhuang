#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"
#include "key.h"
#include "timer.h"

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
    
    //初始化通用定时器15，时间等于（((7199+1)*(period+1))/72000000）s,
    Init_GeneralTimer(4999);
	while(1)
	{
        
	}
}
