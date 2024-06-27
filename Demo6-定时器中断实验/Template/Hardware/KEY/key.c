#include "key.h"
#include "led.h"

/*
*   函数内容：初始化按键GPIO
*   函数参数：无
*   返回值：  无
*/
void Init_Key_GPIO(void)
{
    //使能时钟
    rcu_periph_clock_enable(RCU_GPIOB);
    
    //设置输出模式，上拉
    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
}

/*
*   函数内容：初始化按键GPIO
*   函数参数：无
*   返回值：  uint8_t 按键值
*/
uint8_t Scanf_Key_GPIO(void)
{
    if(gpio_input_bit_get(GPIOB,GPIO_PIN_13)==RESET)
    {
        delay_1ms(10);
        if(gpio_input_bit_get(GPIOB,GPIO_PIN_13)==RESET)
        {
            while(gpio_input_bit_get(GPIOB,GPIO_PIN_13)==RESET);
            return KEY1;
        }
    }
    if(gpio_input_bit_get(GPIOB,GPIO_PIN_15)==RESET)
    {
        delay_1ms(10);
        if(gpio_input_bit_get(GPIOB,GPIO_PIN_15)==RESET)
        {
            while(gpio_input_bit_get(GPIOB,GPIO_PIN_15)==RESET);
            return KEY2;
        }
    }
    if(gpio_input_bit_get(GPIOB,GPIO_PIN_14)==RESET)
    {
        delay_1ms(10);
        if(gpio_input_bit_get(GPIOB,GPIO_PIN_14)==RESET)
        {
            while(gpio_input_bit_get(GPIOB,GPIO_PIN_14)==RESET);
            return KEY3;
        }
    }
    return 0;
}

/*
*   函数内容：按键处理函数
*   函数参数：uint8_t 按键值
*   返回值：  无
*/
void Key_Handle(uint8_t keyValue)
{
    switch(keyValue)
    {
        case KEY1:
            Open_LED(led2);
            break;
        case KEY2:
            Open_LED(led3);
            break;
        case KEY3:
            CLose_LED(led2);
            CLose_LED(led3);
            break;
        default:
            break;
    }
}