#include "ecg_acq_task.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__,          \
                    #condition);                                                \
            exit(EXIT_FAILURE);                                                 \
        }                                                                       \
    } while (0)

static uint16_t adc_sample = 2048U;
static ecg_monitor_view_t last_view;
static ecg_monitor_page_t last_static_page;

uint16_t Get_ADC_Latest(void)
{
    return adc_sample;
}

void ECGMonitorUI_DrawStatic(ecg_monitor_page_t page)
{
    last_static_page = page;
}

void ECGMonitorUI_Render(const ecg_monitor_view_t *view,
                         const int16_t *plot_y,
                         uint16_t plot_count)
{
    CHECK(view != NULL);
    CHECK(plot_y != NULL);
    CHECK(plot_count >= 2U);
    last_view = *view;
}

static void tick_ms(uint16_t count)
{
    uint16_t i;

    for (i = 0U; i < count; ++i)
    {
        ECGAcq_TimerTick1ms();
    }
}

static void test_sampling_freeze_and_controls(void)
{
    ECGAcq_Init();
    ECGAcq_Start();
    CHECK(ECGAcq_IsActive() != 0U);
    CHECK(ECGAcq_IsRunning() != 0U);
    CHECK(ECGAcq_GetGain() == 1U);
    CHECK(ECGAcq_GetWindowSeconds() == 2U);
    CHECK(ECGAcq_GetPage() == ECG_MONITOR_PAGE);

    ECGAcq_ShowUI();
    CHECK(last_view.waveform_revision == 0U);
    tick_ms(3U);
    ECGAcq_ShowUI();
    CHECK(last_view.waveform_revision == 0U);
    adc_sample = 2304U;
    tick_ms(1U);
    ECGAcq_ShowUI();
    CHECK(last_view.waveform_revision == 1U);

    ECGAcq_HandleAction(ECG_ACQ_ACTION_TOGGLE_RUN);
    CHECK(ECGAcq_IsRunning() == 0U);
    tick_ms(8U);
    ECGAcq_ShowUI();
    CHECK(last_view.waveform_revision == 1U);

    ECGAcq_HandleAction(ECG_ACQ_ACTION_CYCLE_GAIN);
    CHECK(ECGAcq_GetGain() == 2U);
    ECGAcq_HandleAction(ECG_ACQ_ACTION_TOGGLE_PAGE);
    CHECK(ECGAcq_GetPage() == ECG_WAVE_PAGE);
    CHECK(last_static_page == ECG_WAVE_PAGE);

    ECGAcq_HandleAction(ECG_ACQ_ACTION_WINDOW_5_SECONDS);
    CHECK(ECGAcq_GetWindowSeconds() == 5U);
    ECGAcq_HandleAction(ECG_ACQ_ACTION_WINDOW_2_SECONDS);
    CHECK(ECGAcq_GetWindowSeconds() == 2U);

    ECGAcq_HandleAction(ECG_ACQ_ACTION_MARK_EVENT);
    ECGAcq_ShowUI();
    CHECK(last_view.event_marker_valid != 0U);
    ECGAcq_HandleAction(ECG_ACQ_ACTION_RESET_MEASUREMENTS);
    ECGAcq_ShowUI();
    CHECK(last_view.waveform_revision == 0U);
    CHECK(last_view.event_marker_valid == 0U);
    CHECK(ECGAcq_GetBpm() == 0U);
    CHECK(ECGAcq_GetRrMs() == 0U);
    CHECK(ECGAcq_GetRmssdMs() == 0U);
}

int main(void)
{
    test_sampling_freeze_and_controls();
    puts("ecg_acq_task behavior tests passed");
    return EXIT_SUCCESS;
}
