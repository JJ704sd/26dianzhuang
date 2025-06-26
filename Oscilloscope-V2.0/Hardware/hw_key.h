#ifndef __HW_KEY_H
#define __HW_KEY_H

#include "main.h"

//公共宏定义

#define DOUBLE_TIME 40  	//400ms内按下两次算双击
#define KEY_NoPress 0xFF	
#define KEY_ERROR   0x00
#define KEY_OK      0x01

/*------- 定义按键类使用 -- */

#define KeyPress 0x10
#define KeyDoublePress 0x20 
#define KeyLongPress  0x30

struct key_class
{
	uint32_t key_gpio;				//KEY的端口
	uint32_t key_pin;					//KEY的引脚
	uint8_t key_state;				//KEY的状态
	uint8_t key_press_level;	//按下按键的电平
	uint8_t keyCnt;						//用于按键计时消抖
	uint8_t keyFcnt; 					//用于判断按键多久间隔算双击
	uint8_t keyCount; 		   	//按键按下的次数
	uint8_t keyLongFlag; 			//按键长按标志位，只有初级进入时判断长按才有效
};

/*------- 需要同时扫描全部按键时使用 -- */
enum KEYNum
{
    KEY1 = 0,
    KEY2,
		KEY3,
		KEY_NUM
};

#define GET_KEY1_IN        gpio_input_bit_get(KEY1_GPIO_Port,KEY1_Pin)
#define GET_KEY2_IN        gpio_input_bit_get(KEY2_GPIO_Port,KEY2_Pin)
#define GET_KEY3_IN        gpio_input_bit_get(KEY3_GPIO_Port,KEY3_Pin)

#define Key1Press 0x10
#define Key2Press 0x11
#define Key3Press 0x12

#define Key1DoublePress 0x20
#define Key2DoublePress 0x21
#define Key3DoublePress 0x22

#define Key1LongPress  0x30
#define Key2LongPress  0x31
#define Key3LongPress  0x32

/*------- 需要同时扫描全部按键时使用 -- */

uint8_t key_scanf_all(void);
void key_scanf(struct key_class *key_handle);
struct key_class key_init(uint32_t key_gpio,uint32_t key_pin,uint8_t key_press_level);

#endif

