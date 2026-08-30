# Demo14 HESS ECG Analyzer Firmware Specification

## Objective

Provide a standalone GD32E230C8T6 ECG acquisition and analysis demonstration.
The firmware presents a HESS startup screen, continuously acquires one safe
analog ECG signal, detects R peaks, reports BPM/RR/RMSSD and displays signal
quality on the 160 x 128 TFT.

## Hardware and sampling contract

- Analog input: PA3 / ADC channel 3, strictly 0 V through 3.3 V.
- Recommended bench source: ECG simulator or isolated analog-front-end output,
  centered near 1.65 V; 1.0 Vpp is a safe initial amplitude.
- ADC sample rate: 250 Hz. TIMER0 CH0 produces one conversion trigger every
  4 ms using a 1 MHz timer count, ARR 3999 and CCR 2000.
- TIMER15 supplies the nonblocking 1 ms application tick.
- Demo14 does not initialize or link PWM generation, external frequency
  capture, the former 40 kSa/s DMA stream, or a square-wave self-test page.
- This demonstration is not a medical device. Never connect a patient or an
  unisolated electrode circuit directly to the board.

## Functional requirements

1. `HESS_Splash_Show()` runs before the acquisition UI.
2. The pure ECG core accepts one unsigned 12-bit ADC sample at 250 Hz, removes
   baseline drift for analysis, smooths the signal and detects R peaks with a
   250 ms refractory interval and no dynamic allocation.
3. Valid RR intervals are 300 through 2000 ms. BPM and RMSSD use a bounded
   eight-interval history.
4. Signal quality reports `WAIT`, `GOOD`, `POOR`, `LEAD OFF`, or `CLIPPED` in
   fixed one-second windows. Invalid input must not present stale BPM as current.
5. The raw centered ADC history feeds both display pages; the filtered sample
   remains confined to ECG analysis.
6. `MONITOR` shows waveform, BPM, RR, RMSSD, quality, gain, time window,
   run/freeze state and event marker.
7. `WAVE` is the enlarged ECG view. It displays the same 250 Hz history over a
   default two-second window, not a millisecond oscilloscope frame.
8. `SW1` toggles run/freeze. `SW2` short press cycles x1/x2/x4 gain; `SW2`
   double press toggles `MONITOR` and `WAVE`. `SW3` adds an event marker; double
   press resets measurement history. The encoder selects a 2 or 5 second view.
9. Vertical mapping automatically centers the visible frame and limits gain
   before the trace reaches the plot border; `F` identifies this FIT condition.
10. Chinese labels use explicit GBK bytes supported by the existing font table.
11. UI work is scheduled every 40 ms. Changed plots are composed into final
    color scanlines and sent as one SPI burst per row; unchanged/frozen plots
    are skipped using a 32-bit waveform revision rather than a duplicate plot.

## Resource constraints

- No dynamic allocation or floating-point UI work.
- ECG history is one signed byte per sample and is capped at 1250 bytes,
  sufficient for the five-second maximum window.
- The high-rate DMA double buffer and duplicate 312-byte plot cache must not be
  present in the linked image.
- Interrupt-shared acquisition state uses a sequence snapshot to prevent torn
  display data.
- Final RAM and maximum analyzed stack usage must be taken from the Keil map and
  call-graph reports after the final full rebuild.

## Verification checkpoints

1. Run `Tests/run_tests.ps1`; all host and firmware-contract tests must pass.
2. Rebuild `Project/Demo14_HESS_ECG_Analyzer.uvprojx` with
   `GigaDevice.GD32E23x_DFP 1.1.0`; require zero errors and zero warnings.
3. Flash `Project/Objects/Demo14_HESS_ECG_Analyzer.hex`.
4. Apply only a safe ECG simulator/front-end output to PA3 with common ground.
5. Confirm `SW2` double press enters `WAVE` and at least one ECG cycle is visible
   in the default two-second window; rotate to five seconds and repeat.
6. Check x1/x2/x4, run/freeze, event/reset, BPM/RR/RMSSD, quality states and the
   physical TFT refresh. Host/build checks do not replace this board test.
