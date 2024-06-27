#include "ec11.h"
#include "led.h"

/*
*   函数内容：初始化EC11引脚,DE与B引脚与普通引脚一样
*   函数参数：无
*   返回值：  无
*/
void Init_EC11_GPIO(void)
{
    //使能时钟
    rcu_periph_clock_enable(RCU_GPIOB);
    
    //设置输出模式，上拉
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_3|GPIO_PIN_9);

}

/*
*   函数内容：初始化EC11引脚,旋转判断引脚A需要使用中断
*   函数参数：无
*   返回值：  无
*/
void Init_EC11_EXITGPIO(void)
{
	//使能时钟GPIOB，CMP
  rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_CFGCMP);
	
	//设置引脚模式，上拉
	gpio_mode_set(GPIOB,GPIO_MODE_INPUT,GPIO_PUPD_PULLUP,GPIO_PIN_4);
	
	//中断使能
	nvic_irq_enable(EXTI4_15_IRQn,1U);
	
	//配置中断线
	syscfg_exti_line_config(EXTI_SOURCE_GPIOB,EXTI_SOURCE_PIN4);
	
	//初始化中断线，设置为中断模式，上升沿和下降沿触发
	exti_init(EXTI_4,EXTI_INTERRUPT,EXTI_TRIG_BOTH);
	
  //中断线使能
  exti_interrupt_enable(EXTI_4);
    
	//中断标志位清除
	exti_interrupt_flag_clear(EXTI_4);    
}

uint8_t A_cnt=0;
uint8_t B_value=0;
/*
*   函数内容：中断线4-15中断服务函数
*   函数参数：无
*   返回值：  无
*/
void EXTI4_15_IRQHandler(void)
{
   if(RESET != exti_interrupt_flag_get(EXTI_4))  
   {
		 if((gpio_input_bit_get(GPIOB,GPIO_PIN_4) == RESET) && (A_cnt == 0))	//A相下降沿触发一次
		 {
			 A_cnt++;			//计数值加一，表示已经触发了第一次中断
			 B_value = 0;	//读取B相电平，若为高电平则B_level置1，反之保持0
			 if(gpio_input_bit_get(GPIOB,GPIO_PIN_3) == SET)
			 {
				 B_value = 1;
			 }
		 }
		 else if((gpio_input_bit_get(GPIOB,GPIO_PIN_4) == SET) && (A_cnt == 1))	//A相上升沿触发一次
		 {
			 A_cnt = 0;
			 if((B_value == 1) && (gpio_input_bit_get(GPIOB,GPIO_PIN_3) == RESET))
			 {
				 Open_LED(1);CLose_LED(2);
				 //正转
			 }
			 if((B_value == 0) && (gpio_input_bit_get(GPIOB,GPIO_PIN_3) == SET))
			 {
				 Open_LED(2);CLose_LED(1);
				 //反转
			 }
		 }
		 exti_interrupt_flag_clear(EXTI_4);
   }
}
