# Demo15 Multi-Function Monitor

Demo15 combines the three electrical-training deliverables in one independent
GD32E230C8T6 project:

1. `OSCILLOSCOPE`: 20 kSa/s PA3 acquisition for 2-40 ms views plus a packed
   250 Sa/s history for 1-5 s views, trigger/free-run display, Vpp and frequency
   measurement, and the PA2 PWM reference output.
2. `ECG_MONITOR`: a 250 Hz ECG history, heartbeat detection, BPM display and
   one-to-five-second timebase views.
3. `HESS_ANALYZER`: the extended Demo14 analysis core with BPM, RR, RMSSD and
   signal-quality classification.

SpO2 is not implemented in this firmware. The repository contains no blood-
oxygen sensor driver, bus/pin contract or SpO2 algorithm, so the UI does not
show fabricated oxygen-saturation values. A real SpO2 mode requires the exact
sensor module and wiring to be defined first.

The folder uses `DemoNN-Title-Case`; the Keil project and output use
`Demo15_Multi_Function_Monitor` so paths remain ASCII and tool-friendly.

## Controls

- Hold `SW2` for two seconds: cycle `OSCILLOSCOPE -> ECG_MONITOR ->
  HESS_ANALYZER -> OSCILLOSCOPE`.
- Rotate the encoder: change the oscilloscope timebase from 2 ms through 5 s,
  or change the ECG/HESS timebase.
- Hold `SW1` for two seconds in `OSCILLOSCOPE`: toggle 5 Vpp/1 Vpp display
  range.
- Press `SW1`: enable or disable PA2 PWM.
- Press `SW2`: change PWM frequency.
- Press `SW3`: change PWM duty.
- Press `KEYD`: run or hold the displayed waveform.

Long-press actions suppress the corresponding short-press result.

## Hardware contract

- MCU: GD32E230C8T6.
- Analog input: PA3 / ADC channel 3, strictly 0 V through 3.3 V.
- PWM reference output: PA2 / TIMER14_CH0.
- Frequency input capture: PA6 / TIMER2_CH0.
- Display: 160 x 128 SPI TFT.

Use an ECG simulator or isolated analog-front-end output with common ground.
Do not connect a patient, an unisolated electrode circuit, negative voltage or
more than 3.3 V directly to PA3. This project is not a medical device.

## Build and verification

Run:

```powershell
& .\Tests\run_tests.ps1
```

Then rebuild `Project/Demo15_Multi_Function_Monitor.uvprojx` and flash
`Project/Objects/Demo15_Multi_Function_Monitor.hex`. Host tests and a clean
Keil build do not replace the final on-board checks listed in the specification.

See `Spec/Demo15_MULTI_FUNCTION_MONITOR_SPEC.md` for acceptance checkpoints and
`Spec/DEMO15_VIDEO_DERIVED_DESIGN_SPEC.md` for the video-derived rationale,
evidence boundary and signal-generator test matrix.

## Verified build

- ArmClang 6.24 with size optimization (`-Oz`).
- Device pack: `GigaDevice.GD32E23x_DFP 1.1.0`.
- Result: `0 Error(s), 0 Warning(s)`.
- Program size: Code 17318 B, RO data 6550 B.
- Image RAM: RW 24 B plus ZI 6472 B = 6496 B. The ZI total includes the
  configured 1536 B stack, leaving 1696 B of the 8 KB SRAM unallocated.
- The previous call-graph estimate used at most 320 B of stack, with the usual
  caveat for indirect calls and interrupt nesting; it fits inside the reserved
  stack.
- HEX SHA-256:
  `B1038CD8F3B3FFAB2B17B67D813B050F39E6770C4C4F95453033A3D9C34E78AD`.

These values come from a full rebuild. Physical-board acceptance is still
required.
