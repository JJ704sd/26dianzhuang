#include "signal_output.h"
#include "mid_pwm.h"
#include <stdio.h>
#include <stdlib.h>
static uint16_t period,duty; static uint8_t state;
void set_pwm_period(uint16_t v){period=v;} uint16_t get_pwm_period(void){return period;}
void set_pwm_duty(uint16_t v){duty=v;} uint16_t get_pwm_duty(void){return duty;}
void set_pwm_state(uint8_t v){state=v;} uint8_t get_pwm_state(void){return state;}
uint16_t get_pwm_out_freq(void){return period?1000000U/period:0U;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL: %s\n",#x);exit(EXIT_FAILURE);}}while(0)
int main(void){
    unsigned i; uint16_t min=65535U,max=0U;
    SignalOutput_Init(); CHECK(SignalOutput_GetMode()==SIGNAL_OUTPUT_SQUARE); CHECK(state==PWM_OFF);
    SignalOutput_SetEnabled(1U); CHECK(period==1000U); CHECK(duty==500U); CHECK(state==PWM_ON);
    SignalOutput_NextMode(); CHECK(SignalOutput_GetMode()==SIGNAL_OUTPUT_SINE); CHECK(period==50U);
    for(i=0U;i<100U;i++){SignalOutput_Tick1ms();if(duty<min)min=duty;if(duty>max)max=duty;}
    CHECK((uint32_t)(max-min)>30U); CHECK(SignalOutput_GetValue()==100U);
    SignalOutput_NextMode(); CHECK(SignalOutput_GetMode()==SIGNAL_OUTPUT_ECG); CHECK(SignalOutput_GetValue()==72U);
    puts("signal output tests passed"); return EXIT_SUCCESS;
}
