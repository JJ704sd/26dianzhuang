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
	
	//中断线使能
	nvic_irq_enable(EXTI4_15_IRQn,1U);
	
	//配置中断线
	syscfg_exti_line_config(EXTI_SOURCE_GPIOB,EXTI_SOURCE_PIN4);
	
	//初始化中断线，设置为中断模式，上升沿触发
	exti_init(EXTI_4,EXTI_INTERRUPT,EXTI_TRIG_RISING);
	
	//中断标志位清除
	exti_interrupt_flag_clear(EXTI_4);    
}

/*
*   函数内容：中断线4-15中断服务函数，这里为了防止误触，做了旋转计数判断，需要沿一个方向多旋转几次才算成功
              注意中断服务函数中不要做操作时间长的操作
*   函数参数：无
*   返回值：  无
*/
void EXTI4_15_IRQHandler(void)
{
   //顺时针判断计数
   static uint8_t clockwiseNum=0;
   //逆时针判断计数
   static uint8_t anticlockwiseNum=0;
    
   if(RESET != exti_interrupt_flag_get(EXTI_4))  
   {
        if(gpio_input_bit_get(GPIOB,GPIO_PIN_3)==RESET) //顺时针转两格
        {
            delay_1ms(5);
            anticlockwiseNum=0;
            if(gpio_input_bit_get(GPIOB,GPIO_PIN_3)==RESET) //顺时针转两格
            {
                clockwiseNum++;
                if(clockwiseNum>=2)
                {
                    clockwiseNum=0;
                    Open_LED(led2);Open_LED(led3);
                }            
            }
        }
        else                                            //逆时针转两格
        {
            delay_1ms(5);
            clockwiseNum=0;
            if(gpio_input_bit_get(GPIOB,GPIO_PIN_3)==SET) //逆时针转两格
            {
                anticlockwiseNum++;
                if(anticlockwiseNum>=2)
                {
                    anticlockwiseNum=0;
                    CLose_LED(led2);CLose_LED(led3);
                }
            }
        }
        exti_interrupt_flag_clear(EXTI_4);
    }
}