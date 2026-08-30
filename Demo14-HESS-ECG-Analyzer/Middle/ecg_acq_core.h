#ifndef ECG_ACQ_CORE_H
#define ECG_ACQ_CORE_H

#include <stdint.h>

#define ECG_ACQ_SAMPLE_RATE_HZ       250U
#define ECG_ACQ_RR_HISTORY_COUNT       8U
#define ECG_DISPLAY_SAMPLE_RATE_HZ  40000U
#define ECG_DISPLAY_WAVE_WINDOW_MS      5U
#define ECG_DISPLAY_WAVE_WINDOW_SAMPLES \
    ((ECG_DISPLAY_SAMPLE_RATE_HZ * ECG_DISPLAY_WAVE_WINDOW_MS) / 1000U)

typedef enum
{
    ECG_SIGNAL_WAIT = 0,
    ECG_SIGNAL_GOOD,
    ECG_SIGNAL_POOR,
    ECG_SIGNAL_LEAD_OFF,
    ECG_SIGNAL_CLIPPED
} ecg_signal_quality_t;

typedef struct
{
    int32_t baseline_q8;
    int32_t smooth_q8;
    uint32_t sample_count;
    uint32_t last_peak_sample;
    uint32_t refractory_samples;
    uint32_t noise_q8;
    uint16_t rr_ms[ECG_ACQ_RR_HISTORY_COUNT];
    uint8_t rr_count;
    uint8_t rr_write;
    uint16_t bpm;
    uint16_t latest_rr_ms;
    uint16_t rmssd_ms;
    uint16_t window_min;
    uint16_t window_max;
    uint16_t rail_count;
    uint16_t window_count;
    ecg_signal_quality_t quality;
} ecg_acq_core_t;

typedef struct
{
    int16_t filtered;
    uint8_t r_peak;
    uint16_t bpm;
    uint16_t rr_ms;
    uint16_t rmssd_ms;
    ecg_signal_quality_t quality;
} ecg_acq_result_t;

void ECGAcqCore_Init(ecg_acq_core_t *core);
ecg_acq_result_t ECGAcqCore_Process(ecg_acq_core_t *core, uint16_t adc_sample);
int8_t ECGAcqCore_DisplaySample(uint16_t adc_sample);
uint8_t ECGAcqCore_MapDisplaySamples(const int8_t *samples,
                                     uint16_t sample_count,
                                     uint8_t requested_gain,
                                     uint8_t rising_trigger,
                                     int16_t plot_y0,
                                     int16_t plot_y1,
                                     int16_t plot_center_y,
                                     int16_t plot_half_height,
                                     int16_t *plot_y,
                                     uint16_t plot_count);
const char *ECGAcqCore_QualityText(ecg_signal_quality_t quality);

#endif
