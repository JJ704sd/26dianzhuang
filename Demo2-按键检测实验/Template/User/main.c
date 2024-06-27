#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"
#include "key.h"

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
	while(1)
	{
      //按键扫描
      keyValue=Scanf_Key_GPIO();
      
      //按键处理
      Key_Handle(keyValue);
	}
}
