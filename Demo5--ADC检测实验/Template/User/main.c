#include "gd32e23x.h"
#include "systick.h"
#include <stdio.h>
#include "led.h"
#include "main.h"
#include "key.h"
#include "usart.h"
#include "ec11.h"
#include "adc.h"

//ADC接收数据缓冲区
float adcData[100]={0};

int main(void)
{
    int i=0;
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
    
    //初始化ADC引脚
    ADC_Config();
    
	while(1)
	{
       for(i=0;i<1;i++)
       {
            adcData[i]=(Get_ADC_Value(ADC_CHANNEL_3)*3.3f)/4096.0f; 
            delay_1us(20);                              //这里的延时不能太长，否则采集数据失真，太短数据量太大
       }
       for(i=0;i<1;i++)
       {
           printf("%.1f ",adcData[i]);
       }
       printf("\r\n");
       delay_1ms(500);
	}
}
