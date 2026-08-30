#ifndef __HW_EC11_H
#define __HW_EC11_H

#include "main.h"

struct ec11_class
{
	uint32_t ec11_Agpio;		//EC11的A端口
	uint32_t ec11_Apin;					//EC11的A引脚
	uint32_t ec11_Bgpio;		//EC11的B端口
	uint32_t ec11_Bpin;					//EC11的B引脚
	uint8_t ec11_direction;			//EC11的方向
};

#define ec11_forward 	0x01		//正转
#define ec11_reversal 0x02		//反转
#define ec11_static		0x03		//禁止

struct ec11_class ec11_init(uint32_t ec11_Agpio,uint32_t ec11_Apin,uint32_t ec11_Bgpio,uint32_t ec11_Bpin);
void ec11_exti_callback(struct ec11_class *handle);

#endif

