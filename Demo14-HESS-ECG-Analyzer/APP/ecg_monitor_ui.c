#include "ecg_monitor_ui.h"

#include <stdio.h>

#include "mid_lcd.h"

#define ECG_SCREEN_X_END 160U
#define ECG_SCREEN_Y_END 128U
#define ECG_GRID_MINOR_X  10U
#define ECG_GRID_MINOR_Y   8U
#define ECG_GRID_MAJOR_X  50U
#define ECG_GRID_MAJOR_Y  40U
#define ECG_MAX_PLOT_POINTS \
    (ECG_WAVE_PLOT_X1 - ECG_WAVE_PLOT_X0 + 1U)

static ecg_monitor_view_t previous_view;
static int16_t previous_plot[ECG_MAX_PLOT_POINTS];
static uint16_t plot_scanline[ECG_MAX_PLOT_POINTS];
static uint16_t previous_plot_count;
static uint8_t previous_view_valid;
static uint8_t previous_plot_valid;

/* The TFT Chinese font is indexed by two-byte GBK codes. */
static const uint8_t text_waveform[] = {0xCA,0xBE,0xB2,0xA8,0x00};
static const uint8_t text_input_status[] =
    {0xCA,0xE4,0xC8,0xEB,0xD7,0xB4,0xCC,0xAC,0x00};
static const uint8_t text_status[] = {0xD7,0xB4,0xCC,0xAC,0x00};
static const uint8_t text_amplitude[] = {0xB7,0xF9,0xD6,0xB5,0x00};
static const uint8_t text_frequency[] = {0xC6,0xB5,0xC2,0xCA,0x00};
static const uint8_t text_output[] = {0xCA,0xE4,0xB3,0xF6,0x00};
static const uint8_t text_input[] = {0xCA,0xE4,0xC8,0xEB,0x00};
static const uint8_t text_open_close[] = {0xBF,0xAA,0xB9,0xD8,0x00};

static const char *ecg_monitor_quality_text(ecg_signal_quality_t quality)
{
    switch (quality)
    {
        case ECG_SIGNAL_GOOD:
            return "GOOD";
        case ECG_SIGNAL_POOR:
            return "POOR";
        case ECG_SIGNAL_LEAD_OFF:
            return "LEAD OFF";
        case ECG_SIGNAL_CLIPPED:
            return "CLIPPED";
        case ECG_SIGNAL_WAIT:
        default:
            return "WAIT";
    }
}

static uint16_t ecg_monitor_quality_color(ecg_signal_quality_t quality)
{
    return (quality == ECG_SIGNAL_GOOD) ? GREEN : YELLOW;
}

static int16_t ecg_monitor_clamp_y(int16_t y, uint16_t y0, uint16_t y1)
{
    if (y < (int16_t)y0)
    {
        return (int16_t)y0;
    }
    if (y > (int16_t)y1)
    {
        return (int16_t)y1;
    }
    return y;
}

static void ecg_monitor_draw_grid(uint16_t x0, uint16_t x1,
                                  uint16_t y0, uint16_t y1,
                                  uint16_t center_y)
{
    uint16_t x;
    uint16_t y;

    /* Sparse minor intersections keep the SPI refresh cost bounded. */
    for (x = x0; x <= x1; x += ECG_GRID_MINOR_X)
    {
        for (y = y0; y <= y1; y += ECG_GRID_MINOR_Y)
        {
            TFT_DrawPoint(x, y, DARKBLUE);
        }
    }

    /* Continuous major divisions form a readable ECG-style reference grid. */
    for (x = x0; x <= x1; x += ECG_GRID_MAJOR_X)
    {
        TFT_Fill(x, y0, (uint16_t)(x + 1U), (uint16_t)(y1 + 1U), DARKBLUE);
    }
    for (y = y0; y <= y1; y += ECG_GRID_MAJOR_Y)
    {
        TFT_Fill(x0, y, (uint16_t)(x1 + 1U), (uint16_t)(y + 1U), DARKBLUE);
    }
    TFT_Fill(x0, center_y, (uint16_t)(x1 + 1U),
             (uint16_t)(center_y + 1U), GRAYBLUE);
    TFT_Fill(x0, y0, (uint16_t)(x1 + 1U), (uint16_t)(y0 + 1U), DARKBLUE);
    TFT_Fill(x0, y1, (uint16_t)(x1 + 1U), (uint16_t)(y1 + 1U), DARKBLUE);
    TFT_Fill(x0, y0, (uint16_t)(x0 + 1U), (uint16_t)(y1 + 1U), DARKBLUE);
    TFT_Fill(x1, y0, (uint16_t)(x1 + 1U), (uint16_t)(y1 + 1U), DARKBLUE);
}

static void ecg_monitor_draw_plot_scanlines(const ecg_monitor_view_t *view,
                                            const int16_t *plot_y,
                                            uint16_t plot_count,
                                            uint16_t x0, uint16_t x1,
                                            uint16_t y0, uint16_t y1,
                                            uint16_t center_y)
{
    uint16_t width = (uint16_t)(x1 - x0 + 1U);
    uint16_t y;

    if (plot_count < width)
    {
        width = plot_count;
    }
    for (y = y0; y <= y1; ++y)
    {
        uint16_t i;
        for (i = 0U; i < width; ++i)
        {
            uint16_t x = (uint16_t)(x0 + i);
            uint16_t color = BLACK;
            uint16_t current_y = (uint16_t)ecg_monitor_clamp_y(
                plot_y[i], y0, y1);
            uint8_t on_trace = (y == current_y) ? 1U : 0U;

            if ((((x - x0) % ECG_GRID_MINOR_X) == 0U) &&
                (((y - y0) % ECG_GRID_MINOR_Y) == 0U))
            {
                color = DARKBLUE;
            }
            if ((((x - x0) % ECG_GRID_MAJOR_X) == 0U) ||
                (((y - y0) % ECG_GRID_MAJOR_Y) == 0U))
            {
                color = DARKBLUE;
            }
            if (y == center_y)
            {
                color = GRAYBLUE;
            }
            if ((x == x0) || (x == x1) || (y == y0) || (y == y1))
            {
                color = DARKBLUE;
            }
            if (i != 0U)
            {
                uint16_t previous_y = (uint16_t)ecg_monitor_clamp_y(
                    plot_y[i - 1U], y0, y1);
                uint16_t top = (previous_y < current_y) ? previous_y : current_y;
                uint16_t bottom = (previous_y > current_y) ? previous_y : current_y;
                if ((y >= top) && (y <= bottom))
                {
                    on_trace = 1U;
                }
            }
            if (on_trace != 0U)
            {
                color = GREEN;
            }
            if ((view->event_marker_valid != 0U) &&
                (x == view->event_marker_x))
            {
                color = RED;
            }
            plot_scanline[i] = color;
        }
        TFT_DrawPixelRow(x0, y, plot_scanline, width);
    }
}

static uint8_t ecg_monitor_plot_changed(const ecg_monitor_view_t *view,
                                        const int16_t *plot_y,
                                        uint16_t plot_count)
{
    uint16_t i;

    if ((previous_plot_valid == 0U) ||
        (previous_view.page != view->page) ||
        (previous_plot_count != plot_count) ||
        (previous_view.event_marker_valid != view->event_marker_valid) ||
        (previous_view.event_marker_x != view->event_marker_x))
    {
        return 1U;
    }
    for (i = 0U; i < plot_count; ++i)
    {
        if (previous_plot[i] != plot_y[i])
        {
            return 1U;
        }
    }
    return 0U;
}

static void ecg_monitor_update_plot(const ecg_monitor_view_t *view,
                                    const int16_t *plot_y,
                                    uint16_t plot_count,
                                    uint16_t x0, uint16_t x1,
                                    uint16_t y0, uint16_t y1,
                                    uint16_t center_y)
{
    uint16_t i;
    uint16_t width = (uint16_t)(x1 - x0 + 1U);

    if (plot_count > width)
    {
        plot_count = width;
    }
    if (ecg_monitor_plot_changed(view, plot_y, plot_count) == 0U)
    {
        return;
    }

    /* Compose final pixels row by row so the panel never exposes an all-black
       intermediate frame and each scanline uses only one address window. */
    ecg_monitor_draw_plot_scanlines(view, plot_y, plot_count,
                                    x0, x1, y0, y1, center_y);

    for (i = 0U; i < plot_count; ++i)
    {
        previous_plot[i] = plot_y[i];
    }
    previous_plot_count = plot_count;
    previous_plot_valid = 1U;
}

static void ecg_monitor_render_overview(const ecg_monitor_view_t *view,
                                        const int16_t *plot_y,
                                        uint16_t plot_count)
{
    char text[21];
    uint16_t display_rmssd;
    uint16_t value_color;
    uint8_t display_gain = (view->gain > 9U) ? 9U : view->gain;
    uint8_t display_window = (view->window_seconds > 99U) ?
                             99U : view->window_seconds;

    ecg_monitor_update_plot(view, plot_y, plot_count,
                            ECG_MONITOR_PLOT_X0, ECG_MONITOR_PLOT_X1,
                            ECG_MONITOR_PLOT_Y0, ECG_MONITOR_PLOT_Y1,
                            (uint16_t)ECG_MONITOR_PLOT_CENTER_Y);

    if ((previous_view_valid == 0U) ||
        (previous_view.running != view->running))
    {
        TFT_Fill(120U, 0U, ECG_SCREEN_X_END, 16U, BLACK);
        TFT_ShowString(128U, 0U,
                       (const uint8_t *)((view->running != 0U) ? "RUN" : "FRZ"),
                       (view->running != 0U) ? GREEN : YELLOW,
                       BLACK, 16U, 0U);
    }

    if ((previous_view_valid == 0U) ||
        (previous_view.bpm != view->bpm) ||
        (previous_view.quality != view->quality))
    {
        TFT_Fill(108U, 32U, ECG_SCREEN_X_END, 56U, BLACK);
        value_color = (view->quality == ECG_SIGNAL_GOOD) ? GREEN : YELLOW;
        if ((view->bpm == 0U) || (view->bpm > 999U) ||
            (view->quality == ECG_SIGNAL_LEAD_OFF) ||
            (view->quality == ECG_SIGNAL_CLIPPED))
        {
            TFT_ShowString(112U, 32U, (const uint8_t *)"---",
                           value_color, BLACK, 24U, 0U);
        }
        else
        {
            sprintf(text, "%3u", (unsigned int)view->bpm);
            TFT_ShowString(112U, 32U, (const uint8_t *)text,
                           value_color, BLACK, 24U, 0U);
        }
    }

    if ((previous_view_valid == 0U) ||
        (previous_view.rr_ms != view->rr_ms))
    {
        TFT_Fill(0U, 80U, 78U, 96U, BLACK);
        if (view->rr_ms == 0U)
        {
            TFT_ShowString(2U, 80U, (const uint8_t *)"RR:--- ",
                           CYAN, BLACK, 16U, 0U);
        }
        else
        {
            sprintf(text, "RR:%4u", (unsigned int)view->rr_ms);
            TFT_ShowString(2U, 80U, (const uint8_t *)text,
                           CYAN, BLACK, 16U, 0U);
        }
    }

    if ((previous_view_valid == 0U) ||
        (previous_view.rmssd_ms != view->rmssd_ms))
    {
        TFT_Fill(78U, 80U, ECG_SCREEN_X_END, 96U, BLACK);
        if (view->rmssd_ms == 0U)
        {
            TFT_ShowString(78U, 80U, (const uint8_t *)"RMSSD:--- ",
                           CYAN, BLACK, 16U, 0U);
        }
        else
        {
            display_rmssd = (view->rmssd_ms > 9999U) ? 9999U : view->rmssd_ms;
            sprintf(text, "RMSSD:%4u", (unsigned int)display_rmssd);
            TFT_ShowString(78U, 80U, (const uint8_t *)text,
                           CYAN, BLACK, 16U, 0U);
        }
    }

    if ((previous_view_valid == 0U) ||
        (previous_view.quality != view->quality))
    {
        TFT_Fill(0U, 96U, ECG_SCREEN_X_END, 112U, BLACK);
        TFT_ShowChinese(2U, 98U, (uint8_t *)text_input_status,
                        ecg_monitor_quality_color(view->quality),
                        BLACK, 12U, 0U);
        sprintf(text, "%-8s", ecg_monitor_quality_text(view->quality));
        TFT_ShowString(52U, 96U, (const uint8_t *)text,
                       ecg_monitor_quality_color(view->quality),
                       BLACK, 16U, 0U);
    }
    if ((previous_view_valid == 0U) ||
        (previous_view.gain != view->gain) ||
        (previous_view.fit_limited != view->fit_limited) ||
        (previous_view.window_seconds != view->window_seconds) ||
        (previous_view.event_marker_valid != view->event_marker_valid))
    {
        TFT_Fill(0U, 112U, ECG_SCREEN_X_END, ECG_SCREEN_Y_END, BLACK);
        TFT_ShowChinese(2U, 114U, (uint8_t *)text_amplitude,
                        WHITE, BLACK, 12U, 0U);
        sprintf(text, "x%u%s %2us %s", (unsigned int)display_gain,
                (view->fit_limited != 0U) ? "F" : " ",
                (unsigned int)display_window,
                (view->event_marker_valid != 0U) ? "EVT" : "   ");
        TFT_ShowString(28U, 112U, (const uint8_t *)text,
                       WHITE, BLACK, 16U, 0U);
    }
}

static void ecg_monitor_render_wave(const ecg_monitor_view_t *view,
                                    const int16_t *plot_y,
                                    uint16_t plot_count)
{
    char text[21];
    uint8_t display_gain = (view->gain > 9U) ? 9U : view->gain;

    if ((previous_view_valid == 0U) ||
        (previous_view.bpm != view->bpm) ||
        (previous_view.running != view->running) ||
        (previous_view.quality != view->quality))
    {
        TFT_Fill(0U, 0U, ECG_SCREEN_X_END, 16U, BLACK);
        if ((view->bpm == 0U) || (view->bpm > 999U) ||
            (view->quality == ECG_SIGNAL_LEAD_OFF) ||
            (view->quality == ECG_SIGNAL_CLIPPED))
        {
            sprintf(text, "HR:--- %s",
                    (view->running != 0U) ? "RUN" : "FRZ");
        }
        else
        {
            sprintf(text, "HR:%3u %s", (unsigned int)view->bpm,
                    (view->running != 0U) ? "RUN" : "FRZ");
        }
        TFT_ShowChinese(0U, 0U, (uint8_t *)text_waveform,
                        (view->running != 0U) ? GREEN : YELLOW,
                        BLACK, 16U, 0U);
        TFT_ShowString(36U, 0U, (const uint8_t *)text,
                       (view->running != 0U) ? GREEN : YELLOW,
                       BLACK, 16U, 0U);
    }

    ecg_monitor_update_plot(view, plot_y, plot_count,
                            ECG_WAVE_PLOT_X0, ECG_WAVE_PLOT_X1,
                            ECG_WAVE_PLOT_Y0, ECG_WAVE_PLOT_Y1,
                            (uint16_t)ECG_WAVE_PLOT_CENTER_Y);

    if ((previous_view_valid == 0U) ||
        (previous_view.quality != view->quality) ||
        (previous_view.gain != view->gain) ||
        (previous_view.timebase_ms != view->timebase_ms) ||
        (previous_view.fit_limited != view->fit_limited) ||
        (previous_view.wave_frame_ready != view->wave_frame_ready) ||
        (previous_view.wave_span != view->wave_span) ||
        (previous_view.event_marker_valid != view->event_marker_valid))
    {
        TFT_Fill(0U, 112U, ECG_SCREEN_X_END, ECG_SCREEN_Y_END, BLACK);
        TFT_ShowChinese(0U, 114U, (uint8_t *)text_status,
                        ecg_monitor_quality_color(view->quality),
                        BLACK, 12U, 0U);
        if (view->wave_frame_ready == 0U)
        {
            sprintf(text, "DMA WAIT x%u %ums", (unsigned int)display_gain,
                    (unsigned int)view->timebase_ms);
        }
        else if (view->wave_span < 2U)
        {
            sprintf(text, "FLAT    x%u %ums", (unsigned int)display_gain,
                    (unsigned int)view->timebase_ms);
        }
        else
        {
            sprintf(text, "%-8s x%u%s %ums",
                    ecg_monitor_quality_text(view->quality),
                    (unsigned int)display_gain,
                    (view->fit_limited != 0U) ? "F" : " ",
                    (unsigned int)view->timebase_ms);
        }
        TFT_ShowString(26U, 112U, (const uint8_t *)text,
                       ecg_monitor_quality_color(view->quality),
                       BLACK, 16U, 0U);
    }
}

static void ecg_monitor_render_frequency(const ecg_monitor_view_t *view)
{
    char text[21];
    uint32_t difference;
    uint32_t tolerance;
    uint32_t measured = view->pwm_measured_hz;

    if ((previous_view_valid == 0U) ||
        (previous_view.pwm_target_hz != view->pwm_target_hz) ||
        (previous_view.pwm_enabled != view->pwm_enabled))
    {
        TFT_Fill(0U, 24U, ECG_SCREEN_X_END, 40U, BLACK);
        TFT_ShowChinese(0U, 26U, (uint8_t *)text_output,
                        (view->pwm_enabled != 0U) ? GREEN : YELLOW,
                        BLACK, 12U, 0U);
        sprintf(text, ":%4uHz %s", (unsigned int)view->pwm_target_hz,
                (view->pwm_enabled != 0U) ? "ON " : "OFF");
        TFT_ShowString(24U, 24U, (const uint8_t *)text,
                       (view->pwm_enabled != 0U) ? GREEN : YELLOW,
                       BLACK, 16U, 0U);
    }

    if ((previous_view_valid == 0U) ||
        (previous_view.pwm_target_hz != view->pwm_target_hz) ||
        (previous_view.pwm_measured_hz != view->pwm_measured_hz) ||
        (previous_view.pwm_enabled != view->pwm_enabled))
    {
        TFT_Fill(0U, 48U, ECG_SCREEN_X_END, 88U, BLACK);
        if (measured == 0U)
        {
            TFT_ShowChinese(0U, 50U, (uint8_t *)text_input,
                            YELLOW, BLACK, 12U, 0U);
            TFT_ShowString(24U, 48U, (const uint8_t *)":----Hz NO SIG",
                           YELLOW, BLACK, 16U, 0U);
            TFT_ShowString(0U, 72U, (const uint8_t *)"DIFF:---- NO DATA",
                           YELLOW, BLACK, 16U, 0U);
        }
        else
        {
            if (measured > 99999U)
            {
                TFT_ShowChinese(0U, 50U, (uint8_t *)text_input,
                                CYAN, BLACK, 12U, 0U);
                TFT_ShowString(24U, 48U, (const uint8_t *)":>99KHz SIGNAL",
                               CYAN, BLACK, 16U, 0U);
            }
            else
            {
                TFT_ShowChinese(0U, 50U, (uint8_t *)text_input,
                                CYAN, BLACK, 12U, 0U);
                sprintf(text, ":%5luHz SIGNAL", (unsigned long)measured);
                TFT_ShowString(24U, 48U, (const uint8_t *)text,
                               CYAN, BLACK, 16U, 0U);
            }

            if (view->pwm_enabled == 0U)
            {
                TFT_ShowString(0U, 72U, (const uint8_t *)"SOURCE:EXTERNAL",
                               CYAN, BLACK, 16U, 0U);
            }
            else
            {
                difference = (measured >= view->pwm_target_hz) ?
                             (measured - view->pwm_target_hz) :
                             ((uint32_t)view->pwm_target_hz - measured);
                tolerance = (uint32_t)view->pwm_target_hz / 100U;
                if (tolerance < 2U)
                {
                    tolerance = 2U;
                }
                if (difference > 9999U)
                {
                    difference = 9999U;
                }
                sprintf(text, "ERR:%4luHz %s", (unsigned long)difference,
                        (difference <= tolerance) ? "MATCH" : "MISMATCH");
                TFT_ShowString(0U, 72U, (const uint8_t *)text,
                               (difference <= tolerance) ? GREEN : YELLOW,
                               BLACK, 16U, 0U);
            }
        }
    }
}

void ECGMonitorUI_DrawStatic(ecg_monitor_page_t page)
{
    previous_view_valid = 0U;
    previous_plot_valid = 0U;
    previous_plot_count = 0U;
    TFT_Fill(0U, 0U, ECG_SCREEN_X_END, ECG_SCREEN_Y_END, BLACK);
    if (page == ECG_FREQ_PAGE)
    {
        TFT_ShowString(0U, 0U, (const uint8_t *)"PWM",
                       WHITE, BLACK, 16U, 0U);
        TFT_ShowChinese(28U, 2U, (uint8_t *)text_frequency,
                        WHITE, BLACK, 12U, 0U);
        TFT_ShowChinese(0U, 26U, (uint8_t *)text_output,
                        YELLOW, BLACK, 12U, 0U);
        TFT_ShowString(24U, 24U, (const uint8_t *)":----Hz OFF",
                       YELLOW, BLACK, 16U, 0U);
        TFT_ShowChinese(0U, 50U, (uint8_t *)text_input,
                        YELLOW, BLACK, 12U, 0U);
        TFT_ShowString(24U, 48U, (const uint8_t *)":----Hz NO SIG",
                       YELLOW, BLACK, 16U, 0U);
        TFT_ShowString(0U, 72U, (const uint8_t *)"DIFF:---- NO DATA",
                       YELLOW, BLACK, 16U, 0U);
        TFT_ShowString(0U, 96U, (const uint8_t *)"PA2>PA6 DUTY:50",
                       WHITE, BLACK, 16U, 0U);
        TFT_ShowString(0U, 112U, (const uint8_t *)"KEYD:",
                       GRAY, BLACK, 16U, 0U);
        TFT_ShowChinese(40U, 112U, (uint8_t *)text_open_close,
                        GRAY, BLACK, 16U, 0U);
        TFT_ShowString(72U, 112U, (const uint8_t *)"ENC:",
                       GRAY, BLACK, 16U, 0U);
        TFT_ShowChinese(104U, 114U, (uint8_t *)text_frequency,
                        GRAY, BLACK, 12U, 0U);
        return;
    }
    if (page == ECG_WAVE_PAGE)
    {
        TFT_ShowChinese(0U, 0U, (uint8_t *)text_waveform,
                        YELLOW, BLACK, 16U, 0U);
        TFT_ShowString(36U, 0U, (const uint8_t *)"HR:--- FRZ",
                       YELLOW, BLACK, 16U, 0U);
        ecg_monitor_draw_grid(ECG_WAVE_PLOT_X0, ECG_WAVE_PLOT_X1,
                              ECG_WAVE_PLOT_Y0, ECG_WAVE_PLOT_Y1,
                              (uint16_t)ECG_WAVE_PLOT_CENTER_Y);
        TFT_ShowChinese(0U, 114U, (uint8_t *)text_status,
                        YELLOW, BLACK, 12U, 0U);
        TFT_ShowString(26U, 112U, (const uint8_t *)"DMA WAIT x1 5ms",
                       YELLOW, BLACK, 16U, 0U);
        return;
    }

    TFT_ShowString(2U, 0U, (const uint8_t *)"HESS ECG",
                   WHITE, BLACK, 16U, 0U);
    TFT_ShowString(128U, 0U, (const uint8_t *)"FRZ",
                   YELLOW, BLACK, 16U, 0U);
    ecg_monitor_draw_grid(ECG_MONITOR_PLOT_X0, ECG_MONITOR_PLOT_X1,
                          ECG_MONITOR_PLOT_Y0, ECG_MONITOR_PLOT_Y1,
                          (uint16_t)ECG_MONITOR_PLOT_CENTER_Y);
    TFT_ShowChinese(112U, 18U, (uint8_t *)text_frequency,
                    WHITE, BLACK, 12U, 0U);
    TFT_ShowString(112U, 32U, (const uint8_t *)"---",
                   YELLOW, BLACK, 24U, 0U);
    TFT_ShowString(116U, 60U, (const uint8_t *)"BPM",
                   WHITE, BLACK, 16U, 0U);
    TFT_ShowString(2U, 80U, (const uint8_t *)"RR:--- ",
                   CYAN, BLACK, 16U, 0U);
    TFT_ShowString(78U, 80U, (const uint8_t *)"RMSSD:--- ",
                   CYAN, BLACK, 16U, 0U);
    TFT_ShowChinese(2U, 98U, (uint8_t *)text_input_status,
                    YELLOW, BLACK, 12U, 0U);
    TFT_ShowString(52U, 96U, (const uint8_t *)"WAIT",
                   YELLOW, BLACK, 16U, 0U);
    TFT_ShowChinese(2U, 114U, (uint8_t *)text_amplitude,
                    WHITE, BLACK, 12U, 0U);
    TFT_ShowString(28U, 112U, (const uint8_t *)"x1  5s",
                   WHITE, BLACK, 16U, 0U);
}

void ECGMonitorUI_Render(const ecg_monitor_view_t *view,
                         const int16_t *plot_y,
                         uint16_t plot_count)
{
    if (view == (const ecg_monitor_view_t *)0)
    {
        return;
    }

    if (view->page == ECG_FREQ_PAGE)
    {
        ecg_monitor_render_frequency(view);
        previous_view = *view;
        previous_view_valid = 1U;
        return;
    }

    if ((plot_y == (const int16_t *)0) || (plot_count < 2U))
    {
        return;
    }

    if (view->page == ECG_WAVE_PAGE)
    {
        ecg_monitor_render_wave(view, plot_y, plot_count);
    }
    else
    {
        ecg_monitor_render_overview(view, plot_y, plot_count);
    }
    previous_view = *view;
    previous_view_valid = 1U;
}
