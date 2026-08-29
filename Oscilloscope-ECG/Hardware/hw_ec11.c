#include "hw_ec11.h"

/*
 * 函数内容：EC11注册
 * 函数参数：
						GPIO_TypeDef *ec11_Agpio -- EC11的A端口
						uint16_t ec11_Apin -- EC11的A引脚
						GPIO_TypeDef *ec11_Bgpio -- EC11的B端口
						uint16_t ec11_Bpin -- EC11的B引脚
 * 返回值：无
 */
struct ec11_class ec11_init(uint32_t ec11_Agpio,uint32_t ec11_Apin,uint32_t ec11_Bgpio,uint32_t ec11_Bpin)
{
	struct ec11_class handle;
	handle.ec11_Agpio = ec11_Agpio;
	handle.ec11_Apin = ec11_Apin;
	handle.ec11_Bgpio = ec11_Bgpio;
	handle.ec11_Bpin = ec11_Bpin;
	handle.ec11_direction = ec11_static;
	return handle;
}

/*
 * 函数内容：EC11中断回调函数，需配置为双边沿触发
 * 函数参数：struct ec11_class *handle -- ec11句柄
 * 返回值：无
 */
void ec11_exti_callback(struct ec11_class *handle)
{
	static uint8_t cnt = 0,b_value = 0;
	
	//A相下降沿率先触发一次
	if((gpio_input_bit_get(handle->ec11_Agpio,handle->ec11_Apin) == RESET) && (cnt == 0))
	{
		cnt++;
		b_value = 0;
		//如果此时B相为高电平
		if(gpio_input_bit_get(handle->ec11_Bgpio,handle->ec11_Bpin) == SET)
		{
			b_value = 1;
		}
	}
	//A相上升沿再触发一次
	else if((gpio_input_bit_get(handle->ec11_Agpio,handle->ec11_Apin) == SET) && (cnt == 1))
	{
		cnt = 0;
		//如果B相此时是低电平，且之前是高电平
		if((gpio_input_bit_get(handle->ec11_Bgpio,handle->ec11_Bpin) == RESET) && (b_value == 1))
		{
			//反转
			handle->ec11_direction = ec11_reversal;
		}
		//如果此时B相是高电平，且之前是低电平
		else if((gpio_input_bit_get(handle->ec11_Bgpio,handle->ec11_Bpin) == SET) && (b_value == 0))
		{
			//正转
			handle->ec11_direction = ec11_forward;
		}
	}
	exti_interrupt_flag_clear(EXTI_4);
}
