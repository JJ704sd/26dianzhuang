#ifndef __ECG_TASK_H
#define __ECG_TASK_H

#include "main.h"
#include "ecg_core.h"

#define ECG_DISPLAY_SPAN_MIN 1U
#define ECG_DISPLAY_SPAN_MAX 4U

void ECG_Init(void);
void ECG_Start(void);
void ECG_Stop(void);
void ECG_TimerTick1ms(void);
void ECG_StaticUI(void);
void ECG_ShowUI(void);
void ECG_KeyHandle(uint16_t key_pin, uint8_t key_state);
void ECG_SetDisplaySpan(uint8_t periods);
uint8_t ECG_GetDisplaySpan(void);
uint16_t ECG_GetBpm(void);
uint16_t ECG_GetOutputBpm(void);
ecg_signal_quality_t ECG_GetSignalQuality(void);
uint8_t ECG_GetAlarmFlags(void);
uint8_t ECG_IsActive(void);

#endif
