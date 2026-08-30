#include "hess_analyzer.h"

#include <stdio.h>

#include "ecg_acq_core.h"
#include "mid_lcd.h"

static ecg_acq_core_t analyzer_core;
static ecg_acq_result_t latest_result;

void HESSAnalyzer_Init(void)
{
    ECGAcqCore_Init(&analyzer_core);
    latest_result.filtered = 0;
    latest_result.r_peak = 0U;
    latest_result.bpm = 0U;
    latest_result.rr_ms = 0U;
    latest_result.rmssd_ms = 0U;
    latest_result.quality = ECG_SIGNAL_WAIT;
}

void HESSAnalyzer_ProcessSample(uint16_t adc_sample)
{
    latest_result = ECGAcqCore_Process(&analyzer_core, adc_sample);
}

uint8_t HESSAnalyzer_HasNewPeak(void)
{
    return latest_result.r_peak;
}

uint16_t HESSAnalyzer_GetBpm(void)
{
    return latest_result.bpm;
}

uint16_t HESSAnalyzer_GetRrMs(void)
{
    return latest_result.rr_ms;
}

void HESSAnalyzer_DrawStatic(void)
{
    TFT_Fill(0U, 0U, 160U, 128U, BLACK);
    TFT_ShowString(2U, 0U, (const uint8_t *)"HESS", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(80U, 0U, (const uint8_t *)"TB", WHITE, BLACK, 16U, 0U);

    TFT_DrawLine(0U, 16U, 159U, 16U, CYAN);
    TFT_DrawLine(124U, 16U, 124U, 95U, CYAN);
    TFT_DrawLine(0U, 96U, 159U, 96U, CYAN);

    TFT_ShowString(128U, 18U, (const uint8_t *)"BPM", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 50U, (const uint8_t *)"RR", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(2U, 96U, (const uint8_t *)"RMSSD", WHITE, BLACK, 16U, 0U);
    TFT_ShowString(80U, 96U, (const uint8_t *)"QUALITY", WHITE, BLACK, 16U, 0U);
}

void HESSAnalyzer_ShowUI(uint8_t running, const char *timebase_label)
{
    char text[12];

    TFT_ShowString(44U, 0U,
                   (const uint8_t *)((running != 0U) ? "RUN " : "HOLD"),
                   BLACK, (running != 0U) ? GREEN : YELLOW, 16U, 0U);
    TFT_ShowString(100U, 0U, (const uint8_t *)timebase_label,
                   YELLOW, BLACK, 16U, 0U);

    if (latest_result.bpm != 0U)
    {
        sprintf(text, "%3u", (unsigned int)latest_result.bpm);
    }
    else
    {
        sprintf(text, "---");
    }
    TFT_ShowString(128U, 34U, (const uint8_t *)text, YELLOW, BLACK, 16U, 0U);

    if (latest_result.rr_ms != 0U)
    {
        sprintf(text, "%4u", (unsigned int)latest_result.rr_ms);
    }
    else
    {
        sprintf(text, "----");
    }
    TFT_ShowString(128U, 66U, (const uint8_t *)text, CYAN, BLACK, 16U, 0U);

    sprintf(text, "%4ums", (unsigned int)latest_result.rmssd_ms);
    TFT_Fill(2U, 112U, 74U, 128U, BLACK);
    TFT_ShowString(2U, 112U, (const uint8_t *)text, GREEN, BLACK, 16U, 0U);
    TFT_Fill(80U, 112U, 160U, 128U, BLACK);
    TFT_ShowString(80U, 112U,
                   (const uint8_t *)ECGAcqCore_QualityText(latest_result.quality),
                   (latest_result.quality == ECG_SIGNAL_GOOD) ? GREEN : YELLOW,
                   BLACK, 16U, 0U);
}
