# Demo14 HESS ECG Analyzer Firmware Specification

## Objective

Provide a standalone GD32E230C8T6 ECG acquisition and analysis demonstration.
The firmware presents a HESS startup screen, continuously acquires the analog
ECG input, filters the samples, detects R peaks, reports heart rate and RR/HRV
metrics, and exposes signal-quality status on the 160 x 128 TFT.

## Hardware contract

- MCU: GD32E230C8T6.
- Analog ECG input: PA3 / ADC channel 3, 0 V through 3.3 V only.
- PWM test output: PA2 / TIMER14 channel 0, 50% duty.
- Frequency-monitor input: PA6 / TIMER2 channel 0, 0 V through 3.3 V only.
- Sample rate: 250 Hz, driven from the existing 1 ms application tick.
- Display: SPI TFT, 160 x 128 pixels.
- Keys and rotary encoder may control view options without blocking sampling.
- The ECG input is measurement-only; Demo14 does not generate a PWM test signal.

## Functional requirements

1. `HESS_Splash_Show()` draws the HESS ECG Analyzer startup screen before the
   acquisition UI is shown.
2. Public acquisition-task operations use the `ECGAcq_` prefix and keep
   hardware/UI coordination outside the pure signal-processing core.
3. `ECGAcqCore_Process()` accepts one unsigned 12-bit ADC sample per call at
   250 Hz and clamps values above 4095 before analysis.
4. The core removes baseline drift, smooths the centered signal, enforces a
   250 ms refractory interval, and detects R peaks without dynamic allocation.
5. Valid RR intervals are 300 through 2000 ms. Heart rate is calculated from
   the bounded eight-interval history; RMSSD is reported when at least two RR
   intervals are available.
6. Signal quality is evaluated in fixed one-second windows and reports `WAIT`,
   `GOOD`, `POOR`, `LEAD OFF`, or `CLIPPED`.
7. `SW1` toggles live acquisition and waveform freeze. `SW2` short press cycles
   the display gain through x1, x2, and x4; `SW2` double press cycles the
   hospital-style `MONITOR`, enlarged `WAVE`, and PWM `FREQ` pages.
8. A short `SW3` press records an event marker in the waveform timeline; a
   double `SW3` press resets the acquisition and HRV history.
9. The rotary encoder selects a 2, 5, or 10 second waveform window.
10. The UI displays live waveform/status information using concise Chinese
    descriptions plus standard ECG/PWM abbreviations and remains within the
    160 x 128 display boundary. Chinese labels use explicit GBK byte arrays
    compatible with the existing TFT font table. The three-page layout and
    display seam are governed by `Demo14_HOSPITAL_MONITOR_UI_SPEC.md`.
11. Source paths, identifiers, and project output names introduced by Demo14 use
    English ASCII naming. User-facing TFT descriptions may use the verified GBK
    glyph set.
12. The `FREQ` page keeps PA2's configured PWM frequency separate from PA6's
    captured frequency, expires stale input after 1500 ms, and supports
    100/250/500/1000/2000 Hz presets without floating point.
13. The waveform history is derived from the raw 12-bit ADC sample, centered at
    midscale and saturated to the signed display range. Baseline removal and
    smoothing remain confined to ECG analysis so a steady square-wave level is
    not rendered as an exponential decay.
14. Every Chinese label used by the UI has a matching 12- or 16-pixel glyph.
    Font-table indexes are explicit GBK byte escapes so source-file encoding
    cannot change the lookup key; the 16-pixel `示` glyph has two distinct
    horizontal strokes.

## Resource and safety constraints

- No dynamic memory allocation.
- Acquisition must remain non-blocking after the startup animation completes.
- Sampling state shared with interrupt context must be declared and accessed in
  a way that prevents torn snapshots.
- Values shown to the user must distinguish unavailable data from a measured
  zero value.
- Host tests validate algorithms and source contracts; they do not replace
  verification with the actual ECG analog front end.

## Verification checkpoints

1. Run `Tests/run_tests.ps1`; the ECG core tests and firmware contract must pass.
2. Rebuild `Project/Demo14_HESS_ECG_Analyzer.uvprojx` with
   `GigaDevice.GD32E23x_DFP 1.1.0` and confirm zero errors and zero warnings.
3. Flash `Project/Objects/Demo14_HESS_ECG_Analyzer.hex` to GD32E230C8T6.
4. Confirm the splash screen transitions to the live analyzer UI.
5. Apply a safe ECG simulator signal to PA3 and verify waveform direction,
   250 Hz sampling, BPM, RR, RMSSD, lead-off, clipping, and key/encoder polarity.
6. Repeat with the intended analog front end and realistic noise before treating
   the readings as accepted hardware results.
7. Verify PA2 PWM with an oscilloscope, then connect PA2 to PA6 and check target,
   measured frequency, loopback error, preset switching, and no-signal timeout.
8. Apply a safe, midscale-biased square wave to PA3 and confirm that both high
   and low plateaus remain flat on `WAVE`; then inspect all Chinese page labels,
   especially `示波`, on the physical TFT.
