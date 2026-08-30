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
- Use uniformly spaced chronological samples for ordinary fast-scope pages.
  Preserve the raw fast-history envelope with chronological min/max peak
  detection only in the dedicated 200 mVpp `NOISE` page.
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
- Animate free-run by selecting adjacent continuous history windows. Never
  rotate samples inside a window or connect its newest end to its oldest end.
  If the history has no spare samples beyond the displayed span, show the
  newest continuous window without artificial movement.

The video also exposes Vmax, Vmin, average and RMS-style values.  These are a
planned measurement-panel extension, not part of this minimal change: adding
them must not shrink the waveform below a useful size or break the 32 KB flash
and 8 KB SRAM budgets.

### 4.2 ECG and simulated SpO2/PPG modes

- Decimate the shared acquisition stream to 250 Hz and keep five seconds of
  display history.
- ECG mode generates a 60/80 BPM PWM-DAC envelope, then displays the genuinely
  recaptured PA3 waveform, measured BPM, signal validity, run/hold and timebase.
- Simulated SpO2 mode displays the received waveform, BPM, Vpp and signal state
  and is permanently marked `SIM` plus `DUT` or `2CH`.
- Heart-rate values time out when no valid peak remains current.
- Draw ECG/PPG incrementally with a fixed left-to-right column cursor and a
  per-column min/max envelope. Normal samples may update only the active plot
  column; the numeric side and bottom fields change only when their displayed
  value changes. Wrapping must not connect the right and left plot edges.
- These modes must consume genuine PA3 samples. Synthetic screen graphics or
  placeholder medical values are not acceptance evidence.

### 4.3 Status feedback

- LED1 turns on after GPIO/LED initialization to indicate that firmware has
  entered normal startup.
- LED2 mirrors the PA2 PWM output state: on for `PWM_ON`, off for `PWM_OFF`.
- LCD `OUT` state and LED2 must agree after short presses, long-press side-effect
  restoration and mode changes.

### 4.4 SpO2 boundary

The third mode defaults to an explicit CH1 square-wave protocol in which duty
maps linearly to the 70-100% teaching display range. Tagged RED/IR input may use
the ratio-of-ratios path. Both are simulation estimates, not clinical results.

## 5. Control contract

| Control | Scope general | Scope `S-ECG` / ECG mode | SpO2 mode |
| --- | --- | --- | --- |
| SW1 short | Toggle raw PA2 PWM | Toggle ECG PWM-DAC output | Toggle raw PA2 PWM |
| SW1 hold 2 s | Cycle 5 Vpp / 1 Vpp / 200 mVpp without retaining the short action | No range change | No range change |
| SW2 short | Adjust raw PWM frequency | No action | Adjust raw PWM frequency |
| SW2 hold 2 s | Cycle Scope -> ECG -> SpO2 without retaining the short action | Same | Same |
| SW3 short | Adjust raw PWM duty | Toggle 60/80 BPM | Adjust raw PWM duty |
| SW3 hold 2 s | Toggle Scope general / `S-ECG` without retaining the short action | Return to Scope general when already in `S-ECG`; no special action in ECG mode | No special action |
| Encoder rotate | Change scope timebase | Change ECG timebase | Change PPG timebase |
| Encoder press | Run/hold | Run/hold | Run/hold |

## 6. Acceptance matrix

All generator signals must share ground with the board and stay within the
validated analog-front-end range. PA3 itself must never be driven outside
0-3.3 V. Never connect a patient or unisolated electrode circuit.

| Checkpoint | Generator/input | Expected result |
| --- | --- | --- |
| Fast sine | 1 kHz, safe 2 Vpp input, 2-4 ms timebase | Several stable cycles; Vpp and frequency agree within hardware calibration tolerance. |
| Square duty | 1 kHz square/pulse at 25%, 50% and 75% duty | Shape changes visibly; `DIN` follows the configured duty and is not confused with PWM output duty. |
| Low-frequency sine | 1 Hz, 2-5 s timebase | At least one complete cycle is visible without the former nearly-flat fast-window appearance. |
| Self-generated ECG | PA2 through verified low-pass to PA3, common ground; select 60 then 80 BPM | ECG trace is recognizable; measured BPM becomes valid only after qualified peaks and approaches the selected target. |
| Scope ECG bridge | Hold SW3 in Scope with the ECG loopback connected | `S-ECG` reuses the incremental sweep trace; returning preserves normal scope settings. |
| Simulated PPG | CH1 square at exactly 1 or 2 Hz and 25/50/75% duty, or PA3 RED/IR plus PA6 tag | `SIM/DUT` shows 78/85/93%; `SIM/2CH` shows the ratio-derived percentage. |
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
| PWM-generated ECG and scope bridge | 16.95 kHz phase-walking PA2 PWM envelope, 60/80 presets, direct board-front-end loopback, explicit `S-ECG` subview | Implemented; loopback/board validation pending |
| Vmax/Vmin/Vavg/Vrms panel | No current UI allocation | Planned |
| Simulated PPG waveform | DG1032Z CH1 through existing PA3 path | Implemented; hardware validation pending |
| Simulated SpO2 percentage | `spo2_receiver.c` pre-reconstruction ADC-code duty window plus optional `spo2_core.c` RED/IR ratio | DUT and 2CH teaching estimates implemented; physical validation pending |
