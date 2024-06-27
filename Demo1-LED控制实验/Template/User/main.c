#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"


int main(void)
{
    //	初始化滴答定时器
	systick_config();	
	
    //初始化LED引脚
	Init_LED_GPIO();
	
	while(1)
	{
        //打开LED2、LED3
        Open_LED(led2);Open_LED(led3);
        
        //延时500ms
        delay_1ms(500);
        
        //关闭LED2、LED3
        CLose_LED(led2);CLose_LED(led3);
        
        //延时500ms
        delay_1ms(500);
	}
}
