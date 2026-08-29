#ifndef ECG_ACQ_TASK_H
#define ECG_ACQ_TASK_H

#include "main.h"
#include "ecg_acq_core.h"
#include "ecg_monitor_ui.h"

#define ECG_ACQ_WINDOW_MIN_SECONDS  2U
#define ECG_ACQ_WINDOW_MAX_SECONDS 10U

void ECGAcq_Init(void);
void ECGAcq_Start(void);
void ECGAcq_Stop(void);
void ECGAcq_TimerTick1ms(void);
void ECGAcq_StaticUI(void);
void ECGAcq_ShowUI(void);
void ECGAcq_KeyHandle(uint16_t key_pin, uint8_t key_state);
void ECGAcq_Rotate(int8_t direction);
uint8_t ECGAcq_IsActive(void);
uint8_t ECGAcq_IsRunning(void);
uint8_t ECGAcq_GetGain(void);
uint8_t ECGAcq_GetWindowSeconds(void);
ecg_monitor_page_t ECGAcq_GetPage(void);
uint16_t ECGAcq_GetBpm(void);
uint16_t ECGAcq_GetRrMs(void);
uint16_t ECGAcq_GetRmssdMs(void);
ecg_signal_quality_t ECGAcq_GetQuality(void);

#endif
