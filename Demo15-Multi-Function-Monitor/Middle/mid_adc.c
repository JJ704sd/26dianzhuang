#include "mid_adc.h"

#include "hw_adc.h"

#define ADC_SOFTWARE_TIMEOUT 100000U

static uint8_t adc_convert_bit = ADC_CONVERT_UN_FINSIH;
static uint16_t *adc_dma_buffer;
static uint16_t adc_dma_count;
static mid_adc_dma_callback_t adc_dma_callback;

uint16_t Get_ADC_Sample(void)
{
    uint32_t timeout = ADC_SOFTWARE_TIMEOUT;

    adc_flag_clear(ADC_FLAG_EOC);
    adc_software_trigger_enable(ADC_REGULAR_CHANNEL);
    while((adc_flag_get(ADC_FLAG_EOC) == RESET) && (timeout > 0U)){
        timeout--;
    }
    if(timeout == 0U){
        return 0U;
    }
    return adc_regular_data_read();
}

uint16_t Get_ADC_Average(uint16_t num)
{
    uint16_t i;
    uint16_t value;
    uint16_t max_value = 0U;
    uint16_t min_value = 0xFFFFU;
    uint32_t sum_value = 0U;

    if(num < 3U){
        num = 3U;
    }
    for(i = 0U; i < num; i++){
        value = Get_ADC_Sample();
        if(value < min_value){
            min_value = value;
        }
        if(value > max_value){
            max_value = value;
        }
        sum_value += value;
    }
    sum_value -= max_value;
    sum_value -= min_value;
    return (uint16_t)(sum_value / (num - 2U));
}

void Set_ADC_Channel(uint8_t channel)
{
    adc_regular_channel_config(0U, channel, ADC_SAMPLETIME_13POINT5);
}

void mid_adc_start_dma(uint16_t *buffer, uint16_t count,
                       mid_adc_dma_callback_t callback)
{
    adc_dma_buffer = buffer;
    adc_dma_count = count;
    adc_dma_callback = callback;
    adc_convert_bit = ADC_CONVERT_UN_FINSIH;
    mx_adc_dma_init((uint32_t)buffer, count);
}

void DMA_Channel0_IRQHandler(void)
{
    uint16_t half_count = adc_dma_count / 2U;

    if(dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_HTF) != RESET){
        dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_HTF);
        if((adc_dma_callback != 0) && (adc_dma_buffer != 0) && (half_count != 0U)){
            adc_dma_callback(adc_dma_buffer, half_count);
        }
    }
    if(dma_interrupt_flag_get(DMA_CH0, DMA_INT_FLAG_FTF) != RESET){
        adc_convert_bit = ADC_CONVERT_FINSIH;
        dma_interrupt_flag_clear(DMA_CH0, DMA_INT_FLAG_FTF);
        if((adc_dma_callback != 0) && (adc_dma_buffer != 0) && (half_count != 0U)){
            adc_dma_callback(&adc_dma_buffer[half_count], half_count);
        }
    }
}

uint8_t get_adc_convert_value(void)
{
    return adc_convert_bit;
}

void set_adc_convert_value(uint8_t bit)
{
    adc_convert_bit = bit;
}
