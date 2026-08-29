#ifndef MAIN_H
#define MAIN_H

#include "gd32e23x.h"

#define LED1_GPIO_Port 	GPIOC
#define LED1_Pin 				GPIO_PIN_14
#define LED2_GPIO_Port 	GPIOC
#define LED2_Pin 				GPIO_PIN_15

#define KEY1_Pin 				GPIO_PIN_13
#define KEY1_GPIO_Port 	GPIOB
#define KEY2_Pin 				GPIO_PIN_14
#define KEY2_GPIO_Port 	GPIOB
#define KEY3_Pin 				GPIO_PIN_15
#define KEY3_GPIO_Port 	GPIOB
#define KEYD_Pin 				GPIO_PIN_9
#define KEYD_GPIO_Port 	GPIOB
#define KEYB_Pin 				GPIO_PIN_3
#define KEYB_GPIO_Port 	GPIOB
#define KEYA_Pin 				GPIO_PIN_4
#define KEYA_GPIO_Port 	GPIOB

#define LCD_RES_Pin 				GPIO_PIN_5
#define LCD_RES_GPIO_Port 	GPIOB
#define LCD_DC_Pin 					GPIO_PIN_6
#define LCD_DC_GPIO_Port 		GPIOB
#define LCD_CS_Pin 					GPIO_PIN_7
#define LCD_CS_GPIO_Port 		GPIOB
#define LCD_BLK_Pin 				GPIO_PIN_8
#define LCD_BLK_GPIO_Port 	GPIOB

#endif /* MAIN_H */
