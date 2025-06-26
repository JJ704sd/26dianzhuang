#include "hw_lcdinit.h"
#include "systick.h"

/*
*   函数内容：TFT发送单个字节数据
*   函数参数：无
*   返回值：无
*/
void TFT_WR_DATA8(uint8_t data)
{
    gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,RESET);   	//拉低片选信号
    
		while(spi_i2s_flag_get(SPI0,SPI_FLAG_TBE) == RESET);	//等待发送缓冲区空闲
    spi_i2s_data_transmit(SPI0, data);  									//发送数据
		
    gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,SET);     	//拉高片选信号
}

/*
*   函数内容：TFT发送2个字节数据
*   函数参数：无
*   返回值：无
*/
void TFT_WR_DATA(uint16_t data)
{
	uint8_t sendData[2] = {0};
	sendData[0] = (data>>8);
	sendData[1] = (data);
    
	gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,RESET);   //拉低片选信号
   
	while(spi_i2s_flag_get(SPI0,SPI_FLAG_TBE) == RESET);	//等待发送缓冲区空闲
  spi_i2s_data_transmit(SPI0, sendData[0]);  									//发送数据 
	while(spi_i2s_flag_get(SPI0,SPI_FLAG_TBE) == RESET);	//等待发送缓冲区空闲
  spi_i2s_data_transmit(SPI0, sendData[1]);  									//发送数据   
    
	gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,SET);     //拉高片选信号
}

/*
*   函数内容：TFT发送命令数据
*   函数参数：无
*   返回值：无
*/
void TFT_WR_REG(uint8_t reg)
{
	gpio_bit_write(LCD_DC_GPIO_Port,LCD_DC_Pin,RESET);   //拉低命令信号
	gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,RESET);   //拉低片选信号

	while(spi_i2s_flag_get(SPI0,SPI_FLAG_TBE) == RESET);	//等待发送缓冲区空闲
  spi_i2s_data_transmit(SPI0, reg);  									//发送数据

	gpio_bit_write(LCD_DC_GPIO_Port,LCD_DC_Pin,SET);     //拉高命令信号
	gpio_bit_write(LCD_CS_GPIO_Port,LCD_CS_Pin,SET);     //拉高片选信号
}

void TFT_Address_Set(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
	TFT_WR_REG(0x2a);//列地址设置
	TFT_WR_DATA(x1);
	TFT_WR_DATA(x2);
	TFT_WR_REG(0x2b);//行地址设置
	TFT_WR_DATA(y1);
	TFT_WR_DATA(y2);
	TFT_WR_REG(0x2c);//储存器写
}

void TFT_Init(void)
{
	gpio_bit_write(LCD_RES_GPIO_Port,LCD_RES_Pin,RESET);  	//复位
	delay_1ms(100);
	gpio_bit_write(LCD_RES_GPIO_Port,LCD_RES_Pin,SET);     //复位完成
	delay_1ms(100);
	
	gpio_bit_write(LCD_BLK_GPIO_Port,LCD_BLK_Pin,SET);     //打开背光
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
}

