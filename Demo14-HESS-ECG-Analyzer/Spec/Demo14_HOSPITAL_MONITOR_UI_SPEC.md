# Demo14 Hospital-style ECG Monitor UI Specification

## 1. Purpose and scope

This increment keeps the existing Demo14 ECG acquisition and analysis behavior,
but replaces the single dense screen with three practical pages:

- `MONITOR`: a bedside-monitor overview with a prominent heart-rate value,
  ECG trace, RR/RMSSD metrics, signal quality, run/freeze state, gain, and time
  window.
- `WAVE`: an enlarged ECG trace for waveform inspection, with a compact heart
  rate and status strip.
- `FREQ`: a PWM output and real input-capture monitor showing PA2 target output,
  PA6 measured frequency, signal presence, and loopback match status.

This remains teaching firmware. It must not display diagnostic claims, alarm
limits, patient identity, SpO2, respiration, or other measurements that the
hardware does not actually acquire.

Function descriptions shown to the demonstrator use concise Chinese labels
from the existing TFT font table. Standard measurement abbreviations such as
`ECG`, `HR`, `BPM`, `RR`, `RMSSD`, `Hz`, and short quality codes remain in ASCII
where they are clearer and more compact. Chinese strings are stored as explicit
GBK bytes rather than UTF-8 source literals.

## 2. Preserved hardware and analysis contract

- MCU: GD32E230C8T6; display: 160 x 128 SPI TFT.
- ECG input remains PA3 / ADC channel 3, limited to 0 V through 3.3 V.
- Sampling remains 250 Hz and non-blocking after the HESS splash.
- `ecg_acq_core` remains the only owner of baseline suppression, smoothing,
  R-wave detection, BPM, RR, RMSSD, and signal-quality analysis.
- No dynamic memory allocation and no floating-point requirement are added.
- Existing `GOOD`, `POOR`, `LEAD OFF`, `CLIPPED`, and `WAIT` meanings remain
  unchanged.
- PWM output uses PA2 / TIMER14 channel 0. Frequency input uses PA6 / TIMER2
  channel 0. These pins do not replace the PA3 ECG input.

## 3. Caller-visible behavior

### 3.1 Pages

1. Startup enters `MONITOR` mode.
2. `MONITOR` shows:
   - `HESS ECG` and run/freeze state in the header;
   - a green ECG waveform in the upper half;
   - a large green or yellow heart-rate value with `BPM` label;
   - latest RR and RMSSD values, using `---` when unavailable;
   - quality, gain, selected time window, and event indication.
3. `WAVE` shows:
   - an enlarged ECG plot using at least 90 vertical pixels;
   - compact BPM, quality, gain, time window, and run/freeze information;
   - the same event marker semantics as `MONITOR`.
4. Every page switch performs one complete static redraw before incremental
   refresh resumes. All drawing coordinates remain inside 160 x 128.
5. Heart rate is unavailable when BPM is zero and must be rendered as `---`,
   never as a measured `0`.
6. Both waveform pages use a practical ECG reference grid: low-brightness minor
   intersections, continuous major divisions, a highlighted zero-reference
   line, and a bounded plot border. The grid is a visual aid only; it must not
   claim calibrated 25 mm/s or 10 mm/mV medical-paper scaling.
7. `FREQ` shows the configured PWM output frequency and 50% duty state separately
   from the frequency measured at PA6. With a PA2-to-PA6 jumper it also shows the
   absolute loopback error and `MATCH`/`MISMATCH` status. This status verifies the
   signal path against the configured generator value; it is not an independent
   oscillator calibration. Without recent PA6 edges it shows `NO SIGNAL` rather
   than retaining a stale measurement.

### 3.2 Controls

- `SW1` short press: toggle running/frozen acquisition.
- `SW2` short press: cycle waveform gain `x1 -> x2 -> x4 -> x1`.
- `SW2` double press: cycle `MONITOR -> WAVE -> FREQ -> MONITOR`.
- `SW3` short press: add an event marker at the current sample.
- `SW3` double press: reset waveform and measurement history without changing
  the selected page, gain, or time window.
- Rotary encoder: select 2, 5, or 10 seconds as before.
- Rotary encoder on `FREQ`: select 100, 250, 500, 1000, or 2000 Hz PWM output.
- Encoder push (`KEYD`) short press on `FREQ`: toggle the PA2 PWM output. PWM
  starts off and PA2 is held at a defined low level while off. ECG-page keys do
  not silently change ECG state while `FREQ` is visible.
- Unsupported long-press events have no effect.

### 3.3 PWM frequency monitor behavior

- TIMER14 runs from a 1 MHz timer tick. The hardware period is programmed as
  `period_ticks - 1`; compare is `period_ticks / 2` for 50% duty.
- TIMER2 runs from a 1 MHz timer tick and measures PA6 rising-edge intervals,
  including 16-bit counter wraparound. PA6 uses a weak pull-down for a defined
  disconnected state and a nonzero digital input filter to reject short glitches.
- A measured value is valid only after two captured edges. If no completed
  measurement is received for 1500 ms, the public reading becomes unavailable.
- `OUT` is a configured generator value; `IN` is a captured value. They must
  never share a variable or label that could imply the output setting was
  measured. Since both timers use the MCU clock, loopback agreement is a routing
  and timer-function check, not proof of absolute frequency accuracy.
- Supported output presets deliberately stay within 100 through 2000 Hz to keep
  interrupt load bounded while remaining useful for bench verification.

## 4. Module boundary and interface

The selected design separates display policy from acquisition coordination:

- `Middle/ecg_acq_core.*`: pure, hardware-independent ECG analysis; unchanged.
- `APP/ecg_acq_task.*`: sampling, coherent snapshots, input dispatch, waveform
  history, plot-point preparation, and page ownership.
- `APP/ecg_monitor_ui.*`: TFT layout and rendering only. It receives an immutable
  view model plus prepared plot points and does not read ADC state, mutate ECG
  analysis, or interpret keys.
- `APP/hess_splash.*`: startup animation only; unchanged.
- `Middle/mid_pwm.*`: PA2 PWM period/duty/state control with integer-only math.
- `Middle/mid_timer.*`: PA6 capture result and 1500 ms stale-data timeout.

Required public UI interface:

```c
typedef enum {
    ECG_MONITOR_PAGE = 0,
    ECG_WAVE_PAGE,
    ECG_FREQ_PAGE
} ecg_monitor_page_t;

void ECGMonitorUI_DrawStatic(ecg_monitor_page_t page);
void ECGMonitorUI_Render(const ecg_monitor_view_t *view,
                         const int16_t *plot_y,
                         uint16_t plot_count);
```

`ecg_monitor_view_t` carries only display-ready state: page, BPM/RR/RMSSD,
quality, running flag, gain, window seconds, optional event-marker x coordinate,
and separate PWM target/measured fields. A null view always returns. Waveform
pages reject a null plot or fewer than two points; `FREQ` intentionally ignores
the plot arguments because it draws no waveform.

`ECGAcq_GetPage()` exposes the current page for tests and future callers. Page
changes are owned by `ECGAcq_KeyHandle()` and trigger `ECGMonitorUI_DrawStatic()`.

### Rejected alternative

Keeping all layouts directly inside `ecg_acq_task.c` would reduce the source
file count, but it couples TFT coordinates and monitor presentation to sampling
and snapshot logic. A generic callback-based renderer was also rejected because
there is only one TFT implementation and callbacks would add interface and
indirect-call cost without real variation. The selected view-model seam keeps
the interface small while making page layout independently contract-testable.

For frequency monitoring, displaying TIMER14's configured divisor as if it were
a measurement was rejected because it cannot detect wiring, pin mux, or timer
failures. Capturing only an external PA6 signal was also rejected as the sole
mode because it provides no built-in source for classroom verification. The
selected PA2 generator plus independent PA6 capture supports both loopback and
external-signal use without affecting PA3 ECG acquisition.

## 5. Timing, consistency, and resource constraints

- The 1 ms tick and 4 ms sample cadence must not call TFT functions.
- TIMER2 capture IRQ priority remains below TIMER15 so PWM measurement cannot
  delay the ECG sample tick for extended periods.
- UI refresh remains 100 ms nominal and may not pause acquisition state.
- A rendered frame must use one coherent snapshot of buffer head/count,
  measurement values, quality, marker state, and acquisition state.
- `buffer_sequence` remains the coherence guard. Multi-byte ISR-shared values
  must not be consumed outside a validated even sequence.
- The existing 2500-byte waveform history is retained; no full-frame buffer or
  dynamic allocation is allowed.
- Minor grid divisions remain sparse points while major divisions are lines, so
  the grid remains useful without turning every 100 ms refresh into a dense
  full-screen line redraw.
- Added static RAM should remain small enough for the GD32E230C8T6 target;
  final RAM/ROM and maximum analyzed stack are recorded only after a clean Keil
  rebuild.

## 6. Verification checkpoints

### Checkpoint A - specification and contract

- This specification exists before implementation starts.
- Firmware contract tests require `ecg_monitor_ui.c` in the Keil project,
  verify all page names, the `ECGAcq_GetPage()` seam, and SW2 double-press page
  switching while retaining short-press gain switching.

### Checkpoint B - host verification

- Existing ECG core tests remain green.
- Host PWM tests verify ARR `N-1`, zero-period clamping, duty clamping, shadow
  update before enable, a defined-low PA2 output while disabled, alternate-function
  restoration while enabled, and integer output-frequency reporting.
- A host-testable page/control state test proves startup in `MONITOR`, page
  toggle behavior, and preservation of page/gain/window across measurement
  reset, or equivalent contract evidence is provided when hardware headers
  prevent direct host linking.
- `Tests/run_tests.ps1` passes with warnings treated as errors.

### Checkpoint C - firmware build

- Keil ArmClang rebuild reports `0 Error(s), 0 Warning(s)`.
- The project references `APP/ecg_monitor_ui.c`, resolves
  `GigaDevice.GD32E23x_DFP 1.1.0`, and produces
  `Project/Objects/Demo14_HESS_ECG_Analyzer.hex`.
- Final code/RO/RW/ZI sizes, maximum analyzed stack, and HEX SHA-256 are updated
  in `Doc/readme.md` only from the final successful rebuild.

### Checkpoint D - physical board acceptance

- HESS splash transitions to `MONITOR` without visible stale regions.
- SW2 short press changes gain; SW2 double press changes page; neither interrupts
  the 250 Hz acquisition cadence.
- Large BPM digits, RR/RMSSD unavailable states, quality colors, event marker,
  and enlarged waveform are readable and remain within the TFT boundary.
- Verify PA3 scaling, key and encoder polarity, TFT refresh/flicker, clipping,
  lead-off, and realistic-noise behavior using a safe ECG simulator or isolated
  analog front end.
- Never connect unisolated electrodes directly to a person.
- Confirm PA2 with an oscilloscope first, then connect PA2 to PA6 for loopback;
  never connect PA6 to a source outside the board's 0 V through 3.3 V range.
