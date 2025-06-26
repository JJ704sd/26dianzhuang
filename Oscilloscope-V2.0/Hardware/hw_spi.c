#include "hw_spi.h"

/*
*   函数内容：初始化SPI0
*   函数参数：无
*   返回值：无
*/
void mx_spi0_init(void)
{
    //spi结构体参数
    spi_parameter_struct spi_init_struct;
    
    //使能GPIOA时钟
    rcu_periph_clock_enable(RCU_GPIOA);
    
    //使能SPI0时钟
    rcu_periph_clock_enable(RCU_SPI0);
    
    //设置复用功能
    gpio_af_set(GPIOA, GPIO_AF_0,   GPIO_PIN_5 | GPIO_PIN_7);
    
    //模式设置
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5 | GPIO_PIN_7);
    
    //输出设置
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_7);
    
    //SPI0复位
    spi_i2s_deinit(SPI0);
    
    //SPI参数初始化
    spi_struct_para_init(&spi_init_struct);
    
    spi_init_struct.trans_mode           = SPI_TRANSMODE_BDTRANSMIT;//单线双向半双工发送模式
    spi_init_struct.device_mode          = SPI_MASTER;              //主机模式
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;      //8位数据位宽
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE; //极性高，第2个边缘采样
    spi_init_struct.nss                  = SPI_NSS_SOFT;            //NSS软件控制
    spi_init_struct.prescale             = SPI_PSC_2;               //2分频
    spi_init_struct.endian               = SPI_ENDIAN_MSB;          //高位在前
    spi_init(SPI0, &spi_init_struct);
    
    spi_enable(SPI0);
}