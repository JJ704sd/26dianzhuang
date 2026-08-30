# Demo14 HESS ECG Analyzer

Demo14 is a standalone GD32E230C8T6 ECG acquisition and analysis firmware. It
shows a HESS startup animation, samples the PA3 analog input at 250 Hz, filters
the waveform, detects R peaks, calculates BPM/RR/RMSSD, and reports input signal
quality on the 160 x 128 TFT. The display now provides a hospital-monitor-style
overview with a prominent heart-rate value and a separate enlarged waveform
page. A third page provides independent PA2 PWM output and PA6 frequency capture
for bench testing.

## Display and controls

The TFT uses concise Chinese function descriptions for easier live
demonstration. Standard compact units and ECG terms (`HR`, `BPM`, `RR`,
`RMSSD`, `Hz`) remain in ASCII.

- `MONITOR`: ECG trace, large BPM value, RR, RMSSD, quality, gain, time window,
  run/freeze state, and event indication.
- `WAVE`: 156 x 93 pixel high-speed preview. TIMER0 triggers the ADC at
  40 kSa/s; circular DMA supplies a stable 200-sample frame, giving a 5 ms
  timebase with rising-edge alignment.
- Both waveform areas use sparse minor intersections, continuous major grid
  lines, a highlighted center reference, and a clear plot border. This grid is
  for visual comparison only and is not calibrated ECG paper.
- `FREQ`: displays PA2 PWM target frequency, PA6 captured frequency, loopback
  error, explicit `MATCH`/`MISMATCH` status, output state, and `NO SIGNAL` after
  a 2500 ms capture timeout. The match status checks the loopback signal path;
  it is not an independent oscillator calibration.
- `SW1` short press: run/freeze.
- `SW2` short press: cycle x1/x2/x4 gain.
- `SW2` double press: cycle `MONITOR`, `WAVE`, and `FREQ` pages.
- `SW3` short press: add an event marker; double press: reset measurements.
- Rotary encoder: select a 2/5/10 second time window.
- On `FREQ`, rotary encoder selects 1/2/5/10/20 Hz; encoder push (`KEYD`)
  toggles the PA2 PWM output. The startup selection is 2 Hz; PWM starts disabled.
  While disabled, PA2 is configured as a GPIO output and held low.

## Build and flash

1. Open `Project/Demo14_HESS_ECG_Analyzer.uvprojx` in Keil uVision.
2. Rebuild the target and confirm zero errors and zero warnings.
3. Flash `Project/Objects/Demo14_HESS_ECG_Analyzer.hex` to a GD32E230C8T6.
4. Connect only a 0 V through 3.3 V ECG simulator or analog front-end output to
   PA3, with a common ground.
5. For PWM loopback, first verify PA2 with an oscilloscope, then connect PA2 to
   PA6. PA6 may instead monitor an external 0 V through 3.3 V PWM source.
   PA6 uses an internal pull-down and a digital capture filter so an open input
   has a defined idle state and short glitches are less likely to be counted.
   PA2 is a 0 V/3.3 V push-pull logic output at 50% duty; it does not provide an
   adjustable analog amplitude. At 1 Hz, allow about two seconds for two capture
   edges before judging the PA6 reading.

Do not connect a patient or an unisolated electrode circuit directly to the
development board. This demonstration is not a medical device.

## Verification

Run the host checks from the Demo14 directory:

```powershell
& .\Tests\run_tests.ps1
```

Then verify the startup screen, live waveform, BPM, RR, RMSSD, lead-off and
clipping status on the physical board. Host tests cannot validate ADC scaling,
front-end polarity, electrical isolation, real noise, or display timing.

For a bench signal generator, verify the waveform at PA3 before connecting it.
Use a common ground and a DC offset near 1.65 V so the complete waveform stays
inside 0 V through 3.3 V; for example, 1.0 Vpp with a 1.65 V offset is a safe
starting point. A source configured for negative voltage or more than 3.3 V
must not be connected directly to PA3.

The dynamic UI is scheduled every 40 ms (25 FPS) and updates numeric fields only
when their values change. Changed plots are composed directly into final-color
scanlines and sent with one SPI burst per row: the 156 x 93 WAVE plot therefore
uses 93 address windows instead of roughly one thousand per-pixel windows, with
no all-black intermediate frame. Chip select is released only after the final
SPI bit has shifted out. A heart-rate result becomes unavailable after three
seconds with no valid R peak, and invalid lead-off/clipped input never presents
an old BPM as current.

The plotted waveform now uses the raw ADC sample centered around 2048, while
heart-rate detection continues to use the baseline-suppressed and smoothed ECG
signal. This separation preserves the flat high and low plateaus of a square
wave without weakening the ECG analysis path. Chinese font indexes are stored
as explicit GBK byte escapes, and host checks cover every Chinese label used by
the UI plus the readable structure of `示` in `示波`.

KEY2 still requests x1/x2/x4 vertical gain. The display automatically centers
each visible frame and limits only the effective scale when the requested gain
would leave the plot. A trailing `F` beside the gain means FIT is active; the
complete waveform is therefore kept at least two pixels away from the border.

## Electrical-training acceptance check

Use these settings for a repeatable bench demonstration:

| Check | Signal/source setting | Pass condition on Demo14 |
| --- | --- | --- |
| 1 kHz square | 2 Vpp, +1.65 V offset, High-Z, common ground | WAVE shows about five cycles in 5 ms; both plateaus and edges are recognizable |
| 1 kHz sine/triangle | 2 Vpp, +1.65 V offset | Shape is recognizable and no false slow wave is shown |
| Vertical gain | Repeat at x1, x2 and x4 | Entire trace remains inside the grid; `F` appears if auto-fit is required |
| Practical frequency ceiling | Square/triangle up to 4 kHz; sine up to 5 kHz | No low-frequency alias trace; modest edge rounding at the ceiling is acceptable |
| Full safe input span | PA3 always remains within 0 V through 3.3 V | No rail overvoltage; up to approximately 3.3 Vpp is representable only when centered near 1.65 V |
| PWM low-frequency loopback | PA2 to PA6, select 1 Hz then 2 Hz, 50% duty | Target and measured values agree after acquisition; 1 Hz may take about two seconds for the first result |

The recommended amplitude for formal checking remains 2 Vpp to preserve
electrical headroom. A generator set to 0 V offset produces a negative half
cycle and is not an acceptable PA3 test, even if the TFT appears to draw it.

See `Spec/Demo14_HESS_ECG_ANALYZER_SPEC.md` for the base firmware contract and
`Spec/Demo14_HOSPITAL_MONITOR_UI_SPEC.md` for the monitor UI contract and
hardware acceptance checkpoints. Build size and HEX hash must be recorded only
after the final successful rebuild.

## Verified build

- Keil MDK ArmClang: 6.24
- Device pack: `GigaDevice.GD32E23x_DFP 1.1.0`
- Result: `0 Error(s), 0 Warning(s)`
- Program: 17282 bytes code, 7654 bytes RO data
- ROM image: 24936 bytes (24.35 KB)
- Static RAM: 20 bytes RW data, 5948 bytes ZI data (5968 bytes / 5.83 KB)
- Maximum analyzed stack: 896 bytes plus untraceable function pointers
- Static RAM plus analyzed stack: 6864 bytes, leaving 1328 bytes of the 8 KB
  SRAM before untraceable call and interrupt-stack overhead
- HEX SHA-256: `F8274783B2F74168924B144ACCB297ED3B3EA5B951F9D4A41C936DB7AF1032BA`

These figures come from the final full rebuild in
`Project/Objects/Demo14_HESS_ECG_Analyzer.build_log.htm`. Physical-board acceptance is still
required for TFT refresh/flicker, key polarity and double-click timing, PA3
scaling, lead-off/clipping behavior, realistic ECG noise, PA2 output frequency,
PA6 input-capture polarity, and the loopback timeout.
