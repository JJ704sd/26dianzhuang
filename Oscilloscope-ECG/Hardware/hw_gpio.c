#include "hw_gpio.h"

/*
 * 函数内容：gpio初始化
 * 函数参数：无
 * 返回值：无
 */
void mx_gpio_init(void)
{
	//使能时钟
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_CFGCMP);
	
	//设置默认引脚输出电平
	gpio_bit_write(LED1_GPIO_Port,LED1_Pin,SET);
	gpio_bit_write(LED2_GPIO_Port,LED2_Pin,SET);
	gpio_bit_write(LCD_RES_GPIO_Port,LCD_RES_Pin,SET);
	gpio_bit_write(LCD_DC_GPIO_Port,LCD_DC_Pin,SET);
	gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,SET);
	gpio_bit_write(LCD_BLK_GPIO_Port,LCD_BLK_Pin,SET);
	
	//设置输出模式，不上下拉
	gpio_mode_set(LED1_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED1_Pin);
	//设置输出模式，不上下拉
	gpio_mode_set(LED2_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED2_Pin);
	//设置输入模式，上拉
	gpio_mode_set(KEY1_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY1_Pin);
	//设置输入模式，上拉
	gpio_mode_set(KEY2_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY2_Pin);
	//设置输入模式，上拉
	gpio_mode_set(KEY3_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEY3_Pin);
	//设置输入模式，上拉
	gpio_mode_set(KEYD_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEYD_Pin);
	//设置输入模式，上拉
	gpio_mode_set(KEYB_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEYB_Pin);
	//设置输出模式，不上下拉
	gpio_mode_set(LCD_RES_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_RES_Pin);
	//设置输出模式，不上下拉
	gpio_mode_set(LCD_DC_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_DC_Pin);
	//设置输出模式，不上下拉
	gpio_mode_set(LCD_CS_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_CS_Pin);
	//设置输出模式，不上下拉
	gpio_mode_set(LCD_BLK_GPIO_Port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LCD_BLK_Pin);
	
	//设置输入模式，上拉
	gpio_mode_set(KEYA_GPIO_Port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, KEYA_Pin);
	
	//设置输出类型，推挽输出，50Mhz
	gpio_output_options_set(LED1_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED1_Pin);
	//设置输出类型，推挽输出，50Mhz
	gpio_output_options_set(LED2_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LED2_Pin);
	//设置输出类型，推挽输出，50Mhz
	gpio_output_options_set(LCD_RES_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_RES_Pin);
	//设置输出类型，推挽输出，50Mhz
	gpio_output_options_set(LCD_DC_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_DC_Pin);
	//设置输出类型，推挽输出，50Mhz
	gpio_output_options_set(LCD_CS_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_CS_Pin);
	//设置输出类型，推挽输出，50Mhz
	gpio_output_options_set(LCD_BLK_GPIO_Port, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_BLK_Pin);
	
	//中断线使能
	nvic_irq_enable(EXTI4_15_IRQn,3U);
	//配置中断线
	syscfg_exti_line_config(EXTI_SOURCE_GPIOB,EXTI_SOURCE_PIN4);
	//初始化中断线，设置为中断模式，上升沿下降沿触发
	exti_init(EXTI_4,EXTI_INTERRUPT,EXTI_TRIG_BOTH);
	//中断标志位清除
	exti_interrupt_flag_clear(EXTI_4);
}
