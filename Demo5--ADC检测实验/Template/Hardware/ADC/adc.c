#include "adc.h"

/**********************************************************
 * 函 数 名 称：ADC_Config
 * 函 数 功 能：初始化ADC
 * 传 入 参 数：无
 * 函 数 返 回：无
 * 作       者：LC
 * 备       注：
**********************************************************/
void ADC_Config(void)
{
    //使能引脚时钟
    rcu_periph_clock_enable(RCU_GPIOA);      
    //使能ADC时钟
    rcu_periph_clock_enable(RCU_ADC);
    //配置ADC时钟
    rcu_adc_clock_config(RCU_ADCCK_AHB_DIV3);
    //配置引脚为模拟浮空输入模式
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3);
    
    //数据右对齐
    adc_data_alignment_config(ADC_DATAALIGN_RIGHT);
    
    //通道长度1
    adc_channel_length_config( ADC_REGULAR_CHANNEL, 1U);
    
    //软件外部触发
    adc_external_trigger_source_config( ADC_REGULAR_CHANNEL, ADC_EXTTRIG_REGULAR_NONE); 
    //外部触发使能
    adc_external_trigger_config(ADC_REGULAR_CHANNEL, ENABLE);

    //使能adc
    adc_enable();
    delay_1ms(1U);
    //开启自校准
    adc_calibration_enable();
}

/**********************************************************
 * 函 数 名 称：Get_ADC_Value
 * 函 数 功 能：读取ADC值
 * 传 入 参 数：ADC_CHANNEL_x=要采集的通道
 * 函 数 返 回：测量到的值
 * 作       者：LC
 * 备       注：默认采样周期为55.5个ADC采样时间
**********************************************************/
uint16_t Get_ADC_Value(uint8_t  ADC_CHANNEL_x)
{
    float vref=0;
    float adc_value = 0;
        
    //设置采集通道
    adc_regular_channel_config(0, ADC_CHANNEL_x, ADC_SAMPLETIME_55POINT5);
    //开始软件转换
    adc_software_trigger_enable(ADC_REGULAR_CHANNEL);

    //等待 ADC 采样完成 
    while ( adc_flag_get(ADC_FLAG_EOC) == RESET ){            
    }
    adc_value = adc_regular_data_read();
    
    //返回采样值
    return adc_value;
}