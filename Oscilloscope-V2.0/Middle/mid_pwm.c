#include "mid_pwm.h"

static uint8_t 	pwm_state = PWM_OFF;
static uint16_t pwm_period = 1000;		//默认周期为1000
static uint16_t pwm_duty = 500;			//默认占空比为500						
	
/*
 * 函数内容：设置pwm输出的占空比
 * 函数参数：uint16_t pwm_duty -- pwm占空比
 * 返回值：无
 */
void set_pwm_duty(uint16_t duty)
{
	pwm_duty = duty;
	timer_channel_output_pulse_value_config(TIMER14, TIMER_CH_0, duty);
}

/*
 * 函数内容：得到pwm输出的占空比
 * 函数参数：无
 * 返回值：uint16_t pwm_duty
 */
uint16_t get_pwm_duty(void)
{
	return pwm_duty;
}

/*
 * 函数内容：设置pwm输出的周期
 * 函数参数：uint16_t pwm_period -- pwm周期
 * 返回值：无
 */
void set_pwm_period(uint16_t period)
{
	pwm_period = period;
	timer_autoreload_value_config(TIMER14,period);
}

/*
 * 函数内容：得到pwm输出的周期
 * 函数参数：无
 * 返回值：uint16_t pwm_period
 */
uint16_t get_pwm_period(void)
{
	return pwm_period;
}

/*
 * 函数内容：设置pwm输出的状态
 * 函数参数：uint8_t state -- 状态值
 * 返回值：无
 */
void set_pwm_state(uint8_t state)
{
	if(state == PWM_ON)
	{
		//timer_channel_output_state_config(TIMER14,TIMER_CH_0,TIMER_CCXN_ENABLE);
		timer_enable(TIMER14);	//开始pwm输出定时器计时
		pwm_state = PWM_ON;
	}
	else if(state == PWM_OFF)
	{
		//timer_channel_output_state_config(TIMER14,TIMER_CH_0,TIMER_CCXN_DISABLE); 
		timer_disable(TIMER14);	//开始pwm输出定时器计时
		pwm_state = PWM_OFF;
	}
}

/*
 * 函数内容：得到pwm输出的状态
 * 函数参数：无
 * 返回值：uint8_t -- pwm状态
 */
uint8_t get_pwm_state(void)
{
	return pwm_state;
}

/*
 * 函数内容：得到pwm的频率
 * 函数参数：无
 * 返回值：uint16_t
 */
uint16_t get_pwm_out_freq(void)
{
	uint16_t pwm_freq = 0;
	pwm_freq = TIMER_FREQ / pwm_period;
	return pwm_freq;
}
