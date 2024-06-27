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
    //初始化滴答定时器
	systick_config();	
	
    //初始化LED引脚
	Init_LED_GPIO();
	
    //初始化按键引脚
    Init_Key_GPIO();
    
    //初始化PWM输出，输出频率=（72000000/((71+1)*(period+1))）
    //占空比是周期与比较值的判断，同时还与极性与模式有关
    //比较值*周期则是占空比，500/1000=50%占空比，周期是1K
    Init_PWM_Output(999,500);
    
	while(1)
	{
        
	}
}
