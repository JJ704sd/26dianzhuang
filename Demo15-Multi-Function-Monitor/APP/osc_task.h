#ifndef __OSC_TASK_H
#define __OSC_TASK_H

#include "main.h"
#include "scope_view.h"

#define OSC_PAUSE 0x01U
#define OSC_RUN   0x02U

#define ECG_TIMEBASE_COUNT 4U
#define SCOPE_TIMEBASE_COUNT 9U

typedef enum
{
    DEMO15_MODE_OSCILLOSCOPE = 0,
    DEMO15_MODE_ECG_MONITOR,
    DEMO15_MODE_SPO2_MONITOR,
    DEMO15_MODE_COUNT
} demo15_mode_t;

void ECG_Init(uint16_t vref_value);
void ECG_AcquisitionStart(void);

void Demo15_SelectNextMode(void);
demo15_mode_t Demo15_GetMode(void);

void TFT_StaticUI(void);
void TFT_ShowUI(void);
void osc_waveShow(uint16_t vref_value);
void key_scanf_handle(const uint16_t key_pin, const uint8_t key_state);

void set_ecg_timebase_index(uint8_t value);
uint8_t get_ecg_timebase_index(void);
void set_scope_timebase_index(uint8_t value);
uint8_t get_scope_timebase_index(void);
void toggle_scope_small_signal(void);
uint8_t get_scope_small_signal(void);
void cycle_scope_view_mode(void);
scope_view_mode_t get_scope_view_mode(void);
void set_osc_stop_bit(uint8_t value);
uint8_t get_osc_stop_bit(void);

#endif
