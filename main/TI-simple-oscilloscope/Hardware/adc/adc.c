#include "adc.h"
#include "uart.h"

#define ADC_VALUE_NUM 300U

uint16_t adc_value[ADC_VALUE_NUM];

/*
*   函数内容：得到ADC值
*   函数参数：value--数组下标
*   返回值：  无
*/
uint16_t Get_ADC_Value(uint16_t value)
{
    uint16_t returnValue=0;
    if(value>ADC_VALUE_NUM)
    {
        value=0;
    }
    returnValue=adc_value[value];
    adc_value[value]=0;
    return returnValue;
}
/*
*   函数内容：初始化ADC
*   函数参数：无
*   返回值：    无
*/
void Init_ADC(void)
{
    //清除ADC中断标志
    NVIC_ClearPendingIRQ(ADC12_0_INST_INT_IRQN);
    //使能ADC中断
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
}

void ADC_DMA_Init(void)
{
    //设置DMA搬运的起始地址
    DL_DMA_setSrcAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &ADC0->ULLMEM.MEMRES[0]);
    //设置DMA搬运的目的地址
    DL_DMA_setDestAddr(DMA, DMA_CH0_CHAN_ID, (uint32_t) &adc_value[0]);

    DL_DMA_setTransferSize(DMA, DMA_CH0_CHAN_ID, ADC_VALUE_NUM);
    //开启DMA
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);
    //开启ADC转换
    DL_ADC12_startConversion(ADC12_0_INST);
}

extern volatile struct Oscilloscope oscilloscope;

void ADC0_IRQHandler(void)
{
    //如果ADC的DMA完成
    if( DL_ADC12_getPendingInterrupt(ADC12_0_INST) == DL_ADC12_IIDX_DMA_DONE )
    {
        oscilloscope.showbit=1;
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
        //清除中断标志位
        DL_DMA_clearInterruptStatus(DMA,DL_DMA_INTERRUPT_CHANNEL0);
        DL_ADC12_clearInterruptStatus(ADC12_0_INST, DL_ADC12_INTERRUPT_DMA_DONE);
    }
}