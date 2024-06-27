#include "timer.h"
#include "led.h"

/*
*   函数内容：初始化通用定时器15
*   函数参数：无
*   返回值：  无
*/
void Init_GeneralTimer(uint32_t period)
{   
    //使能时钟
    rcu_periph_clock_enable(RCU_TIMER15);
    
    //定时器初始化参数结构体
    timer_parameter_struct timer_initpara;
    
    //复位定时器15
	timer_deinit(TIMER15);
    
    //配置外设预分频器
    timer_prescaler_config(TIMER15,1,TIMER_PSC_RELOAD_NOW);
    
    // 初始化结构体
    timer_struct_para_init(&timer_initpara);
    
    // 配置定时器参数 
	timer_initpara.prescaler        = 7199;                 //  时钟预分频值 0-65535  psc_clk = CK_TIMER / pre
	timer_initpara.alignedmode      = TIMER_COUNTER_EDGE;   // 边缘对齐
	timer_initpara.counterdirection = TIMER_COUNTER_UP;     // 向上计数
	timer_initpara.period           = period;                   // 周期
	//在输入捕获的时候使用  数字滤波器使用的采样频率之间的分频比例 
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;        // 分频因子
	//只有高级定时器才有 配置为x，就重复x+1次进入中断 
	timer_initpara.repetitioncounter = 0; // 重复计数器 0-255  
	timer_init(TIMER15,&timer_initpara); // 初始化定时器
    
    //定时器更新中断标志位清除
    timer_interrupt_flag_clear(TIMER15,TIMER_INT_FLAG_UP);
    
    //中断优先级
    nvic_irq_enable(TIMER15_IRQn,0);
    
    //使能更新事件中断
    timer_interrupt_enable(TIMER15,TIMER_INT_UP); 

    timer_enable(TIMER15);
}

void TIMER15_IRQHandler(void)
{
    static uint8_t num=0;
    if(timer_interrupt_flag_get(TIMER15,TIMER_INT_FLAG_UP) == SET)    
    {
        if(num == 1){
            Open_LED(led2);Open_LED(led3);
        }
        else{
            num=0;
            CLose_LED(led2);CLose_LED(led3);
        }   
        num++;        
        //定时器更新中断标志位清除
        timer_interrupt_flag_clear(TIMER15,TIMER_INT_FLAG_UP);
    }
}