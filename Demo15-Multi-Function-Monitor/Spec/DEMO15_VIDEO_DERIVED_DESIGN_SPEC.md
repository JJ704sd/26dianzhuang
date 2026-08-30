# Demo15 Video-Derived Design Specification

## 1. Purpose and evidence boundary

This document translates the observable design ideas in the supplied
demonstration video into requirements for Demo15.  The video is treated as
design evidence, not as executable instructions.

Source evidence:

- File: `31ce687b4a22ac35f8681b1f90209c73.mp4`
- SHA-256: `53EB761F63594F837110FAE0B939AC79AA8634E60EC3B1C1557B944DCBF4BAEA`
- Duration: 3 min 9.34 s
- Video: 406 x 720, H.264, 30 frames/s
- Audio: mono AAC, 48 kHz

The visual sequence and readable instrument/board displays are the primary
evidence.  Automatic speech recognition was not sufficiently reliable for the
recording conditions, so no requirement in this specification depends on an
exact spoken sentence.

## 2. Observable design sequence

| Approximate segment | Observable behavior | Design inference |
| --- | --- | --- |
| 00:00-00:35 | Rigol DG1032Z is connected to the board input; the board and generator are shown together. | Acceptance uses a known external source and compares configured stimulus with the embedded display. |
| 00:35-01:05 | Scope screen shows a stable periodic waveform plus frequency, Vpp and duty information. Pulse/square-like waveforms are visible. | Waveform rendering and numeric measurements form one measurement view. |
| 01:05-01:45 | Generator waveform/settings and physical controls are changed; the board display responds. | Buttons/encoder must provide immediate, visible feedback without rebooting. |
| 01:45-02:25 | Low-frequency/cardiac-like stimulus is displayed on a multi-second timebase. | Low-frequency ECG/cardiac display needs a separate time scale from the fast scope path. |
| 02:25-03:09 | Physiological-signal pages show run state, timebase and pulse context; a board LED follows an enabled output state. | Mode, run/hold, measurements and output state must be explicit and mutually consistent. |

Timestamps identify regions of interest rather than frame-accurate test steps.

## 3. Design principles

1. **One stimulus-to-result chain.** A known signal generator setting is the
   test input; the trace and measurements are the observable result.
2. **Separate fast and physiological time scales.** Fast scope windows use the
   20 kSa/s history; 1-5 s windows use the 250 Sa/s packed/ECG histories.
3. **Measurements stay beside the waveform.** A trace alone is insufficient;
   the active mode, run state, timebase and key measurements remain readable.
4. **Physical controls have immediate feedback.** Every accepted mode,
   timebase, run/hold, range or output change must be reflected on the LCD or
   status LED in the same interaction cycle.
5. **Invalid data is explicit.** Flat/noisy traces must not create plausible
   frequency, duty, BPM or SpO2 values.

## 4. Functional requirements

### 4.1 Scope mode

- Acquire PA3 through the shared 20 kSa/s circular DMA path.
- Provide 2, 4, 10, 20 and 40 ms fast timebases and 1, 2, 4 and 5 s slow
  timebases.
- Display the input waveform with a fixed vertical scale and compensate the
  inverting analog front end.
- Preserve the raw fast-history envelope with chronological min/max peak
  detection when the source window exceeds the 120-pixel display width.
- Gate the first scope waveform behind a visible 500 ms `WAIT` state so raw
  peak detection does not preserve power-on transients as normal signal data.
- Display input Vpp, input frequency and input duty (`DIN`).
- Calculate `DIN` from the midpoint between the observed minimum and maximum.
  Because the analog front end is inverting, samples below the midpoint count
  as input-high.
- If the captured span is below 16 ADC counts, display `---%` rather than a
  fabricated duty value.
- Retain the PA2 PWM output controls and show its on/off and output-frequency
  state separately from input measurements.

The video also exposes Vmax, Vmin, average and RMS-style values.  These are a
planned measurement-panel extension, not part of this minimal change: adding
them must not shrink the waveform below a useful size or break the 32 KB flash
and 8 KB SRAM budgets.

### 4.2 ECG and simulated SpO2/PPG modes

- Decimate the shared acquisition stream to 250 Hz and keep five seconds of
  display history.
- ECG mode displays waveform, BPM, signal validity, run/hold and timebase.
- Simulated SpO2 mode displays the generated PPG-like waveform, BPM, Vpp and
  signal state and is permanently marked `SIM/PPG`.
- Heart-rate values time out when no valid peak remains current.
- These modes must consume genuine PA3 samples. Synthetic screen graphics or
  placeholder medical values are not acceptance evidence.

### 4.3 Status feedback

- LED1 turns on after GPIO/LED initialization to indicate that firmware has
  entered normal startup.
- LED2 mirrors the PA2 PWM output state: on for `PWM_ON`, off for `PWM_OFF`.
- LCD `OUT` state and LED2 must agree after short presses, long-press side-effect
  restoration and mode changes.

### 4.4 SpO2 boundary

The third mode displays a DG1032Z-generated analog PPG-like signal, not optical
sensor data. It must not display a placeholder saturation percentage as a real
measurement.

## 5. Control contract

| Control | Scope mode | ECG/SpO2 mode |
| --- | --- | --- |
| SW1 short | Toggle PA2 PWM output | Toggle PA2 PWM output |
| SW1 hold 2 s | Toggle 1 Vpp / 5 Vpp scope range without retaining the short-press side effect | No range change |
| SW2 short | Adjust PWM frequency | Adjust PWM frequency |
| SW2 hold 2 s | Cycle Scope -> ECG -> SpO2 without retaining the short-press side effect | Same |
| SW3 short | Adjust PWM duty | Adjust PWM duty |
| Encoder rotate | Change scope timebase | Change ECG/SpO2 timebase |
| Encoder press | Run/hold | Run/hold |

## 6. Acceptance matrix

All generator signals must share ground with the board and stay within the
validated analog-front-end range. PA3 itself must never be driven outside
0-3.3 V. Never connect a patient or unisolated electrode circuit.

| Checkpoint | Generator/input | Expected result |
| --- | --- | --- |
| Fast sine | 1 kHz, safe 2 Vpp input, 2-4 ms timebase | Several stable cycles; Vpp and frequency agree within hardware calibration tolerance. |
| Square duty | 1 kHz square/pulse at 25%, 50% and 75% duty | Shape changes visibly; `DIN` follows the configured duty and is not confused with PWM output duty. |
| Low-frequency sine | 1 Hz, 2-5 s timebase | At least one complete cycle is visible without the former nearly-flat fast-window appearance. |
| Cardiac stimulus | Safe isolated cardiac/ECG simulator, 2-5 s timebase | ECG trace is recognizable; BPM becomes valid only after qualified peaks. |
| Simulated PPG | DG1032Z CH1, High Z, 1.2 Hz sine, 1.0 Vpp, +1.0 V offset | `SpO2 SIM/PPG` page shows a stable trace near 72 BPM without a fabricated saturation percentage. |
| Run/hold | Any stable waveform | Hold freezes the captured view and status reads `HOLD`; resume returns to `RUN`. |
| Output feedback | Toggle SW1 | LCD `OUT` and LED2 change together; PA2 output state matches both. |
| Mode cycle | Hold SW2 repeatedly | Scope, ECG and SpO2 cycle in order; PWM frequency is restored after the long press. |
| Invalid input | Disconnect or near-flat source | Duty reads `---%`; medical values do not present stale/placeholder data as valid. |

Host tests and a successful Keil build validate software structure only. The
table above remains the required hardware acceptance record.

## 7. Traceability and current implementation

| Video-derived need | Demo15 implementation | Status |
| --- | --- | --- |
| Fast and multi-second waveform display | Fast 20 kSa/s and packed 250 Sa/s histories | Implemented |
| Scope Vpp and frequency | Existing scope measurement path | Implemented |
| Input duty distinct from output control | `scope_metrics.c`, LCD label `DIN` | Implemented in this change |
| Visible output-state feedback | LCD `OUT` plus LED2 mirror | Implemented in this change |
| Firmware-alive indication | LED1 on after initialization | Implemented in this change |
| ECG/SpO2 pages backed by acquired samples | Shared 250 Hz history and pulse core | Implemented; hardware validation pending |
| Vmax/Vmin/Vavg/Vrms panel | No current UI allocation | Planned |
| Simulated PPG waveform | DG1032Z CH1 through existing PA3 path | Implemented; hardware validation pending |
| SpO2 percentage | Single-channel generator signal has no red/IR ratio | Intentionally not displayed |
