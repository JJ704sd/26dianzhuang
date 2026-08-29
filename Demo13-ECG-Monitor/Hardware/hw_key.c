#include "hw_key.h"

/*
 * 函数内容：按键实例注册
 * 函数参数：
							GPIO_TypeDef *key_gpio -- 按键端口
							uint16_t key_pin -- 按键引脚
							key_press_level -- 按键按下时电平
 * 返回值：struct key_class -- 按键句柄
 */
struct key_class key_init(uint32_t key_gpio,uint32_t key_pin,uint8_t key_press_level)
{
	struct key_class key_handle;
	key_handle.key_gpio = key_gpio;
	key_handle.key_pin = key_pin;
	key_handle.key_press_level = key_press_level;
	key_handle.key_state = KEY_NoPress;
	key_handle.keyCnt = 0;
	key_handle.keyCount = 0;
	key_handle.keyFcnt = 0;
	key_handle.keyLongFlag = 0;
	return key_handle;
}

/*
 * 函数内容：扫描单个实例按键 -- 每隔10ms调用一次
 * 函数参数：struct key_class *key_handle -- 按键句柄
 * 返回值：无
 */
void key_scanf(struct key_class *key_handle)
{
	key_handle->key_state = KEY_NoPress;	//默认是没有按键按下
	//如果有按键按下
	if(gpio_input_bit_get(key_handle->key_gpio,key_handle->key_pin) == key_handle->key_press_level)
	{
		key_handle->keyCnt++;						//记录按键按下时间
		if(key_handle->keyCnt >= 120)
		{
			key_handle->keyCnt = 120;			//防止数据越界
		}
	}
	else
	{
		//如果按键抬起
		if((key_handle->keyCnt >= 100) && (key_handle->keyLongFlag == 0)){
			//按键大于1s且之前没有多击，证明是长按
			key_handle->key_state = KeyLongPress;
		}
		else if(key_handle->keyCnt >= 2)
		{
			//按键按下时间大于20ms，证明是一次按下
			key_handle->keyCount++;     			//按键按下的次数增加
			key_handle->keyFcnt = DOUBLE_TIME;   	//在指定时间内再次按下算双击
			key_handle->keyLongFlag = 1;    		//已经有一次短按了，才能等待多击结束后才可长按
		}
		key_handle->keyCnt = 0;
		if(key_handle->keyFcnt)
		{
			key_handle->keyFcnt--;  //间隔时间不断减少
			if(key_handle->keyFcnt <= 0)    
			{
					//间隔时间到了
					if(key_handle->keyCount == 1){
							//单击
							key_handle->key_state = KeyPress;
					}
					else if(key_handle->keyCount == 2){
							//双击
							key_handle->key_state = KeyDoublePress;
					}
					key_handle->keyFcnt = 0;
					key_handle->keyCount = 0;
					key_handle->keyLongFlag = 0;
			}
		}
	}
}

/*
 * 函数内容：按键扫描,全部扫描，需要根据每一个按键进行判断 -- 每隔10ms调用一次
 * 函数参数：无
 * 返回值：判断是哪个按键按下，支持长按、双击还是单击判断，同一时刻仅允许对一个按键判断
 */
uint8_t key_scanf_all(void)
{
    uint8_t ret = KEY_ERROR;    //按键返回值
    static uint8_t keyCnt[3] = {0};  //用于按键计时消抖
    static uint8_t keyFcnt[3] = {0}; //用于判断按键多久间隔算双击
    static uint8_t keyCount[3] = {0};    //按键按下的次数
    static uint8_t keyLongFlag[3] = {0}; //按键长按标志位，只有初级进入时判断长按才有效
    if(GET_KEY1_IN == RESET){   
        keyCnt[KEY1]++;           //计时变量，记录按键按下低电平时间
        if(keyCnt[KEY1] >= 120){
            //防止数据越界
            keyCnt[KEY1] = 120;
        }
    }     
    if(GET_KEY1_IN == SET)
    {
        //如果按键抬起
        if((keyCnt[KEY1] >= 100) && (keyLongFlag[KEY1] == 0)){
            //按键大于1s且之前没有多击，证明是长按
            ret = Key1LongPress;
        }
        else if(keyCnt[KEY1] >= 2)
        {
            //按键按下时间大于20ms，证明是一次按下
            keyCount[KEY1]++;     //按键按下的次数增加
            keyFcnt[KEY1] = DOUBLE_TIME;   //在300ms内再次按下算双击
            keyLongFlag[KEY1] = 1;    //已经有一次短按了，才能等待多击结束后才可长按
        }
        keyCnt[KEY1] = 0;
        if(keyFcnt[KEY1])
        {
            keyFcnt[KEY1]--;  //间隔时间不断减少
            if(keyFcnt[KEY1] <= 0)    
            {
                //间隔时间到了
                if(keyCount[KEY1] == 1){
                    //单击
                    ret = Key1Press;
                }
                else if(keyCount[KEY1] ==2){
                    //双击
                    ret = Key1DoublePress;
                }
                keyFcnt[KEY1] = 0;
                keyCount[KEY1] = 0;
                keyLongFlag[KEY1] = 0;
            }
        }
    }

    if(GET_KEY2_IN == RESET){   
        keyCnt[KEY2]++;           //计时变量，记录按键按下低电平时间
        if(keyCnt[KEY2] >= 120){
            //防止数据越界
            keyCnt[KEY2] = 120;
        }
    }     
    if(GET_KEY2_IN == SET)
    {
        //如果按键抬起
        if((keyCnt[KEY2] >= 100) && (keyLongFlag[KEY2] == 0)){
            //按键大于1s且之前没有多击，证明是长按
            ret = Key2LongPress;
        }
        else if(keyCnt[KEY2] >= 2)
        {
            //按键按下时间大于20ms，证明是一次按下
            keyCount[KEY2]++;     //按键按下的次数增加
            keyFcnt[KEY2] = DOUBLE_TIME;   //在300ms内再次按下算双击
            keyLongFlag[KEY2] = 1;    //已经有一次短按了，才能等待多击结束后才可长按
        }
        keyCnt[KEY2] = 0;
        if(keyFcnt[KEY2])
        {
            keyFcnt[KEY2]--;  //间隔时间不断减少
            if(keyFcnt[KEY2] <= 0)    
            {
                //间隔时间到了
                if(keyCount[KEY2] == 1){
                    //单击
                    ret = Key2Press;
                }
                else if(keyCount[KEY2] ==2){
                    //双击
                    ret = Key2DoublePress;
                }
                keyFcnt[KEY2] = 0;
                keyCount[KEY2] = 0;
                keyLongFlag[KEY2] = 0;
            }
        }
    }
		
		if(GET_KEY3_IN == RESET){   
        keyCnt[KEY3]++;           //计时变量，记录按键按下低电平时间
        if(keyCnt[KEY3] >= 120){
            //防止数据越界
            keyCnt[KEY3] = 120;
        }
    }     
    if(GET_KEY3_IN == SET)
    {
        //如果按键抬起
        if((keyCnt[KEY3] >= 100) && (keyLongFlag[KEY3] == 0)){
            //按键大于1s且之前没有多击，证明是长按
            ret = Key3LongPress;
        }
        else if(keyCnt[KEY3] >= 2)
        {
            //按键按下时间大于20ms，证明是一次按下
            keyCount[KEY3]++;     //按键按下的次数增加
            keyFcnt[KEY3] = DOUBLE_TIME;   //在300ms内再次按下算双击
            keyLongFlag[KEY3] = 1;    //已经有一次短按了，才能等待多击结束后才可长按
        }
        keyCnt[KEY3] = 0;
        if(keyFcnt[KEY3])
        {
            keyFcnt[KEY3]--;  //间隔时间不断减少
            if(keyFcnt[KEY3] <= 0)    
            {
                //间隔时间到了
                if(keyCount[KEY3] == 1){
                    //单击
                    ret = Key3Press;
                }
                else if(keyCount[KEY3] ==2){
                    //双击
                    ret = Key3DoublePress;
                }
                keyFcnt[KEY3] = 0;
                keyCount[KEY3] = 0;
                keyLongFlag[KEY3] = 0;
            }
        }
    }
    return ret;
}

