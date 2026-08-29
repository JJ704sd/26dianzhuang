#include "mid_timer.h"
#include "ecg_task.h"
#include "timer_math.h"

static __IO uint16_t key_timer_value;			//按键定时器
static __IO uint16_t tft_timer_value;			//屏幕定时器
static __IO uint32_t runtime_ms;				//1ms运行时间，按uint32_t自然回绕
static volatile uint8_t key_timer_bit = RUN_MS_TIMER;		//ms定时器标志位
static volatile uint8_t tft_timer_bit = RUN_MS_TIMER;		//ms定时器标志位

/*
 * 函数内容：定时器更新中断回调函数
 * 函数参数：TIM_HandleTypeDef *htim -- 定时器句柄
 * 返回值：无
 */

void TIMER15_IRQHandler(void)
{
	timer_interrupt_flag_clear(TIMER15,TIMER_INT_FLAG_UP);
	runtime_ms++;
	ECG_TimerTick1ms();

	if(key_timer_bit == RUN_MS_TIMER)
	{
		key_timer_value++;
		if(key_timer_value >= 10000){
			key_timer_value = 0;		//防止数据越界
		}
	}

	if(tft_timer_bit == RUN_MS_TIMER)
	{
		tft_timer_value++;
		if(tft_timer_value >= 10000){
			tft_timer_value = 0;		//防止数据越界
		}
	}
}

/*
 * 函数内容：得到1ms运行时间，溢出时自然回绕
 * 函数参数：无
 * 返回值：uint32_t -- 1ms计数值
 */
uint32_t get_runtime_ms(void)
{
	return runtime_ms;
}

/*
 * 函数内容：得到按键定时器值
 * 函数参数：无
 * 返回值：uint16_t -- 定时器值
 */
uint16_t get_key_timer_value(void)
{
	return key_timer_value;
}

/*
 * 函数内容：设置按键定时器值
 * 函数参数：无
 * 返回值：uint16_t -- 定时器值
 */
void set_key_timer_value(uint16_t value)
{
	key_timer_value =  value;
}

/*
 * 函数内容：设置按键定时器标志位值
 * 函数参数：uint8_t value
 * 返回值：无
 */
void set_key_bit_value(uint8_t value)
{
	key_timer_bit = value;
}

/*
 * 函数内容：得到屏幕定时器值
 * 函数参数：无
 * 返回值：uint16_t -- 定时器值
 */
uint16_t get_tft_timer_value(void)
{
	return tft_timer_value;
}

/*
 * 函数内容：设置屏幕定时器值
 * 函数参数：无
 * 返回值：uint16_t -- 定时器值
 */
void set_tft_timer_value(uint16_t value)
{
	tft_timer_value =  value;
}

/*
 * 函数内容：设置屏幕定时器标志位值
 * 函数参数：uint8_t value
 * 返回值：无
 */
void set_tft_bit_value(uint8_t value)
{
	tft_timer_bit = value;
}

static __IO uint16_t ccnumber = 0;						//捕获次数
static __IO uint32_t freq = 0;							//频率值
static __IO uint16_t readvalue1 = 0, readvalue2 = 0;	//两次捕获值
static __IO uint32_t count = 0;							//周期间隔时长
static __IO uint16_t timer2_overflow_count = 0;

/*
 * 函数内容：定时器通道捕获回调函数
 * 函数参数：TIM_HandleTypeDef *htim -- 定时器句柄
 * 返回值：无
 */
void TIMER2_IRQHandler(void)
{
	const uint8_t capture_pending =
		(SET == timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_CH0)) ? 1U : 0U;
	const uint8_t update_pending =
		(SET == timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_UP)) ? 1U : 0U;
	uint16_t captured_value = 0U;

	if(capture_pending != 0U)
	{
		captured_value = timer_channel_capture_value_register_read(TIMER2, TIMER_CH_0);
	}
	if(update_pending != 0U)
	{
		if((ccnumber == 1U) && (timer2_overflow_count < 0xFFFFU) &&
		   ((capture_pending == 0U) ||
		    (timer_pending_wrap_precedes_capture(captured_value,0xFFFFU) != 0U)))
		{
			timer2_overflow_count++;
		}
		timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
	}
	if(capture_pending != 0U)
  {
		if(0 == ccnumber){
			readvalue1 = captured_value;
			timer2_overflow_count = 0U;
			ccnumber = 1;
		}
		else if(1 == ccnumber)
		{
			readvalue2 = captured_value;
			count = timer_capture_elapsed(readvalue1,readvalue2,
									 timer2_overflow_count,0xFFFFU);
			freq = (count == 0U) ? 0U : (1000000U / count);
			readvalue1 = 0;readvalue2 = 0;
			ccnumber = 0;
			count=0;
			timer2_overflow_count = 0U;
		}
		timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_CH0);
	}
}

/*
 * 函数内容：得到频率值
 * 函数参数：无
 * 返回值：uint32_t -- 频率值
 */
uint32_t get_freq_value(void)
{
	return freq;
}
