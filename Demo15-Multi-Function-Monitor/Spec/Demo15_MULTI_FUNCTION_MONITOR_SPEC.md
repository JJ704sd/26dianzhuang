# Demo15 Multi-Function Monitor Firmware Specification

## Objective

Provide one independently buildable GD32E230C8T6 project that covers the
oscilloscope implementation, ECG monitoring implementation and a simulated
SpO2/PPG signal monitor without modifying Demo12, Demo13 or Demo14.

## Functional contract

### Oscilloscope mode

- Sample PA3 at 20 kSa/s through circular DMA and retain an 800-sample fast
  history.
- Offer free-run, rising-trigger and falling-trigger views from 2 ms through
  5 s. Use the 20 kSa/s history for 2-40 ms and a packed 250 Sa/s history for
  1-5 s so low-frequency waveforms fit within the 8 KB SRAM budget.
- Show input peak-to-peak voltage, input frequency and input duty. Label input
  duty as `DIN` so it cannot be confused with the PA2 PWM output duty.
- Preserve raw ADC samples in the 2-40 ms fast-scope history. Ordinary scope
  ranges shall use uniformly spaced chronological points when compressed so
  square-wave duration is not replaced by min/max pairs. Only the dedicated
  `NOISE` range shall use chronological min/max peak detection so narrow noise
  excursions remain visible.
- After reset, acquire 10000 raw samples (500 ms at 20 kSa/s) before drawing
  the first scope waveform. Display `WAIT` during this one-time settle period
  so power-on/front-end transients do not set the apparent waveform height.
  Mode changes must not restart the settle period.
- The 1-5 s slow history uses 80-sample averaging and 8-bit packing to fit the
  SRAM budget. It is intended for low-frequency signals, not raw wideband-noise
  inspection.
- When copying the 1-5 s history for display and measurements, apply a five-
  sample median filter so isolated one- or two-sample front-end/ADC glitches do
  not inflate Vpp or appear as needles on a low-frequency square wave. Do not
  apply this filter to the 2-40 ms raw fast-scope path.
- Retain PA2 PWM enable, frequency and duty controls, and PA6 frequency capture.
- Provide an explicit `S-ECG` scope subview selected by a two-second SW3 hold.
  It shall reuse the ECG history and renderer without allocating another
  1250-sample buffer, and shall preserve the normal scope timebase/range/view.
- In free-run, move the view by selecting a bounded historical lag from the
  available samples. Every frame shall remain one continuous chronological
  window. Never rotate one captured window and splice its newest end back to
  its oldest sample, because that creates a non-physical square-wave edge.
  When the selected span fills all available history, fall back to the newest
  continuous window instead of fabricating motion.
  Provide 5 Vpp, 1 Vpp and 200 mVpp vertical ranges; cycle them with a
  two-second SW1 hold without retaining the PWM short-press side effect.
- Treat the 200 mVpp range as the dedicated `NOISE` view. Reclaim the normal
  OUT/FOUT side panel to expand the waveform from 120 to 156 horizontal pixels
  while keeping the bottom Vpp, FIN and DIN measurements. The 5 Vpp/1 Vpp,
  ECG and SpO2 layouts shall remain unchanged, and a range change shall redraw
  the static layout immediately.

### ECG monitor mode

- Generate a P-QRS-T duty envelope on a PA2 PWM-DAC carrier with 59 timer
  ticks at 1 MHz (approximately 16.95 kHz). The carrier period shall be
  coprime with the 50-tick TIMER0 ADC interval so 80-sample averaging sees all
  carrier phases instead of repeatedly sampling one phase. Service the
  generator from the existing application 1 ms callback; do not register a
  second callback on TIMER15.
- Provide only two acceptance presets: 60 BPM = 1000 ms = 1 Hz and
  80 BPM = 750 ms = 1.333 Hz. The envelope repetition period sets BPM; PWM
  duty represents instantaneous waveform amplitude.
- Self-capture through the physical laboratory loopback `PWM H3/TP9 -> BNC
  center/VIN -> board analog front end -> ADC PA3`. The supplied PCB has no
  dedicated PWM low-pass on this path, so firmware decimation must average the
  phase-walking carrier. The ADC node remains subject to 0-3.3 V verification.
- Decimate the shared fast acquisition to 250 Hz.
- Retain five seconds (1250 signed 8-bit samples) of ECG display history.
- Display the ECG trace, BPM, signal state and selectable timebase.
- Draw with a fixed left-to-right incremental sweep cursor. Accumulate each
  timebase's 250 Hz samples into exactly 120 display-column buckets, preserving
  the minimum and maximum of every bucket. Clear and redraw only the current
  waveform column; never join the final right-edge point to the new left edge.
- Do not clear or redraw the whole screen during normal acquisition. Rewrite
  right-side measurements and the bottom status fields only when their value
  changes. A mode/timebase change or run-resume may clear only the waveform
  plot and restart the sweep.
- Clear stale heart-rate measurements after a signal timeout.

### Simulated SpO2/PPG monitor mode

- Accept safe RED/IR PPG samples on PA3 with PA6 tagging the active wavelength
  (`LOW=RED`, `HIGH=IR`).
- When PA6/paired data is unavailable, provide an explicit `DUT` simulator
  result from the PA3 waveform duty cycle. For a non-degenerate two-level
  window, map duty to the 70-100% teaching range; 25%, 50% and 75% produce
  78%, 85% and 93%. Flat 0%/100% windows are invalid.
- Display the acquired IR waveform, calculated simulated SpO2 percentage, BPM,
  Vpp, signal state and selectable 1-5 s timebase.
- Mark the page permanently as `SIM` plus `WAIT/DUT/2CH`, and share the ECG waveform history
  instead of allocating a second buffer.
- Retain the existing startup animation.
- For `DUT`, use an exact 1.000 Hz or 2.000 Hz square wave so the two-second
  window contains complete cycles; safe amplitude, offset and phase may vary.
- Calculate `R=(ACred/DCred)/(ACir/DCir)` and use the teaching approximation
  `SpO2=110-25R`; reject incomplete, flat, rail-clipped or implausible windows.

### SpO2 calibration boundary

- Ratio-of-ratios measurement requires both RED and IR PPG data. A single
  untagged trace can produce only the explicitly labelled duty-coded simulator
  value, not a measured saturation.
- `110-25R` is a common teaching approximation; clinical accuracy requires an
  empirical calibration curve for the specific optical/simulator system.

## Mode and naming contract

- Long-press SW2 for two seconds to cycle all three modes.
- Long-press SW3 for two seconds in oscilloscope mode to toggle `S-ECG`.
- In ECG pages, SW1 controls generation, SW3 toggles 60/80 BPM, the encoder
  controls ECG span, and KEYD controls display run/hold. SW2 short press shall
  not alter the ECG preset.
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
3. Confirm the project links the scope view, ECG/pulse core, retained startup
   animation and emits the correctly named HEX file.
4. Flash the board, hold SW2 repeatedly and verify the three-mode cycle.
   Immediately after reset, verify that Scope shows `WAIT` briefly and then
   enters `RUN` with the same settled height seen after cycling modes.
5. In oscilloscope mode, check PA3 waveform/Vpp/input duty, PA6 frequency, PA2
   PWM and all timebases with safe sine, square and pulse inputs. Check a 1 Hz
   waveform at 2-5 s, a 1 kHz waveform at 2-4 ms, and 25/50/75% input duty.
   At 4 s, apply a 2 Hz, 50% duty square/pulse and verify that both plateaus
   remain flat without isolated needles or a moving window-seam edge; `DIN`
   should remain near 50%. For a 1 kHz square wave use 2-4 ms; long 1-5 s
   windows are intentionally limited to the 250 Sa/s low-frequency path.
   With DG1032Z noise output, verify that 2-40 ms fast timebases retain narrow
   positive and negative excursions; use the 200 mVpp range for small noise
   and verify that the title changes to `NOISE`, the plot reaches the right
   side of the screen without stale OUT/FOUT text, the Vpp/FIN/DIN row remains
   readable, and the trace visibly advances in free-run. Cycle back to 1 Vpp
   and confirm the standard side panel is restored.
6. For self-generated ECG, first scope PA2 and confirm an approximately
   16.95 kHz carrier whose duty follows the P-QRS-T envelope. Then connect the
   PWM header/test point to BNC center/VIN, select the correct input path,
   verify ADC/PA3 remains in 0-3.3 V, and check both 60 BPM
   (1000 ms) and 80 BPM (750 ms). Confirm the trace advances one column at a
   time from left to right, wraps without a cross-screen line, and does not
   flash or clear the parameter areas. Confirm measured BPM approaches the
   selected target after enough beats, and unchanged values remain stable.
   Check SW1 output enable, SW3 preset, timebase and KEYD run/hold.
7. In oscilloscope mode, hold SW3 to enter `S-ECG`; confirm the same captured
   ECG waveform/BPM is displayed, then return and confirm the prior scope
   timebase, vertical range and trigger/free-run setting are retained.
8. In simulated SpO2 mode, select a safe 1.000 Hz or 2.000 Hz CH1 square wave, then check
   25%, 50% and 75% duty. After each 500-sample window verify `SIM/DUT` and
   78%, 85% and 93%; a flat or disconnected input must return `---/WAIT`.
   Then provide paired RED/IR data on PA3 and the wavelength
   tag on PA6. Exercise multiple ratios and variable safe waveform parameters;
   check `O2%`, `SpO2`, `SIM`, `PPG`, waveform, BPM, Vpp, signal state and
   run/hold. Verify a valid pair changes the source to `2CH`; disconnecting PA6
   returns to the clearly marked `DUT` path rather than a false 2CH value.

Steps 4 through 8 remain hardware acceptance items even when host tests and the
Keil build pass.
