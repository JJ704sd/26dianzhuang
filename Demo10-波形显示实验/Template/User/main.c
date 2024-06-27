#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"
#include "tft_init.h"
#include "tft.h"

int main(void)
{
    uint8_t i=0;
    uint8_t data[120]={0};
    //	初始化滴答定时器
	systick_config();	
	
    //初始化LED引脚
	Init_LED_GPIO();
	
    //初始化TFT屏幕引脚及默认配置
    TFT_Init();
    //TFT屏幕显示UI
    TFT_ShowUI();
    
	while(1)
	{
        //模拟ADC采集数据
        for(i=0;i<30;i++)
        {
           data[i]=0; 
        }
        for(i=30;i<60;i++)
        {
           data[i]=33; 
        }
        for(i=60;i<90;i++)
        {
           data[i]=0; 
        }
        for(i=90;i<120;i++)
        {
           data[i]=33; 
        }
        
        //显示波形
        for(i=0;i<120;i++)
        {
            drawCurve(80,data[i]);
        }
	}
}
