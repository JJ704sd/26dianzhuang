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
- `WAVE`: 156 x 93 pixel enlarged ECG plot with a compact BPM/status strip.
- Both waveform areas use sparse minor intersections, continuous major grid
  lines, a highlighted center reference, and a clear plot border. This grid is
  for visual comparison only and is not calibrated ECG paper.
- `FREQ`: displays PA2 PWM target frequency, PA6 captured frequency, loopback
  error, explicit `MATCH`/`MISMATCH` status, output state, and `NO SIGNAL` after
  a 1500 ms capture timeout. The match status checks the loopback signal path;
  it is not an independent oscillator calibration.
- `SW1` short press: run/freeze.
- `SW2` short press: cycle x1/x2/x4 gain.
- `SW2` double press: cycle `MONITOR`, `WAVE`, and `FREQ` pages.
- `SW3` short press: add an event marker; double press: reset measurements.
- Rotary encoder: select a 2/5/10 second time window.
- On `FREQ`, rotary encoder selects 100/250/500/1000/2000 Hz; encoder push
  (`KEYD`) toggles the PA2 PWM output. PWM starts disabled.
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

The dynamic UI uses incremental waveform restoration and updates numeric fields
only when their values change. Large solid regions are sent as one continuous
SPI color burst, and chip select is released only after the final SPI bit has
shifted out. A heart-rate result becomes unavailable after three seconds with
no valid R peak, and invalid lead-off/clipped input never presents an old BPM as
a current reading.

The plotted waveform now uses the raw ADC sample centered around 2048, while
heart-rate detection continues to use the baseline-suppressed and smoothed ECG
signal. This separation preserves the flat high and low plateaus of a square
wave without weakening the ECG analysis path. Chinese font indexes are stored
as explicit GBK byte escapes, and host checks cover every Chinese label used by
the UI plus the readable structure of `示` in `示波`.

See `Spec/Demo14_HESS_ECG_ANALYZER_SPEC.md` for the base firmware contract and
`Spec/Demo14_HOSPITAL_MONITOR_UI_SPEC.md` for the two-page UI contract and
hardware acceptance checkpoints. Build size and HEX hash must be recorded only
after the final successful rebuild.

## Verified build

- Keil MDK ArmClang: 6.24
- Device pack: `GigaDevice.GD32E23x_DFP 1.1.0`
- Result: `0 Error(s), 0 Warning(s)`
- Program: 15774 bytes code, 7614 bytes RO data
- ROM image: 23388 bytes (22.84 KB)
- RAM: 20 bytes RW data, 4628 bytes ZI data (4648 bytes / 4.54 KB total)
- Maximum analyzed stack: 768 bytes plus untraceable function pointers
- HEX SHA-256: `2ED0FC9C08104E87A32BAB338A7EECB7D875BBD592BD08A56457BA29D2865D74`

These figures come from the final full rebuild in
`Project/Demo14_Square_Chinese_Fix_rebuild.log`. Physical-board acceptance is still
required for TFT refresh/flicker, key polarity and double-click timing, PA3
scaling, lead-off/clipping behavior, realistic ECG noise, PA2 output frequency,
PA6 input-capture polarity, and the loopback timeout.
