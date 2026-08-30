#include "mid_adc.h"
#include "hw_adc.h"
#include "stdio.h"
#include "string.h"

static volatile uint8_t adc_convert_bit = ADC_CONVERT_UN_FINSIH;
static volatile uint16_t adc_scope_buffer[ADC_SCOPE_DMA_SAMPLES];
static volatile uint16_t adc_scope_ready_offset;
static volatile uint8_t adc_scope_ready_sequence;
static volatile uint8_t adc_scope_ready;

/*
 * 函数内容：冒泡排序
 * 函数参数：无
 * 返回值：无
 */
static void bubble_sort(float arr[], uint16_t len)
{
    uint16_t i = 0, j = 0;
    float temp = 0;
    for (i = 0; i < len - 1; i++)
    {
        for (j = 0; j < len - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

/*
 * 函数内容；得到对应ADC通道值
 * 函数参数：无
 * 返回值：无
 */
static uint16_t Get_ADC_Val(void)
{
	uint16_t val = 0;
	
	//设置采集通道
	adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
	//等待 ADC 采样完成 
	while ( adc_flag_get(ADC_FLAG_EOC) == RESET ){            
	}
	val = adc_regular_data_read();
	
	return val;
}

/* Read the latest result from the TIMER0-triggered ADC without waiting. */
uint16_t Get_ADC_Latest(void)
{
	return adc_regular_data_read();
}

void ADC_StreamInit(void)
{
    adc_scope_ready_offset = 0U;
    adc_scope_ready_sequence = 0U;
    adc_scope_ready = 0U;
    mx_adc_scope_dma_init((uint32_t)adc_scope_buffer,
                          ADC_SCOPE_DMA_SAMPLES);
}

uint16_t ADC_StreamCopyLatestDisplay(int8_t *destination, uint16_t capacity,
                                     uint8_t *frame_span)
{
    uint16_t i;
    uint16_t offset;
    uint8_t sequence_before;
    uint8_t sequence_after;
    int16_t minimum;
    int16_t maximum;

    if (frame_span != (uint8_t *)0)
    {
        *frame_span = 0U;
    }

    if ((destination == (int8_t *)0) ||
        (capacity < ADC_SCOPE_HALF_SAMPLES) ||
        (adc_scope_ready == 0U))
    {
        return 0U;
    }
    do
    {
        sequence_before = adc_scope_ready_sequence;
        offset = adc_scope_ready_offset;
        minimum = 127;
        maximum = -128;
        for (i = 0U; i < ADC_SCOPE_HALF_SAMPLES; ++i)
        {
            int32_t centered = (int32_t)adc_scope_buffer[offset + i] - 2048;
            int16_t display_sample;

            centered /= 16;
            if (centered > 127) { centered = 127; }
            if (centered < -128) { centered = -128; }
            display_sample = (int16_t)centered;
            destination[i] = (int8_t)display_sample;
            if (display_sample < minimum) { minimum = display_sample; }
            if (display_sample > maximum) { maximum = display_sample; }
        }
        sequence_after = adc_scope_ready_sequence;
    } while (sequence_before != sequence_after);
    if (frame_span != (uint8_t *)0)
    {
        *frame_span = (uint8_t)(maximum - minimum);
    }
    return ADC_SCOPE_HALF_SAMPLES;
}

/*
 * 函数内容；得到ADC通道的平均值值
 * 函数参数：无
 * 返回值：无
 */
uint16_t Get_ADC_Average(uint16_t num)
{
    uint16_t i = 0,val = 0;
    uint32_t sum_val = 0;
    uint16_t max_value = 0,min_value = 9999;
    for(i =0;i<num;i++)
    {
        val = Get_ADC_Val();
        if(min_value > val){
            min_value = val;
        }
        if(max_value < val){
            max_value = val;
        }
        sum_val = sum_val + val;
    }
    sum_val = sum_val - max_value - min_value;
    sum_val = sum_val / (num - 2);
    return sum_val;
}

/*
 * 函数内容；设置ADC通道
 * 函数参数：无
 * 返回值：无
 */
void Set_ADC_Channel(uint8_t channel)
{
	//ADC常规通道配置--PA3，顺序组0，通道3，采样时间13.5个时钟周期
	adc_regular_channel_config(0, channel, ADC_SAMPLETIME_13POINT5);
}

/*
 * 函数内容；ADC转换完成中断回调
 * 函数参数：无
 * 返回值：无
 */
void DMA_Channel0_IRQHandler(void)
{
	if(dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_HTF)){
        adc_scope_ready_sequence++;
        adc_scope_ready_offset = 0U;
        adc_scope_ready = 1U;
        adc_scope_ready_sequence++;
        dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_HTF);
    }
	if(dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_FTF)){
        adc_scope_ready_sequence++;
        adc_scope_ready_offset = ADC_SCOPE_HALF_SAMPLES;
        adc_scope_ready = 1U;
		adc_convert_bit = ADC_CONVERT_FINSIH;
        adc_scope_ready_sequence++;
		dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_FTF);
	}
}

/*
 * 函数内容；得到adc转换完成标志位
 * 函数参数：无
 * 返回值：uint8_t
 */
uint8_t get_adc_convert_value(void)
{
	return adc_convert_bit;
}

/*
 * 函数内容；设置adc转换完成标志位
 * 函数参数：无
 * 返回值：uint8_t
 */
void set_adc_convert_value(uint8_t bit)
{
	adc_convert_bit = bit;
}
