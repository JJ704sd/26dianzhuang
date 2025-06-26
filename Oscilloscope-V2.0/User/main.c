#include "gd32e23x.h"
#include "systick.h"
#include <string.h>
#include <stdio.h>
#include "hw_led.h"
#include "main.h"
#include "hw_gpio.h"
#include "hw_key.h"
#include "hw_ec11.h"
#include "hw_lcdinit.h"
#include "hw_spi.h"
#include "hw_tim.h"
#include "hw_adc.h"
#include "mid_timer.h"
#include "mid_pwm.h"
#include "mid_adc.h"
#include "mid_lcd.h"
#include "osc_task.h"

enum led_instance
{
	led1 = 0,
	led2
};
enum key_instance
{
	key1 = 0,
	key2,
	key3,
	keyd
};

static struct led_class led_hanlde[2];	//创建2个led灯实例句柄
static struct key_class key_handle[4];	//创建4个按键实例句柄
static struct ec11_class ec11_handle;		//创建1个ec11实例句柄

int main(void)
{   
	uint16_t key_timer_value = 0;
	uint16_t tft_timer_value = 0;
	uint16_t adc_vref_value = 0;
	uint8_t step_value = 0;
	uint8_t osc_stop_bit = 0;
	
  systick_config();		//时钟初始化
	
	mx_gpio_init();			//gpio配置初始化
	mx_spi0_init();			//spi0配置初始化
	mx_adc_init();			//adc配置初始化
	mx_tim2_init();			//tim2配置初始化
	mx_tim14_init();		//tim14配置初始化
	mx_tim15_init();		//tim15配置初始化
	
	led_hanlde[led1] = led_init(LED1_GPIO_Port,LED1_Pin,RESET);	//注册led实例
	led_hanlde[led2] = led_init(LED2_GPIO_Port,LED2_Pin,RESET);	//注册led实例
	
	key_handle[key1] = key_init(KEY1_GPIO_Port,KEY1_Pin,RESET);	//注册key实例
	key_handle[key2] = key_init(KEY2_GPIO_Port,KEY2_Pin,RESET);	//注册key实例
	key_handle[key3] = key_init(KEY3_GPIO_Port,KEY3_Pin,RESET);	//注册key实例
	key_handle[keyd] = key_init(KEYD_GPIO_Port,KEYD_Pin,RESET);	//注册key实例
	
	ec11_handle = ec11_init(KEYA_GPIO_Port,KEYA_Pin,KEYB_GPIO_Port,KEYB_Pin);	//注册ec11实例
	
	Set_ADC_Channel(ADC_CHANNEL_17);
	adc_vref_value = Get_ADC_Average(200);
	
	delay_1ms(1000);
	TFT_Init();
	delay_1ms(1000);	//初始化两次，解决上电白屏问题，看起来像tft复位不同步，但是在初次初始化前加大延时无效果
	TFT_Init();
	TFT_Fill(0,0,160,128,BLACK);
	TFT_StaticUI();

	Set_ADC_Channel(ADC_CHANNEL_3);
	clear_adc_value();
	Register_oscShowData();
	
	timer_enable(TIMER15);	//开始消息定时器计时
	timer_enable(TIMER2);		//开始频率捕获定时器计时
	while(1)
	{  
		key_timer_value = get_key_timer_value();
		tft_timer_value = get_tft_timer_value();
		if(key_timer_value >= 10)
		{
			key_scanf(&key_handle[key1]);
			key_scanf(&key_handle[key2]);
			key_scanf(&key_handle[key3]);
			key_scanf(&key_handle[keyd]);
			set_key_timer_value(0);
		}
		if(tft_timer_value >= 500)
		{
			set_key_bit_value(PAUSE_MS_TIMER);
			set_tft_bit_value(PAUSE_MS_TIMER);
			osc_stop_bit = get_osc_stop_bit();
			if(osc_stop_bit == OSC_RUN){
				osc_waveShow(adc_vref_value);
			}
			TFT_ShowUI();
			set_tft_timer_value(0);
			set_key_bit_value(RUN_MS_TIMER);
			set_tft_bit_value(RUN_MS_TIMER);
		}
		if(key_handle[key1].key_state != KEY_NoPress)
		{
			key_scanf_handle(key_handle[key1].key_pin,key_handle[key1].key_state);
			key_handle[key1].key_state = KEY_NoPress;
		}
		if(key_handle[key2].key_state != KEY_NoPress)
		{
			key_scanf_handle(key_handle[key2].key_pin,key_handle[key2].key_state);
			key_handle[key2].key_state = KEY_NoPress;
		}
		if(key_handle[key3].key_state != KEY_NoPress)
		{
			key_scanf_handle(key_handle[key3].key_pin,key_handle[key3].key_state);
			key_handle[key3].key_state = KEY_NoPress;
		}
		if(key_handle[keyd].key_state != KEY_NoPress)
		{
			key_scanf_handle(key_handle[keyd].key_pin,key_handle[keyd].key_state);
			key_handle[keyd].key_state = KEY_NoPress;
		}
		if(ec11_handle.ec11_direction != ec11_static)
		{
			if(ec11_handle.ec11_direction == ec11_forward)
			{
				step_value = get_step_value();
				step_value = step_value - 1;
				if(step_value <= 1)
				{
					step_value = 1;
				}
				set_step_value(step_value);
			}
			else
			{
				step_value = get_step_value();
				step_value = step_value + 1;
				if(step_value > 20)
				{
					step_value = 20;
				}
				set_step_value(step_value);
			}
			ec11_handle.ec11_direction = ec11_static;
		}
	}
}

void EXTI4_15_IRQHandler(void)
{
	if(exti_interrupt_flag_get(EXTI_4) != RESET)
	{
		ec11_exti_callback(&ec11_handle);
	}
}