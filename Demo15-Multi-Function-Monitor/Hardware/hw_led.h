#ifndef __HW_LED_H
#define __HW_LED_H

#include "main.h"

//led灯类
struct led_class
{
	uint8_t led_state;			//LED的状态
	uint8_t led_on_level;		//打开LED的电平
	uint32_t led_pin;				//LED的引脚
	uint32_t led_gpio;			//LED的端口
};

#define LED_ON 	0x01
#define LED_OFF 0x02

//面向对象封装，变成一个个实例
void led_turn_on(struct led_class *led);
void led_turn_off(struct led_class *led);
void led_toggle(struct led_class *led);
struct led_class led_init(uint32_t led_gpio,uint32_t led_pin,uint8_t led_on_level);

//不做封装，直接根据引脚来
void led_turn_on_all(void);
void led_turn_off_all(void);
void led_toggle_all(void);

#endif

