# Demo13 ECG Monitor Firmware Specification

## Objective

Provide a standalone GD32E230C8T6 ECG experiment monitor with waveform display,
R-peak detection, measured BPM, simulated ECG PWM output, signal quality, and
alarm status.

## Functional requirements

1. PA3 is sampled at 250 Hz without dynamic allocation.
2. R-peak detection applies a 250 ms refractory interval and recent RR
   smoothing; BPM becomes zero after the no-beat timeout.
3. Display history supports one through four heart periods.
4. Signal quality is reported as `GOOD`, `POOR`, `LOST`, or `UNKNOWN`.
5. Alarms report bradycardia, tachycardia, or signal loss; poor quality
   suppresses misleading heart-rate alarms.
6. PA2 produces the simulated ECG PWM envelope and its state is controllable.
7. All new paths, identifiers, project names, and UI labels use ASCII names.

## Hardware contract

- MCU: GD32E230C8T6.
- Conditioned ECG input: PA3 / ADC channel 3, 0 V through 3.3 V only.
- Simulated ECG output: PA2 / TIMER14_CH0.
- Display: SPI TFT, 160 x 128.

## Controls

- Encoder: select one through four displayed heart periods.
- KEY1: toggle simulated ECG output.
- KEY2 single/double press: increase/decrease simulated output BPM.

## Verification

- Run `Tests/run_tests.ps1`.
- Rebuild `Project/Demo13_ECG_Monitor.uvprojx` with ArmClang 6.24.
- Flash `Project/Objects/Demo13_ECG_Monitor.hex`.
- Calibrate quality thresholds with the real analog front end and verify alarms
  with board-level signals. This firmware is an educational experiment and is
  not a medical diagnostic device.
