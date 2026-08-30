#ifndef ECG_MONITOR_UI_H
#define ECG_MONITOR_UI_H

#include <stdint.h>

#include "ecg_acq_core.h"

typedef enum
{
    ECG_MONITOR_PAGE = 0,
    ECG_WAVE_PAGE
} ecg_monitor_page_t;

typedef struct
{
    ecg_monitor_page_t page;
    uint16_t bpm;
    uint16_t rr_ms;
    uint16_t rmssd_ms;
    ecg_signal_quality_t quality;
    uint8_t running;
    uint8_t gain;
    uint8_t window_seconds;
    uint8_t fit_limited;
    uint8_t event_marker_valid;
    uint16_t event_marker_x;
    uint32_t waveform_revision;
} ecg_monitor_view_t;

/* Shared plot geometry. Prepared plot_y values must use these bounds. */
#define ECG_MONITOR_PLOT_X0           2U
#define ECG_MONITOR_PLOT_X1         103U
#define ECG_MONITOR_PLOT_Y0          18U
#define ECG_MONITOR_PLOT_Y1          78U
#define ECG_MONITOR_PLOT_CENTER_Y     48
#define ECG_MONITOR_PLOT_HALF_HEIGHT  28

#define ECG_WAVE_PLOT_X0              2U
#define ECG_WAVE_PLOT_X1            157U
#define ECG_WAVE_PLOT_Y0             17U
#define ECG_WAVE_PLOT_Y1            109U
#define ECG_WAVE_PLOT_CENTER_Y        63
#define ECG_WAVE_PLOT_HALF_HEIGHT     44

void ECGMonitorUI_DrawStatic(ecg_monitor_page_t page);
void ECGMonitorUI_Render(const ecg_monitor_view_t *view,
                         const int16_t *plot_y,
                         uint16_t plot_count);

#endif
