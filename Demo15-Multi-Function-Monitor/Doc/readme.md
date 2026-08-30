# Demo15 Multi-Function Monitor

Demo15 combines the three electrical-training deliverables in one independent
GD32E230C8T6 project:

1. `OSCILLOSCOPE`: 20 kSa/s PA3 acquisition for 2-40 ms views plus a packed
   250 Sa/s history for 1-5 s views, trigger/free-run display, Vpp and frequency
   measurement, raw-noise peak-detect display, and the PA2 PWM reference output.
2. `ECG_MONITOR`: a 250 Hz ECG history, heartbeat detection, BPM display and
   one-to-five-second timebase views.
3. `SPO2_MONITOR`: a PA3 laboratory monitor for a RIGOL DG1032Z simulated
   PPG/blood-oxygen waveform, with waveform, BPM, Vpp and signal state.

The third mode is explicitly marked `SIM/PPG`. It does not use an optical
sensor and does not fabricate an oxygen-saturation percentage from a single
analog channel.

The folder uses `DemoNN-Title-Case`; the Keil project and output use
`Demo15_Multi_Function_Monitor` so paths remain ASCII and tool-friendly.

## Controls

- Hold `SW2` for two seconds: cycle `OSCILLOSCOPE -> ECG_MONITOR ->
  SPO2_MONITOR -> OSCILLOSCOPE`.
- Rotate the encoder: change the oscilloscope timebase from 2 ms through 5 s,
  or change the ECG/SpO2 timebase.
- Hold `SW1` for two seconds in `OSCILLOSCOPE`: toggle 5 Vpp/1 Vpp display
  range.
- Press `SW1`: enable or disable PA2 PWM.
- Press `SW2`: change PWM frequency.
- Press `SW3`: change PWM duty.
- Press `KEYD`: run or hold the displayed waveform.

Long-press actions suppress the corresponding short-press result.

For raw noise observation, use a 2-40 ms oscilloscope timebase. Fast windows
retain the 20 kSa/s ADC samples and use min/max peak detection when compressed
to the LCD width. Hold `SW1` to select the 1 Vpp range for low-amplitude noise.
The 1-5 s views are averaged to 250 Sa/s for low-frequency/physiological
signals and therefore intentionally do not preserve wideband raw noise.

## Hardware contract

- MCU: GD32E230C8T6.
- Analog input: PA3 / ADC channel 3, strictly 0 V through 3.3 V.
- PWM reference output: PA2 / TIMER14_CH0.
- Frequency input capture: PA6 / TIMER2_CH0.
- Display: 160 x 128 SPI TFT.

Use an ECG simulator or isolated analog-front-end output with common ground.
Do not connect a patient, an unisolated electrode circuit, negative voltage or
more than 3.3 V directly to PA3. This project is not a medical device.

For the simulated SpO2 mode, connect DG1032Z CH1 to the board BNC input, set
the generator load to High Z and begin with the profiles in
`Spec/DG1032Z_SPO2_SIM_MODE_SPEC.md`.

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
- Program size: Code 16986 B, RO data 6526 B.
- Image RAM: RW 24 B plus ZI 6472 B = 6496 B. The ZI total includes the
  configured 1536 B stack, leaving 1696 B of the 8 KB SRAM unallocated.
- The previous call-graph estimate used at most 320 B of stack, with the usual
  caveat for indirect calls and interrupt nesting; it fits inside the reserved
  stack.
- HEX SHA-256:
  `768072A2A77BD0347D37052B066369D4B958FDD90F8823C8A0B027A09830D6F3`.

These values come from a full rebuild. Physical-board acceptance is still
required.
