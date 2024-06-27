#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include <string.h>
#include "led.h"
#include "main.h"
#include "tft_init.h"
#include "tft.h"

int main(void)
{
    //	初始化滴答定时器
    systick_config();
    
    //初始化LED引脚
    Init_LED_GPIO();
    
    //初始化TFT屏幕引脚及默认配置
    TFT_Init();
    
    //TFT显示白色填充区域
    TFT_Fill(0,0,160,128,WHITE);
    while(1)
    {
        
    }
}
