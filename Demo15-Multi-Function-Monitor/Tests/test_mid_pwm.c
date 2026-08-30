#include "mid_pwm.h"
#include <stdio.h>
#include <stdlib.h>

static uint16_t arr;
static uint16_t compare;
static uint16_t counter = 7U;
static uint8_t running;
void timer_channel_output_pulse_value_config(uint32_t t,uint16_t c,uint16_t p){(void)t;(void)c;compare=p;}
void timer_autoreload_value_config(uint32_t t,uint16_t r){(void)t;arr=r;}
void timer_counter_value_config(uint32_t t,uint16_t c){(void)t;counter=c;}
void timer_event_software_generate(uint32_t t,uint16_t e){(void)t;(void)e;}
void timer_enable(uint32_t t){(void)t;running=1U;}
void timer_disable(uint32_t t){(void)t;running=0U;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL: %s\n",#x);exit(EXIT_FAILURE);}}while(0)
int main(void){
    set_pwm_period(1000U); CHECK(arr==999U); CHECK(get_pwm_out_freq()==1000U);
    set_pwm_duty(1200U); CHECK(compare==1000U);
    set_pwm_state(PWM_ON); CHECK(running==1U); CHECK(counter==0U);
    set_pwm_state(PWM_OFF); CHECK(running==0U);
    puts("PWM tests passed"); return EXIT_SUCCESS;
}
