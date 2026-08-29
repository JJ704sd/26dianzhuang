#ifndef SIGNAL_GEN_TASK_H
#define SIGNAL_GEN_TASK_H

#include "main.h"

#define SIGNAL_GEN_FREQUENCY_MIN_HZ 20UL
#define SIGNAL_GEN_FREQUENCY_MAX_HZ 20000UL
#define SIGNAL_GEN_DUTY_MIN_PERCENT  5U
#define SIGNAL_GEN_DUTY_MAX_PERCENT  95U

void SignalGen_Init(void);
void SignalGen_Start(void);
void SignalGen_Stop(void);
void SignalGen_StaticUI(void);
void SignalGen_ShowUI(void);
void SignalGen_KeyHandle(uint16_t key_pin, uint8_t key_state);
void SignalGen_Rotate(int8_t direction);
void SignalGen_SetFrequencyHz(uint32_t frequency_hz);
uint32_t SignalGen_GetFrequencyHz(void);
void SignalGen_SetDutyPercent(uint8_t duty_percent);
uint8_t SignalGen_GetDutyPercent(void);
uint8_t SignalGen_IsOutputEnabled(void);
uint8_t SignalGen_IsActive(void);

#endif
