#include "ti/driverlib/m0p/dl_core.h"
#include "ti_msp_dl_config.h"
#include "uart.h"
#include "adc.h"
#include "tft_init.h"
#include "tft.h"
#include "freq.h"
#include "pwm.h"
#include "key.h"


volatile struct Oscilloscope oscilloscope={0};

void Init_Oscilloscope(volatile struct Oscilloscope *value);

int main(void)
{
    uint16_t i=0;
    
    //中间值
    float median=0;
    
    //最小值
    float min = 9999;
	
    //显示值
    float voltage=0;
    
    //触发电压值
    float max_data=1.0f;
    
    //波形放大倍数
    float gainFactor=0;
	
    float adcValue = 0;
    
    //触发沿标记
    uint16_t Trigger_number=0;
    
    //初始化示波器参数
    Init_Oscilloscope(&oscilloscope);

    SYSCFG_DL_init();
 
    //初始化TFT屏幕引脚及默认配置
    TFT_Init();
    //TFT显示白色填充区域
    TFT_Fill(0,0,LCD_W,LCD_H,BLACK);

    Init_USART(9600);

    Init_ADC();
    ADC_DMA_Init();

    //初始化PWM输出
    Init_PWM_Output(oscilloscope.timerPeriod-1,oscilloscope.pwmOut);

    //初始化EC11引脚
    Init_EC11_GPIO();
    
    //初始化按键引脚
    Init_Key_GPIO();
    
    //初始化频率定时器2
    Init_FreqTimer();
    
    
    //初始化静态UI
    TFT_StaticUI();


    while (1) 
    {
        //按键扫描
        //Key_Sacnf(&oscilloscope);
        //按键处理
        Key_Handle(&oscilloscope);
        
        //如果获取电压值完成，开始刷屏
        if(oscilloscope.showbit==1)
        {           
            DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);
            DL_ADC12_stopConversion(ADC12_0_INST);

            oscilloscope.showbit=0;
            oscilloscope.vpp=0;
            min = 9999;
            //转换电压值

            for(i=0;i<300;i++)
            {
                adcValue = (Get_ADC_Value(i)*3.3f)/4095.0f;
                //假设Vdac=1.642V，Vin=3.3V，Vout=adcValue
                //( (3*Vdac) /2) - (Vin/2) = Vout
                //根据已知条件，将等式转换为:
                //Vin = (((3*Vdac) /2) - Vout) * 2
                //简化得到公示:
                //Vin = (2.463 - adcValue) * 2
                oscilloscope.voltageValue[i] = (2.463 - adcValue) * 2.0;		
                //寻找峰峰值
                if((oscilloscope.vpp) < oscilloscope.voltageValue[i])
                {
                    oscilloscope.vpp = oscilloscope.voltageValue[i];
                }
                //寻找最小值
                if(min > oscilloscope.voltageValue[i])
                {
                    min = oscilloscope.voltageValue[i];
                }
                //如果峰峰值小于0.3V，则说明没有输入信号
                if(oscilloscope.vpp <= 0.3)
                {
                    oscilloscope.gatherFreq=0;
                }
            }
            
            //刷屏的同时获取电压值
            ADC_DMA_Init();
            
            
            //找到起始显示波形值
            for(i=0;i<200;i++)
            {
                if(oscilloscope.voltageValue[i] < max_data)
                {
                    for(;i<200;i++)
                    {
                        if(oscilloscope.voltageValue[i] > max_data)
                        {
                            Trigger_number=i;
                            break;
                        }
                    }
                    break;
                }
            }
            
            //如果幅值过小，会出现放大倍数过大导致波形显示异常的问题
            if(oscilloscope.vpp > 0.3)
            {
                //获取中间值,如果有负压，则需要先抬升为正压
                if(min < -0.3){
                    median = oscilloscope.vpp;
                }
                else{
                    median = oscilloscope.vpp / 2.0f;
                }
							
                //放大倍数，需要确定放大之后的区间，我将波形固定显示在（18.75~41.25中），(41.25-18.75)/2=11.25f
                gainFactor = 11.25f/median;
              
            }
            
            //依次显示后续100个数据，这样可以防止波形滚动
            for(i=Trigger_number;i<Trigger_number+100;i++)
            {
                KEYD_SCAN(&oscilloscope);
                if(oscilloscope.keyValue == KEYDPRESS)
                {
                    oscilloscope.keyValue=0;
                    do
                    {
                        KEYD_SCAN(&oscilloscope);
                        if(oscilloscope.keyValue == KEYDPRESS){
                            oscilloscope.keyValue=0;
                            break;
                        }
                    }while(1);
                }
                if(min < -0.3){
                    voltage = oscilloscope.voltageValue[i] + oscilloscope.vpp;
                }
                else{
                    voltage = oscilloscope.voltageValue[i];
                }
                if(voltage >= median)
                {
                    voltage = 30 - (voltage - median)*gainFactor;
                }
                else
                {
                    voltage = 30 + (median - voltage)*gainFactor;
                }
                drawCurve(80,voltage);
            }          
        }        
        //参数显示UI
        TFT_ShowUI(&oscilloscope); 
    }
}

/*
*   函数内容：初始化示波器参数结构体
*   函数参数：volatile struct Oscilloscope *value--示波器参数结构体指针
*   返回值：无
*/
void Init_Oscilloscope(volatile struct Oscilloscope *value)
{
    (*value).showbit    =0;                         //清除显示标志位
    (*value).sampletime =3;                         //adc采样周期
    (*value).keyValue   =0;                         //清楚按键值
    (*value).ouptputbit =1;                         //输出标志位
    (*value).gatherFreq =0;                         //采集频率
    (*value).outputFreq =1000;                      //输出频率
    (*value).pwmOut     =16000;                       //PWM引脚输出的PWM占空比
    (*value).timerPeriod=32000;                      //PWM输出定时器周期
    (*value).vpp        =0.0f;                      //峰峰值
}

