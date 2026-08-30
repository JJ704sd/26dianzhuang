#include "ecg_monitor_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static uint32_t fill_pixels;
static uint32_t drawn_pixels;
static uint32_t address_windows;
static char last_bottom_text[32];

void TFT_Fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
              uint16_t color)
{
    (void)color;
    CHECK(x1 >= x0);
    CHECK(y1 >= y0);
    CHECK(x1 <= 160U);
    CHECK(y1 <= 128U);
    fill_pixels += (uint32_t)(x1 - x0) * (uint32_t)(y1 - y0);
    address_windows++;
}

void TFT_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    (void)color;
    CHECK(x < 160U);
    CHECK(y < 128U);
    drawn_pixels++;
    address_windows++;
}

void TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  uint16_t color)
{
    uint16_t dx = (x0 > x1) ? (uint16_t)(x0 - x1) : (uint16_t)(x1 - x0);
    uint16_t dy = (y0 > y1) ? (uint16_t)(y0 - y1) : (uint16_t)(y1 - y0);

    (void)color;
    CHECK(x0 < 160U);
    CHECK(x1 < 160U);
    CHECK(y0 < 128U);
    CHECK(y1 < 128U);
    drawn_pixels += (uint32_t)((dx > dy) ? dx : dy) + 1U;
    /* The production line primitive opens one LCD address window per pixel. */
    address_windows += (uint32_t)((dx > dy) ? dx : dy) + 1U;
}

void TFT_DrawPixelRow(uint16_t x, uint16_t y, const uint16_t *colors,
                      uint16_t count)
{
    CHECK(colors != NULL);
    CHECK(count != 0U);
    CHECK((uint32_t)x + count <= 160U);
    CHECK(y < 128U);
    drawn_pixels += count;
    address_windows++;
}

void TFT_ShowString(uint16_t x, uint16_t y, const uint8_t *text,
                    uint16_t foreground, uint16_t background,
                    uint8_t size, uint8_t overlay)
{
    size_t length = strlen((const char *)text);

    (void)foreground;
    (void)background;
    (void)overlay;
    CHECK((uint32_t)x + ((uint32_t)length * (size / 2U)) <= 160U);
    CHECK((uint32_t)y + size <= 128U);
    drawn_pixels += (uint32_t)length * (uint32_t)(size / 2U) * size;
    if (y >= 112U)
    {
        snprintf(last_bottom_text, sizeof(last_bottom_text), "%s", text);
    }
}

void TFT_ShowChinese(uint16_t x, uint16_t y, uint8_t *text,
                     uint16_t foreground, uint16_t background,
                     uint8_t size, uint8_t overlay)
{
    uint16_t count = 0U;

    (void)foreground;
    (void)background;
    (void)overlay;
    while (*text != 0U)
    {
        count++;
        text += 2;
    }
    CHECK((uint32_t)x + ((uint32_t)count * size) <= 160U);
    CHECK((uint32_t)y + size <= 128U);
    drawn_pixels += (uint32_t)count * size * size;
}

static void make_plot(int16_t *plot, uint16_t count, int16_t center)
{
    uint16_t i;

    for (i = 0U; i < count; ++i)
    {
        plot[i] = (int16_t)(center + ((i % 13U) == 0U ? 5 : 0));
    }
}

static void test_dynamic_refresh_has_bounded_spi_work(void)
{
    ecg_monitor_view_t view = {0};
    int16_t plot[ECG_WAVE_PLOT_X1 - ECG_WAVE_PLOT_X0 + 1U];
    uint32_t first_fill;

    view.page = ECG_WAVE_PAGE;
    view.quality = ECG_SIGNAL_GOOD;
    view.running = 1U;
    view.gain = 1U;
    view.window_seconds = 2U;
    view.fit_limited = 1U;
    view.waveform_revision = 1U;
    make_plot(plot, (uint16_t)(sizeof(plot) / sizeof(plot[0])),
              ECG_WAVE_PLOT_CENTER_Y);

    ECGMonitorUI_DrawStatic(view.page);
    fill_pixels = 0U;
    drawn_pixels = 0U;
    address_windows = 0U;
    ECGMonitorUI_Render(&view, plot,
                        (uint16_t)(sizeof(plot) / sizeof(plot[0])));
    first_fill = fill_pixels;
    CHECK(strstr(last_bottom_text, "2s") != NULL);
    CHECK(strstr(last_bottom_text, "ms") == NULL);
    CHECK(strstr(last_bottom_text, "GOOD") != NULL);

    plot[40] = (int16_t)(plot[40] - 8);
    view.waveform_revision++;
    fill_pixels = 0U;
    drawn_pixels = 0U;
    address_windows = 0U;
    ECGMonitorUI_Render(&view, plot,
                        (uint16_t)(sizeof(plot) / sizeof(plot[0])));

    printf("UI work: initial fills=%lu, changed-frame fills=%lu, total=%lu, windows=%lu\n",
           (unsigned long)first_fill, (unsigned long)fill_pixels,
           (unsigned long)(fill_pixels + drawn_pixels),
           (unsigned long)address_windows);
    CHECK(first_fill < 24000U);
    CHECK(fill_pixels < 20000U);
    CHECK((fill_pixels + drawn_pixels) < 21000U);
    CHECK(address_windows < 160U);

    /* An unchanged revision must skip every waveform scanline. */
    fill_pixels = 0U;
    drawn_pixels = 0U;
    address_windows = 0U;
    ECGMonitorUI_Render(&view, plot,
                        (uint16_t)(sizeof(plot) / sizeof(plot[0])));
    CHECK(fill_pixels == 0U);
    CHECK(drawn_pixels == 0U);
    CHECK(address_windows == 0U);

    /* Gain must redraw a frozen waveform even if no new sample arrived. */
    view.gain = 2U;
    fill_pixels = 0U;
    drawn_pixels = 0U;
    address_windows = 0U;
    ECGMonitorUI_Render(&view, plot,
                        (uint16_t)(sizeof(plot) / sizeof(plot[0])));
    CHECK(drawn_pixels > 0U);
    CHECK(address_windows >= 93U);
}

static void test_monitor_static_page_uses_default_two_second_window(void)
{
    last_bottom_text[0] = '\0';
    ECGMonitorUI_DrawStatic(ECG_MONITOR_PAGE);
    CHECK(strstr(last_bottom_text, "2s") != NULL);
    CHECK(strstr(last_bottom_text, "5s") == NULL);
}

int main(void)
{
    test_dynamic_refresh_has_bounded_spi_work();
    test_monitor_static_page_uses_default_two_second_window();
    puts("ecg_monitor_ui bounded refresh tests passed");
    return EXIT_SUCCESS;
}
