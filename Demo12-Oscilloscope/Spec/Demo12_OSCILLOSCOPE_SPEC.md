# Demo12 Oscilloscope Firmware Specification

## Objective

Provide a standalone GD32E230C8T6 oscilloscope that samples PA3, renders the
waveform on the 160 x 128 TFT, measures peak-to-peak voltage and input
frequency, and optionally drives the PA2 PWM reference output.

## Functional requirements

1. ADC acquisition uses one-shot DMA frames and never overwrites a frame while
   it is being processed.
2. Trigger selection uses an adaptive midpoint, hysteresis against noise, a
   25-sample pre-trigger region, and a safe fallback for flat signals.
3. The encoder changes the horizontal sampling step from 1 through 10.
4. KEYD pauses or resumes waveform acquisition.
5. KEY1 toggles PA2 PWM, KEY2 changes PWM frequency, and KEY3 changes duty.
6. All new paths, identifiers, project names, and UI labels use ASCII names.

## Hardware contract

- MCU: GD32E230C8T6.
- ADC input: PA3 / ADC channel 3, maximum 3.3 V.
- PWM output: PA2 / TIMER14_CH0.
- Frequency input capture: PA6 / TIMER2_CH0.
- Display: SPI TFT, 160 x 128.

## Verification

- Run `Tests/run_tests.ps1`.
- Rebuild `Project/Demo12_Oscilloscope.uvprojx` with ArmClang 6.24.
- Flash `Project/Objects/Demo12_Oscilloscope.hex`.
- On board, verify trigger stability, pause/resume, control polarity, PA2, PA3,
  PA6, TFT refresh, and signals near 0 V and 3.3 V.
