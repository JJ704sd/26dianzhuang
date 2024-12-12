#include "freq.h"
#include "main.h"

void Init_FreqTimer(void)
{
    NVIC_EnableIRQ(CAPTURE_0_INST_INT_IRQN);
    // DL_TimerG_startCounter(CAPTURE_0_INST);
    //gLoadValue = DL_TimerG_getLoadValue(CAPTURE_0_INST);
}

static __IO uint16_t ccnumber = 0;
static __IO uint32_t freq = 0;
static __IO uint16_t readvalue1 = 0, readvalue2 = 0;
static __IO uint32_t count = 0;

extern volatile struct Oscilloscope oscilloscope;

void Freq_calibration(__IO uint32_t *freq)
{
	if(((*freq) >= 950) && ((*freq) < 1050)){
		(*freq) = 1000;
	}
	else if(((*freq) >= 1050) && ((*freq) < 2050)){
		(*freq) = 2000;
	}
	else if(((*freq) >= 2050) && ((*freq) < 3050)){
		(*freq) = 3000;
	}
	else if(((*freq) >= 3050) && ((*freq) < 4050)){
		(*freq) = 4000;
	}
	else if(((*freq) >= 4050) && ((*freq) < 5050)){
		(*freq) = 5000;
	}
	else if(((*freq) >= 5050) && ((*freq) < 6050)){
		(*freq) = 6000;
	}
	else if(((*freq) >= 6050) && ((*freq) < 7050)){
		(*freq) = 7000;
	}
	else if(((*freq) >= 7050) && ((*freq) < 8050)){
		(*freq) = 8000;
	}
	else if(((*freq) >= 8050) && ((*freq) < 9050)){
		(*freq) = 9000;
	}
	else if(((*freq) >= 9050) && ((*freq) < 10050)){
		(*freq) = 10000;
	}
}

void CAPTURE_0_INST_IRQHandler(void)
{
    if( DL_TimerG_getPendingInterrupt(CAPTURE_0_INST) == DL_TIMERG_IIDX_CC1_UP )
    {
        if(0 == ccnumber)
        {
            // 读第一次通道1捕获值
            readvalue1 = DL_TimerG_getCaptureCompareValue(CAPTURE_0_INST, DL_TIMER_CC_1_INDEX);
            ccnumber = 1;
        }
        else if(1 == ccnumber)
        {
            // 读第2次通道1捕获值 
            readvalue2 = DL_TimerG_getCaptureCompareValue(CAPTURE_0_INST, DL_TIMER_CC_1_INDEX);
            // 如果第二次捕获值大于第一次
            if(readvalue2 > readvalue1)
            {
                count = (readvalue2 - readvalue1); 
            }else
            {
                count = ((0xFFFFU - readvalue1) + readvalue2); 
            }
            //计算频率
            freq = 1000000U / count;     
            Freq_calibration(&freq);
            oscilloscope.gatherFreq = freq; 
            
            readvalue1=0;readvalue2=0;
            count=0;
            freq=0;
            ccnumber = 0;
        }
        DL_TimerG_clearInterruptStatus(CAPTURE_0_INST, DL_TIMER_INTERRUPT_CC1_UP_EVENT);  
    }
}
