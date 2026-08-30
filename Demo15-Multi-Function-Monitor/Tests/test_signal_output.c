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

static void check_adc_can_reconstruct_pwm_envelope(void)
{
    uint32_t millisecond;
    uint32_t sample;
    uint32_t sum = 0U;
    uint16_t window_count = 0U;
    uint16_t minimum = 4095U;
    uint16_t maximum = 0U;

    /* TIMER0 samples every 50 us. Each displayed ECG sample averages 80
     * conversions. A carrier locked to the same 50 us interval always gets
     * sampled at one PWM phase and cannot reconstruct duty. */
    for(millisecond = 0U; millisecond < 1000U; millisecond++){
        SignalOutput_Tick1ms();
        for(sample = 0U; sample < 20U; sample++){
            const uint32_t timer_tick = millisecond * 1000U + sample * 50U;
            const uint16_t raw = ((timer_tick % period) < duty) ? 4095U : 0U;
            sum += raw;
            window_count++;
            if(window_count == 80U){
                const uint16_t average = (uint16_t)(sum / 80U);
                if(average < minimum){minimum = average;}
                if(average > maximum){maximum = average;}
                sum = 0U;
                window_count = 0U;
            }
        }
    }
    CHECK(minimum > 100U);
    CHECK(maximum < 4000U);
    CHECK((uint16_t)(maximum - minimum) > 500U);
}

int main(void){
    unsigned i; uint16_t min=65535U,max=0U; uint16_t first_cycle[100];
    SignalOutput_Init(); CHECK(SignalOutput_GetMode()==SIGNAL_OUTPUT_SQUARE); CHECK(state==PWM_OFF);
    SignalOutput_SetEnabled(1U); CHECK(period==1000U); CHECK(duty==500U); CHECK(state==PWM_ON);
    SignalOutput_NextMode(); CHECK(SignalOutput_GetMode()==SIGNAL_OUTPUT_SINE); CHECK(period==59U);
    for(i=0U;i<100U;i++){SignalOutput_Tick1ms();if(duty<min)min=duty;if(duty>max)max=duty;}
    CHECK((uint32_t)(max-min)>30U); CHECK(SignalOutput_GetValue()==100U);
    SignalOutput_SelectEcg(); CHECK(SignalOutput_GetMode()==SIGNAL_OUTPUT_ECG);
    CHECK(SignalOutput_GetValue()==60U); CHECK(SignalOutput_GetEcgPeriodMs()==1000U);
    CHECK(period==59U);
    check_adc_can_reconstruct_pwm_envelope();
    min=65535U; max=0U;
    for(i=0U;i<1100U;i++){
        SignalOutput_Tick1ms();
        if(i<100U){first_cycle[i]=duty;}
        if((i>=1000U)&&(i<1100U)){CHECK(duty==first_cycle[i-1000U]);}
        if(duty<min){min=duty;}
        if(duty>max){max=duty;}
    }
    CHECK(min>=6U); CHECK(max<=53U); CHECK((uint16_t)(max-min)>=35U);
    SignalOutput_ToggleEcgPreset(); CHECK(SignalOutput_GetValue()==80U);
    CHECK(SignalOutput_GetEcgPeriodMs()==750U);
    for(i=0U;i<850U;i++){
        SignalOutput_Tick1ms();
        if(i<100U){first_cycle[i]=duty;}
        if((i>=750U)&&(i<850U)){CHECK(duty==first_cycle[i-750U]);}
    }
    SignalOutput_SetEnabled(0U); CHECK(state==PWM_OFF);
    max=duty; for(i=0U;i<10U;i++){SignalOutput_Tick1ms();} CHECK(duty==max);
    puts("signal output tests passed"); return EXIT_SUCCESS;
}
