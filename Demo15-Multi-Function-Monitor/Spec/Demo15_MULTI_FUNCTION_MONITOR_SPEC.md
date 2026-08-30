# Demo15 Multi-Function Monitor Firmware Specification

## Objective

Provide one independently buildable GD32E230C8T6 project that covers the
oscilloscope implementation, ECG monitoring implementation and the HESS ECG
analysis extension without modifying Demo12, Demo13 or Demo14.

## Functional contract

### Oscilloscope mode

- Sample PA3 at 20 kSa/s through circular DMA and retain an 800-sample fast
  history.
- Offer free-run, rising-trigger and falling-trigger views from 2 ms through
  5 s. Use the 20 kSa/s history for 2-40 ms and a packed 250 Sa/s history for
  1-5 s so low-frequency waveforms fit within the 8 KB SRAM budget.
- Show input peak-to-peak voltage, input frequency and input duty. Label input
  duty as `DIN` so it cannot be confused with the PA2 PWM output duty.
- Retain PA2 PWM enable, frequency and duty controls, and PA6 frequency capture.

### ECG monitor mode

- Decimate the shared fast acquisition to 250 Hz.
- Retain five seconds (1250 signed 8-bit samples) of ECG display history.
- Display the ECG trace, BPM, signal state and selectable timebase.
- Clear stale heart-rate measurements after a signal timeout.

### HESS analyzer mode

- Feed every 250 Hz ECG sample into the Demo14 analysis core.
- Display BPM, latest RR, RMSSD and `WAIT`, `GOOD`, `POOR`, `LEAD OFF` or
  `CLIPPED` quality.
- Show the HESS startup screen at boot and share the ECG waveform/timebase with
  ECG monitor mode instead of allocating a second history buffer.

### SpO2 status

- SpO2 acquisition and display are not implemented because no blood-oxygen
  sensor, interface pins or validated algorithm are defined in the source
  projects. Do not display placeholder measurements as real SpO2 data.

## Mode and naming contract

- Long-press SW2 for two seconds to cycle all three modes.
- Public mode identifiers use the `DEMO15_MODE_*` prefix.
- Directory names use `DemoNN-Title-Case`; Keil artifact names use ASCII
  `Demo15_Multi_Function_Monitor`.
- Demo15 has its own project, documentation, tests and output directory.

## Resource and safety contract

- Static RAM plus analyzed stack must fit in the GD32E230C8T6 8 KB SRAM with a
  documented margin after a full rebuild.
- ADC input is strictly 0 V to 3.3 V. Never directly connect a patient or an
  unisolated electrode circuit.
- No dynamic allocation is permitted in the acquisition or display path.
- LED1 indicates normal firmware startup; LED2 mirrors the PA2 PWM output state
  and must agree with the LCD `OUT` indication.

The visual rationale, evidence boundary and hardware stimulus matrix are in
`DEMO15_VIDEO_DERIVED_DESIGN_SPEC.md`.

## Verification checkpoints

1. Run `Tests/run_tests.ps1`; require all host and firmware-contract tests to
   pass.
2. Rebuild `Project/Demo15_Multi_Function_Monitor.uvprojx` with ArmClang and
   require zero errors and warnings.
3. Confirm the project links the scope view, ECG core, HESS analyzer and HESS
   splash modules and emits the correctly named HEX file.
4. Flash the board, hold SW2 repeatedly and verify the three-mode cycle.
5. In oscilloscope mode, check PA3 waveform/Vpp/input duty, PA6 frequency, PA2
   PWM and all timebases with safe sine, square and pulse inputs. Check a 1 Hz
   waveform at 2-5 s, a 1 kHz waveform at 2-4 ms, and 25/50/75% input duty.
6. In ECG and HESS modes, use an isolated safe ECG source and check waveform,
   BPM, RR, RMSSD, quality, timebase and run/hold behavior.

Steps 4 through 6 remain hardware acceptance items even when host tests and the
Keil build pass.
