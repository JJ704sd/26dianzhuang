#include "hw_adc.h"
#include "systick.h"

void mx_adc_init(void)
{
	//使能引脚
	rcu_periph_clock_enable(RCU_GPIOA);
	
	//使能ADC时钟
	rcu_periph_clock_enable(RCU_ADC);
	
	//使能时钟配置，最大28M
	rcu_adc_clock_config(RCU_ADCCK_AHB_DIV9);
	
	//引脚配置，PA3，模拟输入，无上下拉
	gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3);
	
	//由TIMER0以250Sa/s定时触发，匹配ECG采集与显示时间窗
	adc_special_function_config(ADC_CONTINUOUS_MODE, DISABLE);

	//ADC扫描功能失能，这里仅一个通道
	adc_special_function_config(ADC_SCAN_MODE, DISABLE);

	//ADC注入组自动转换模式失能，这里无需注入组
	adc_special_function_config(ADC_INSERTED_CHANNEL_AUTO, DISABLE);    
	
	//ADC数据右对齐
	adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
	
	//ADC通道长度配置
	adc_channel_length_config(ADC_REGULAR_CHANNEL, 1U);   

	//ADC常规通道配置--PA3，顺序组0，通道3，采样时间13.5个时钟周期
	adc_regular_channel_config(0, ADC_CHANNEL_3, ADC_SAMPLETIME_13POINT5);    
	
	/* ADC temperature and Vrefint enable */
  adc_tempsensor_vrefint_enable();
		
	//ADC触发器配置，软件触发
	adc_external_trigger_source_config(ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_T0_CH0);
	adc_external_trigger_config(ADC_REGULAR_CHANNEL, ENABLE);
	
	//使能ADC
	adc_enable();
	delay_1ms(1U);
	
	//使能校准和复位
	adc_calibration_enable();
	
}
