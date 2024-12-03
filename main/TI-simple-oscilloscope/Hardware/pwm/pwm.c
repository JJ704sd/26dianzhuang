#include "pwm.h"
#include "adc.h"
#include "main.h"



/*
*		函数内容：初始化定时器，输出PWM信号
*		函数参数：无
*		返回值：	无
*/
void Init_PWM_Output(uint32_t period,uint32_t pulse)
{
    /*
    * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
    * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
    *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
    */
    DL_TimerG_ClockConfig gPWM_0ClockConfig = {
        .clockSel = DL_TIMER_CLOCK_BUSCLK,
        .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
        .prescale = 0U
    };

    DL_TimerG_PWMConfig gPWM_0Config = {
        .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
        .period = period,//周期调整
        .isTimerWithFourCC = false,
        .startTimer = DL_TIMER_START,
    };

    DL_TimerG_setClockConfig(
        PWM_0_INST, (DL_TimerG_ClockConfig *) &gPWM_0ClockConfig);

    DL_TimerG_initPWMMode(
        PWM_0_INST, (DL_TimerG_PWMConfig *) &gPWM_0Config);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(PWM_0_INST,DL_TIMER_CZC_CCCTL1_ZCOND,DL_TIMER_CAC_CCCTL1_ACOND,DL_TIMER_CLC_CCCTL1_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(PWM_0_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_ENABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_0_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    //占空比调整
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, pulse, DL_TIMER_CC_1_INDEX);

    DL_TimerG_enableClock(PWM_0_INST);

    DL_TimerG_setCCPDirection(PWM_0_INST , DL_TIMER_CC1_OUTPUT );
}

void Init_PWM_Output_disable(uint32_t period,uint32_t pulse)
{
    /*
    * Timer clock configuration to be sourced by  / 1 (32000000 Hz)
    * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
    *   32000000 Hz = 32000000 Hz / (1 * (0 + 1))
    */
    DL_TimerG_ClockConfig gPWM_0ClockConfig = {
        .clockSel = DL_TIMER_CLOCK_BUSCLK,
        .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
        .prescale = 0U
    };

    DL_TimerG_PWMConfig gPWM_0Config = {
        .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
        .period = period,//周期调整
        .isTimerWithFourCC = false,
        .startTimer = DL_TIMER_STOP,
    };

    DL_TimerG_setClockConfig(
        PWM_0_INST, (DL_TimerG_ClockConfig *) &gPWM_0ClockConfig);

    DL_TimerG_initPWMMode(
        PWM_0_INST, (DL_TimerG_PWMConfig *) &gPWM_0Config);

    // Set Counter control to the smallest CC index being used
    DL_TimerG_setCounterControl(PWM_0_INST,DL_TIMER_CZC_CCCTL1_ZCOND,DL_TIMER_CAC_CCCTL1_ACOND,DL_TIMER_CLC_CCCTL1_LCOND);

    DL_TimerG_setCaptureCompareOutCtl(PWM_0_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_ENABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERG_CAPTURE_COMPARE_1_INDEX);

    DL_TimerG_setCaptCompUpdateMethod(PWM_0_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERG_CAPTURE_COMPARE_1_INDEX);
    //占空比调整
    DL_TimerG_setCaptureCompareValue(PWM_0_INST, pulse, DL_TIMER_CC_1_INDEX);

    DL_TimerG_disableClock(PWM_0_INST);

    DL_TimerG_setCCPDirection(PWM_0_INST , DL_TIMER_CC1_OUTPUT );
}
/*
*		函数内容：设置PWM占空比
*		函数参数：无
*		返回值：  无
*/
void Set_Output_PWMComparex(uint16_t value)
{
	//timer_channel_output_pulse_value_config(TIMER14, TIMER_CH_0, value);	
    DL_TimerG_setCaptureCompareValue(PWM_0_INST,value,GPIO_PWM_0_C1_IDX);
}

/*
*		函数内容：设置周期，对应设置频率
*		函数参数：无
*		返回值：	无
*/
void Set_Output_Freq(uint32_t value)
{
    //timer_autoreload_value_config(TIMER14,value);
}


