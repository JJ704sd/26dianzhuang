#include "hw_tim.h"

/*
*   函数内容：初始化通用定时器15
*   函数参数：无
*   返回值：  无
*/
void mx_tim15_init(void)
{   
    rcu_periph_clock_enable(RCU_TIMER15);	//使能时钟
    
    timer_parameter_struct timer_initpara;	//定时器初始化参数结构体
    
		timer_deinit(TIMER15);	//复位定时器15
    
    timer_prescaler_config(TIMER15,1,TIMER_PSC_RELOAD_NOW);	//配置外设预分频器    
    
    timer_struct_para_init(&timer_initpara);	// 初始化结构体
    
    // 配置定时器参数 
		timer_initpara.prescaler        = 71;                 	//  时钟预分频值 0-65535  psc_clk = CK_TIMER / pre
		timer_initpara.alignedmode      = TIMER_COUNTER_EDGE;   // 边缘对齐
		timer_initpara.counterdirection = TIMER_COUNTER_UP;     // 向上计数
		timer_initpara.period           = 999;                  // 周期
		//在输入捕获的时候使用  数字滤波器使用的采样频率之间的分频比例 
		timer_initpara.clockdivision = TIMER_CKDIV_DIV1;        // 分频因子
		//只有高级定时器才有 配置为x，就重复x+1次进入中断 
		timer_initpara.repetitioncounter = 0; 									// 重复计数器 0-255  
		timer_init(TIMER15,&timer_initpara); 										// 初始化定时器
    
    timer_interrupt_flag_clear(TIMER15,TIMER_INT_FLAG_UP);	//定时器更新中断标志位清除
    
    nvic_irq_enable(TIMER15_IRQn,2);	//中断优先级
    
    timer_interrupt_enable(TIMER15,TIMER_INT_UP); //使能更新事件中断

    timer_disable(TIMER15);	//失能定时器15
}

/*
*   函数内容：初始化通用定时器2
*   函数参数：无
*   返回值：  无
*/
void mx_tim2_init(void)
{
    timer_ic_parameter_struct timer_icinitpara;	//定时器通道输入参数结构体 
    
    timer_parameter_struct timer_initpara;	//定时器参数结构体
    
    rcu_periph_clock_enable(RCU_GPIOA);	//使能时钟
    rcu_periph_clock_enable(RCU_TIMER2);
    
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_6);	//设置引脚模式
    
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,GPIO_PIN_6);	//设置输出状态
    
    gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_6);	//设置为复用功能
    
    nvic_irq_enable(TIMER2_IRQn, 1U);	//定时器中断使能
    
    timer_deinit(TIMER2);	//复位定时器
     
    timer_struct_para_init(&timer_initpara);	//定时器参数初始化
    
    timer_initpara.prescaler         = 71;                  //预分频器参数
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  //边沿对齐
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;    //向上计数
    timer_initpara.period            = 65535;               //周期
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;    //时钟分频
    timer_init(TIMER2, &timer_initpara);                    //参数初始化
    
    timer_channel_input_struct_para_init(&timer_icinitpara);	//定时器通道输入参数初始化
    
    timer_icinitpara.icpolarity  = TIMER_IC_POLARITY_RISING;        //通道输入极性
    timer_icinitpara.icselection = TIMER_IC_SELECTION_DIRECTTI;     //通道输入模式选择
    timer_icinitpara.icprescaler = TIMER_IC_PSC_DIV1;               //通道输入捕获预分频
    timer_icinitpara.icfilter    = 0x00;                            //通道输入捕获滤波
    timer_input_capture_config(TIMER2,TIMER_CH_0,&timer_icinitpara);
    
    timer_auto_reload_shadow_enable(TIMER2);	//使能自动重装载值
    
    timer_interrupt_flag_clear(TIMER2,TIMER_INT_FLAG_CH0);	//清除中断标志位
    
    timer_interrupt_enable(TIMER2,TIMER_INT_CH0);	//使能定时器通道中断
    
    timer_disable(TIMER2);	//定时器中断失能
}

/*
*   函数内容：初始化通用定时器14
*   函数参数：无
*   返回值：  无
*/
void mx_tim14_init(void)
{
	timer_oc_parameter_struct timer_ocinitpara;	//定时器输出参数结构体
	
  timer_parameter_struct timer_initpara;	//定时器初始化参数结构体
	
	rcu_periph_clock_enable(RCU_GPIOA);	//使能时钟
	
	rcu_periph_clock_enable(RCU_TIMER14);	//使能定时器14
	
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_2);	//GPIO复用模式设置--PA2-TIMER14_CH0
	
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,GPIO_PIN_2);	//输出类型设置
	
	gpio_af_set(GPIOA, GPIO_AF_0, GPIO_PIN_2);	//复用模式0
	
	timer_deinit(TIMER14);	//复位定时器14
	
	timer_struct_para_init(&timer_initpara);	//初始化定时器结构体参数
	
	timer_initpara.prescaler         = 71;									//预分频器参数
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;	//边沿对齐
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;		//向上计数
	timer_initpara.period            = 999;									//周期
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;		//时钟分频
	timer_initpara.repetitioncounter = 0;								    //重装载值
	timer_init(TIMER14, &timer_initpara);
	
	timer_channel_output_struct_para_init(&timer_ocinitpara);	//初始化定时器通道输出参数结构体
	
	timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;						//输出状态，主输出通道开启
	timer_ocinitpara.outputnstate = TIMER_CCXN_DISABLE;			    //互补输出状态关闭
	timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_LOW;	    //输出极性为高
	timer_ocinitpara.ocnpolarity  = TIMER_OCN_POLARITY_LOW;     //互补输出极性为高
	timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_HIGH;   //空闲状态通道输出
	timer_ocinitpara.ocnidlestate = TIMER_OCN_IDLE_STATE_HIGH;  //空闲状态互补通道输出
	
	timer_channel_output_config(TIMER14, TIMER_CH_0, &timer_ocinitpara);
	
	timer_channel_output_pulse_value_config(TIMER14, TIMER_CH_0, 500);	//输出比较值
	
	//输出模式0，当计时器小于比较值时，输出有效电平，为高，大于比较器值时输出为低
	timer_channel_output_mode_config(TIMER14, TIMER_CH_0, TIMER_OC_MODE_PWM0);
	
	//影子模式输出关闭
	timer_channel_output_shadow_config(TIMER14, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);
	
	timer_auto_reload_shadow_enable(TIMER14);	//使能自动重装载
	
	timer_primary_output_config(TIMER14, ENABLE);	//配置定时器为主要输出函数，所有通道使能
	
	timer_disable(TIMER14);		//失能定时器
}