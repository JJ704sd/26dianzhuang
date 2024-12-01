#include "tft_init.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/m0p/dl_core.h"
#include "tft.h"
#define delay_1ms(X) delay_cycles((CPUCLK_FREQ/1000)*X)


static const DL_SPI_Config gSPI_LCD_config_16 = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA1,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_16,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
};

static const DL_SPI_ClockConfig gSPI_LCD_clockConfig_16 = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_LCD_init_16(void) {
    DL_SPI_setClockConfig(SPI_LCD_INST, (DL_SPI_ClockConfig *) &gSPI_LCD_clockConfig_16);

    DL_SPI_init(SPI_LCD_INST, (DL_SPI_Config *) &gSPI_LCD_config_16);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     1600000 = (32000000)/((1 + 9) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_LCD_INST, 9);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_LCD_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);
    /* Enable module */
    DL_SPI_enable(SPI_LCD_INST);
}

/*
*   函数内容：初始化SPI0引脚
*   函数参数：无
*   返回值：无
*/
void Init_SPI0_GPIO(void)
{

}
void Init_SPI0_GPIO16(void)
{
    SYSCFG_DL_SPI_LCD_init_16();
}

/*
*   函数内容：SPI0发送数据
*   函数参数：无
*   返回值：无
*/
static void SPI0_Write(uint8_t data)
{
    //发送数据
    DL_SPI_transmitData8(SPI_LCD_INST,data);

    //等待SPI总线空闲
    while(DL_SPI_isBusy(SPI_LCD_INST));   

}
/*
*   函数内容：SPI0发送数据--16位
*   函数参数：无
*   返回值：无
*/
static void SPI0_Write16(uint16_t data)
{
    //发送数据
    DL_SPI_transmitData16(SPI_LCD_INST,data);
    // //等待SPI总线空闲
     while(DL_SPI_isBusy(SPI_LCD_INST));   
}
/*
*   函数内容：初始化TFT其余引脚
*   函数参数：无
*   返回值：无
*/
static void TFT_GPIO_Init(void)
{
    // //使能时钟
    // rcu_periph_clock_enable(RCU_GPIOB);
    
    // //设置输出模式，不上下拉
    // gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8);
    
    // //设置输出类型，推挽输出，50Mhz
    // gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8);

    // gpio_bit_set(GPIOB,GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8);
}

/*
*   函数内容：TFT发送单个字节数据
*   函数参数：无
*   返回值：无
*/
void TFT_WR_DATA8(uint8_t data)
{
    DL_GPIO_clearPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉低片选信号 
    
    SPI0_Write(data);
    
    DL_GPIO_setPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉高片选信号
}

/*
*   函数内容：TFT发送2个字节数据
*   函数参数：无
*   返回值：无
*/
void TFT_WR_DATA(uint16_t data)
{
    
    DL_GPIO_clearPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉低片选信号 

    SPI0_Write(data>>8);
    
    SPI0_Write(data);
    
    DL_GPIO_setPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉高片选信号
}
/*
*   函数内容：TFT发送2个字节数据，每次发送16个字节
*   函数参数：无
*   返回值：无
*/
void TFT_WR_DATA16(uint16_t data)
{
    DL_GPIO_clearPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉低片选信号 
    
    SPI0_Write16(data);

    DL_GPIO_setPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉高片选信号
}
/*
*   函数内容：TFT发送命令数据
*   函数参数：无
*   返回值：无
*/
void TFT_WR_REG(uint8_t reg)
{
    DL_GPIO_clearPins(GPIO_LCD_DC_PORT, GPIO_LCD_DC_PIN);//拉低命令信号 
    DL_GPIO_clearPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉低片选信号 

    SPI0_Write(reg);
    
    DL_GPIO_setPins(GPIO_LCD_DC_PORT, GPIO_LCD_DC_PIN);//拉高命令信号 
    DL_GPIO_setPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉高片选信号 
}

/*
*   函数内容：TFT发送命令数据，单次发送是16个字节
*   函数参数：无
*   返回值：无
*/
void TFT_WR_REG16(uint16_t reg)
{
    DL_GPIO_clearPins(GPIO_LCD_DC_PORT, GPIO_LCD_DC_PIN);//拉低命令信号 
    DL_GPIO_clearPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉低片选信号 
   
   SPI0_Write16(reg);
    
    DL_GPIO_setPins(GPIO_LCD_DC_PORT, GPIO_LCD_DC_PIN);//拉高命令信号 
    DL_GPIO_setPins(GPIO_LCD_CS_PORT, GPIO_LCD_CS_PIN);//拉高片选信号 
}

void TFT_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
	if(USE_HORIZONTAL==0)
	{
		TFT_WR_REG(0x2a);//列地址设置
		TFT_WR_DATA(x1);
		TFT_WR_DATA(x2);
		TFT_WR_REG(0x2b);//行地址设置
		TFT_WR_DATA(y1);
		TFT_WR_DATA(y2);
		TFT_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		TFT_WR_REG(0x2a);//列地址设置
		TFT_WR_DATA(x1);
		TFT_WR_DATA(x2);
		TFT_WR_REG(0x2b);//行地址设置
		TFT_WR_DATA(y1);
		TFT_WR_DATA(y2);
		TFT_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		TFT_WR_REG(0x2a);//列地址设置
		TFT_WR_DATA(x1);
		TFT_WR_DATA(x2);
		TFT_WR_REG(0x2b);//行地址设置
		TFT_WR_DATA(y1);
		TFT_WR_DATA(y2);
		TFT_WR_REG(0x2c);//储存器写
	}
	else
	{
		TFT_WR_REG(0x2a);//列地址设置
		TFT_WR_DATA(x1);
		TFT_WR_DATA(x2);
		TFT_WR_REG(0x2b);//行地址设置
		TFT_WR_DATA(y1);
		TFT_WR_DATA(y2);
		TFT_WR_REG(0x2c);//储存器写
	}    
}
/*
*   函数内容：设置显示地址，每次发送16位数据
*   函数参数：无
*   返回值：无
*/
void TFT_Address_Set16(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
	if(USE_HORIZONTAL==0)
	{
		TFT_WR_REG16(0x2a);//列地址设置
		TFT_WR_DATA16(x1);
		TFT_WR_DATA16(x2);
		TFT_WR_REG16(0x2b);//行地址设置
		TFT_WR_DATA16(y1);
		TFT_WR_DATA16(y2);
		TFT_WR_REG16(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		TFT_WR_REG16(0x2a);//列地址设置
		TFT_WR_DATA16(x1);
		TFT_WR_DATA16(x2);
		TFT_WR_REG16(0x2b);//行地址设置
		TFT_WR_DATA16(y1);
		TFT_WR_DATA16(y2);
		TFT_WR_REG16(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		TFT_WR_REG16(0x002a);//列地址设置
		TFT_WR_DATA16(x1);
		TFT_WR_DATA16(x2);
		TFT_WR_REG16(0x002b);//行地址设置
		TFT_WR_DATA16(y1);
		TFT_WR_DATA16(y2);
		TFT_WR_REG16(0x002c);//储存器写
	}
	else
	{
		TFT_WR_REG16(0x2a);//列地址设置
		TFT_WR_DATA16(x1);
		TFT_WR_DATA16(x2);
		TFT_WR_REG16(0x2b);//行地址设置
		TFT_WR_DATA16(y1);
		TFT_WR_DATA16(y2);
		TFT_WR_REG16(0x2c);//储存器写
	}    
}
void TFT_Init(void)
{
	//初始化TFT屏幕引脚
    TFT_GPIO_Init();
	//初始化SPI0引脚
    Init_SPI0_GPIO();
    
    DL_GPIO_clearPins(GPIO_LCD_RES_PORT, GPIO_LCD_RES_PIN);//拉低复位信号 
    delay_1ms(100);
    DL_GPIO_setPins(GPIO_LCD_RES_PORT, GPIO_LCD_RES_PIN); //复位完成
    delay_1ms(100);
    
    DL_GPIO_setPins(GPIO_LCD_BLK_PORT, GPIO_LCD_BLK_PIN); //打开背光   
    delay_1ms(100);

	//************* Start Initial Sequence **********//
	TFT_WR_REG(0x11); //Sleep out 
	delay_1ms(120);              //Delay 120ms 
	//------------------------------------ST7735S Frame Rate-----------------------------------------// 
	TFT_WR_REG(0xB1); 
	TFT_WR_DATA8(0x05); 
	TFT_WR_DATA8(0x3C); 
	TFT_WR_DATA8(0x3C); 
	TFT_WR_REG(0xB2); 
	TFT_WR_DATA8(0x05);
	TFT_WR_DATA8(0x3C); 
	TFT_WR_DATA8(0x3C); 
	TFT_WR_REG(0xB3); 
	TFT_WR_DATA8(0x05); 
	TFT_WR_DATA8(0x3C); 
	TFT_WR_DATA8(0x3C); 
	TFT_WR_DATA8(0x05); 
	TFT_WR_DATA8(0x3C); 
	TFT_WR_DATA8(0x3C); 
	//------------------------------------End ST7735S Frame Rate---------------------------------// 
	TFT_WR_REG(0xB4); //Dot inversion 
	TFT_WR_DATA8(0x03); 
	//------------------------------------ST7735S Power Sequence---------------------------------// 
	TFT_WR_REG(0xC0); 
	TFT_WR_DATA8(0x28); 
	TFT_WR_DATA8(0x08); 
	TFT_WR_DATA8(0x04); 
	TFT_WR_REG(0xC1); 
	TFT_WR_DATA8(0XC0); 
	TFT_WR_REG(0xC2); 
	TFT_WR_DATA8(0x0D); 
	TFT_WR_DATA8(0x00); 
	TFT_WR_REG(0xC3); 
	TFT_WR_DATA8(0x8D); 
	TFT_WR_DATA8(0x2A); 
	TFT_WR_REG(0xC4); 
	TFT_WR_DATA8(0x8D); 
	TFT_WR_DATA8(0xEE); 
	//---------------------------------End ST7735S Power Sequence-------------------------------------// 
	TFT_WR_REG(0xC5); //VCOM 
	TFT_WR_DATA8(0x1A); 
	TFT_WR_REG(0x36); //MX, MY, RGB mode 
	if(USE_HORIZONTAL==0){
        TFT_WR_DATA8(0x00);
    }
	else if(USE_HORIZONTAL==1){
        TFT_WR_DATA8(0xC0);
    }
	else if(USE_HORIZONTAL==2){
        TFT_WR_DATA8(0x70);
    }
	else {
        TFT_WR_DATA8(0xA0); 
    }
	//------------------------------------ST7735S Gamma Sequence---------------------------------// 
	TFT_WR_REG(0xE0); 
	TFT_WR_DATA8(0x04); 
	TFT_WR_DATA8(0x22); 
	TFT_WR_DATA8(0x07); 
	TFT_WR_DATA8(0x0A); 
	TFT_WR_DATA8(0x2E); 
	TFT_WR_DATA8(0x30); 
	TFT_WR_DATA8(0x25); 
	TFT_WR_DATA8(0x2A); 
	TFT_WR_DATA8(0x28); 
	TFT_WR_DATA8(0x26); 
	TFT_WR_DATA8(0x2E); 
	TFT_WR_DATA8(0x3A); 
	TFT_WR_DATA8(0x00); 
	TFT_WR_DATA8(0x01); 
	TFT_WR_DATA8(0x03); 
	TFT_WR_DATA8(0x13); 
	TFT_WR_REG(0xE1); 
	TFT_WR_DATA8(0x04); 
	TFT_WR_DATA8(0x16); 
	TFT_WR_DATA8(0x06); 
	TFT_WR_DATA8(0x0D); 
	TFT_WR_DATA8(0x2D); 
	TFT_WR_DATA8(0x26); 
	TFT_WR_DATA8(0x23); 
	TFT_WR_DATA8(0x27); 
	TFT_WR_DATA8(0x27); 
	TFT_WR_DATA8(0x25); 
	TFT_WR_DATA8(0x2D); 
	TFT_WR_DATA8(0x3B); 
	TFT_WR_DATA8(0x00); 
	TFT_WR_DATA8(0x01); 
	TFT_WR_DATA8(0x04); 
	TFT_WR_DATA8(0x13); 
	//------------------------------------End ST7735S Gamma Sequence-----------------------------// 
	TFT_WR_REG(0x3A); //65k mode 
	TFT_WR_DATA8(0x05); 
	TFT_WR_REG(0x29); //Display on 
    
    //TFT_Fill_8(0,0,LCD_W,LCD_H,0x001F);
    //Init_SPI0_GPIO16();  
}

