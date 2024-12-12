#include "key.h"
#include "pwm.h"
#include "tft.h"
#include "adc.h"
#include "ti_msp_dl_config.h"
#include <ti/driverlib/dl_gpio.h>
#define delay_1ms(X) delay_cycles((CPUCLK_FREQ/1000) *X)
static uint8_t keyValue=0;
static uint8_t key1_state = 0;
static uint8_t key2_state = 0;
static uint8_t key3_state = 0;
/*
*   函数内容：初始化按键GPIO
*   函数参数：无
*   返回值：  无
*/
void Init_Key_GPIO(void)
{
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOA_INT_IRQN);
}

/*
*   函数内容：初始化EC11 GPIO
*   函数参数：无
*   返回值：  无
*/
void Init_EC11_GPIO(void)
{

}

/*
*   函数内容：按键处理函数
*   函数参数：无
*   返回值：  无
*/
void Key_Handle(volatile struct Oscilloscope *value)
{
	uint8_t i=0,j=0;
	float tempValue=0;
	switch((*value).keyValue)
	{
		case KEY1PRESS:
            (*value).pwmOut=((uint32_t)(*value).timerPeriod*0.04f)+(*value).pwmOut;
            if((*value).pwmOut > (*value).timerPeriod)
            {
                (*value).pwmOut = 0;
            }
            Set_Output_PWMComparex((*value).pwmOut);
			break;
        case KEY3PRESS:
            tempValue=(*value).pwmOut/((*value).timerPeriod+0.0f);//记录当前占空比百分比
            (*value).timerPeriod = (*value).timerPeriod/2;//当前PWM频率2分频
            printf("timerPeriod = %d\r\n",(*value).timerPeriod);
            if((*value).timerPeriod <= 1000)
            {
                (*value).timerPeriod = 32000;
            }
            (*value).outputFreq = CPUCLK_FREQ/(*value).timerPeriod;
            (*value).pwmOut = (uint16_t)((*value).timerPeriod*tempValue);//计算占占空比

            if( (*value).ouptputbit == 0 )  //如果是PWM开启的状态
                Init_PWM_Output_disable((*value).timerPeriod-1,(*value).pwmOut);//关闭PWM输出
            else//如果是PWM关闭的状态
                Init_PWM_Output((*value).timerPeriod-1,(*value).pwmOut);//开启PWM输出   
            tempValue=0;
			break;
        case KEY2PRESS:
        printf("key2 down\r\n");
            if((*value).ouptputbit == 0)
            {
                (*value).ouptputbit=1;
                Init_PWM_Output((*value).timerPeriod-1,(*value).pwmOut);//开启PWM输出    
            }
            else
            {
                (*value).ouptputbit=0;
                Init_PWM_Output_disable((*value).timerPeriod-1,(*value).pwmOut);//关闭PWM输出
            }
            break;
        case KEYAPRESS:
            printf("keyA down\r\n");
            (*value).sampletime = (*value).sampletime + 1000;
            if( (*value).sampletime > 10000 ) (*value).sampletime = 1000;

            DL_ADC12_disableConversions(ADC12_0_INST);
            DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
             
            DL_ADC12_setSampleTime0(ADC12_0_INST,(*value).sampletime);    
            
            DL_ADC12_enableConversions(ADC12_0_INST);           
            ADC_DMA_Init();
            printf("sampletime=%d\r\n",(*value).sampletime);
            break;
        case KEYBPRESS:
            printf("keyB down\r\n");
            (*value).sampletime = (*value).sampletime - 1000;
            if( (*value).sampletime < 1000 ) (*value).sampletime = 10000;
            
            DL_ADC12_disableConversions(ADC12_0_INST);
            DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

            DL_ADC12_setSampleTime0(ADC12_0_INST,(*value).sampletime);    
            
            DL_ADC12_enableConversions(ADC12_0_INST);   
            ADC_DMA_Init();
            printf("sampletime=%d\r\n",(*value).sampletime);
            break;
		default:
			break;
	}
    (*value).keyValue=0;
    //参数显示UI
    TFT_ShowUI(value); 
}

void KEYD_SCAN(volatile struct Oscilloscope *value)
{
    if( DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYD_PIN) == 0)
	{
		delay_1ms(20);
		if(DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYD_PIN) == 0)
		{
			while(DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYD_PIN) == 0);
			(*value).keyValue = KEYDPRESS;
		}
	}
}

void Key_Sacnf(volatile struct Oscilloscope *value)
{
	if(key1_state == KEYPRESS){
		delay_1ms(20);
		if(DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_PIN_1_PIN) == 0){
			(*value).keyValue = KEY1PRESS;
			key1_state = NoPRESS;
            
		}
	}
	else{
		key1_state = NoPRESS;
	}
	
	if(key2_state == KEYPRESS){
		delay_1ms(20);
		if(DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_PIN_2_PIN) == 0){
			(*value).keyValue = KEY2PRESS;
			key2_state = NoPRESS;
		}
	}
	else{
		key2_state = NoPRESS;
	}
	
	if(key3_state == KEYPRESS){
		delay_1ms(20);
		if(DL_GPIO_readPins(GPIO_KEY_PORT, GPIO_KEY_PIN_3_PIN) == 0){
			(*value).keyValue = KEY3PRESS;
			key3_state = NoPRESS;
		}
	}
	else{
		key3_state = NoPRESS;
	}
}

extern volatile struct Oscilloscope oscilloscope;

void GROUP1_IRQHandler(void)//Group1的中断服务函数
{
    static uint8_t A_cnt=0;
    static uint8_t B_value=0;

    uint32_t gpioA = DL_GPIO_getEnabledInterruptStatus(GPIOA,
    GPIO_KEY_PIN_1_PIN | GPIO_KEY_PIN_2_PIN | GPIO_KEY_PIN_3_PIN | GPIO_EC11_KEYA_PIN);

    if( (gpioA & GPIO_EC11_KEYA_PIN) == GPIO_EC11_KEYA_PIN ) 
    {
        if( (DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYA_PIN) == 0) &&  (A_cnt == 0) )//A相下降沿触发一次
        {
            A_cnt++;			//计数值加一，表示已经触发了第一次中断
            B_value = 0;	//读取B相电平，若为高电平则B_level置1，反之保持0
            if( DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYB_PIN) != 0 )
            {
                B_value = 1;
            }
        }
        else if( (DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYA_PIN) != 0) &&  (A_cnt == 1) )//A相上升沿触发一次
        {
            A_cnt = 0;
            if((B_value == 1) && (DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYB_PIN) == 0))
            {
                oscilloscope.keyValue=KEYBPRESS; 
            }
            if((B_value == 0) && (DL_GPIO_readPins(GPIO_EC11_PORT, GPIO_EC11_KEYB_PIN) != 0))
            {
                oscilloscope.keyValue=KEYAPRESS;
            }
        }
        DL_GPIO_clearInterruptStatus(GPIO_EC11_PORT, GPIO_EC11_KEYA_PIN);
    }

    if((gpioA & GPIO_KEY_PIN_1_PIN) == GPIO_KEY_PIN_1_PIN)
    {
       key1_state = KEYPRESS;
       oscilloscope.keyValue = KEY1PRESS;
       DL_GPIO_clearInterruptStatus(GPIO_KEY_PORT, GPIO_KEY_PIN_1_PIN);
    }
    if((gpioA & GPIO_KEY_PIN_2_PIN) == GPIO_KEY_PIN_2_PIN)
    {
       key2_state = KEYPRESS;
       oscilloscope.keyValue = KEY2PRESS;
       DL_GPIO_clearInterruptStatus(GPIO_KEY_PORT, GPIO_KEY_PIN_2_PIN);
    }
    if((gpioA & GPIO_KEY_PIN_3_PIN) == GPIO_KEY_PIN_3_PIN)
    {
       key3_state = KEYPRESS;
       oscilloscope.keyValue = KEY3PRESS;
       DL_GPIO_clearInterruptStatus(GPIO_KEY_PORT, GPIO_KEY_PIN_3_PIN);
    }
}
