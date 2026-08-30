#ifndef SIGNAL_OUTPUT_H
#define SIGNAL_OUTPUT_H

#include <stdint.h>

typedef enum
{
    SIGNAL_OUTPUT_SQUARE = 0,
    SIGNAL_OUTPUT_SINE,
    SIGNAL_OUTPUT_ECG
} signal_output_mode_t;

void SignalOutput_Init(void);
void SignalOutput_Tick1ms(void);
void SignalOutput_SetEnabled(uint8_t enabled);
void SignalOutput_NextMode(void);
void SignalOutput_Adjust(int8_t direction);
void SignalOutput_SelectEcg(void);
void SignalOutput_ToggleEcgPreset(void);
void SignalOutput_SetEcgBpm(uint16_t bpm);
signal_output_mode_t SignalOutput_GetMode(void);
uint16_t SignalOutput_GetValue(void);
uint16_t SignalOutput_GetEcgPeriodMs(void);
uint8_t SignalOutput_IsEnabled(void);
const char *SignalOutput_GetModeText(void);

#endif
