# Demo15 Multi-Function Monitor

Demo15 combines the three electrical-training deliverables in one independent
GD32E230C8T6 project:

1. `OSCILLOSCOPE`: 20 kSa/s PA3 acquisition for 2-40 ms views plus a packed
   250 Sa/s history for 1-5 s views, trigger/free-run display, Vpp and frequency
   measurement, raw-noise peak-detect display, and the PA2 PWM reference output.
2. `ECG_MONITOR`: a self-generated PWM-DAC ECG envelope, 250 Hz captured ECG
   history, heartbeat detection, BPM display and one-to-five-second views.
   The internal presets are exactly 60 BPM (1000 ms / 1 Hz) and 80 BPM
   (750 ms / 1.333 Hz).
3. `SPO2_MONITOR`: a simulated receiver with waveform, BPM, Vpp, signal state
   and either a CH1 duty-coded estimate or an optional RED/IR estimate.

The third mode is explicitly marked `SIM`. Its default `DUT` path maps the
measured CH1 duty cycle into the 70-100% teaching range; an optional `2CH` path
uses `SpO2 = 110 - 25R`. Neither path is a clinical oxygen measurement.

The folder uses `DemoNN-Title-Case`; the Keil project and output use
`Demo15_Multi_Function_Monitor` so paths remain ASCII and tool-friendly.

## Controls

- Hold `SW2` for two seconds: cycle `OSCILLOSCOPE -> ECG_MONITOR ->
  SPO2_MONITOR -> OSCILLOSCOPE`.
- Rotate the encoder: change the oscilloscope timebase from 2 ms through 5 s,
  or change the ECG/SpO2 timebase.
- Hold `SW1` for two seconds in `OSCILLOSCOPE`: cycle the vertical display
  range through 5 Vpp, 1 Vpp and 200 mVpp. The 200 mVpp range opens the
  `NOISE` view, expanding the plot from 120 to 156 horizontal pixels while
  retaining the Vpp, input-frequency and input-duty readouts.
- Hold `SW3` for two seconds in `OSCILLOSCOPE`: toggle between the normal
  scope page and the `S-ECG` subview. The subview retains the normal scope
  settings and reuses the existing 250 Sa/s ECG history.
- Press `SW1`: enable or disable PA2 PWM.
- Press `SW2`: change PWM frequency.
- Press `SW3`: change PWM duty.
- Press `KEYD`: run or hold the displayed waveform.

In `ECG_MONITOR` and the scope `S-ECG` subview, the controls are deliberately
mode-specific: `SW1` enables/disables ECG generation, `SW3` selects 60 or
80 BPM, the encoder changes the ECG timebase, and `KEYD` holds only the
display. `SW2` has no short-press ECG action, so its two-second mode switch
cannot accidentally change the heart-rate preset.

Long-press actions suppress the corresponding short-press result.

For raw noise observation, use a 2-40 ms oscilloscope timebase. Fast windows
retain the 20 kSa/s ADC samples. Ordinary 5 Vpp/1 Vpp scope pages use uniform,
chronological display samples so compressed square waves keep their time
weighting and never join the newest sample back to the oldest sample. The
dedicated 200 mVpp `NOISE` page uses min/max peak detection when compressed
to the LCD width. Hold `SW1` repeatedly to select that range for small
noise; this is 25 times the vertical sensitivity of the 5 Vpp range. Return to
1 Vpp or 5 Vpp before applying a larger signal so the trace does not clip.
The `NOISE` page reclaims the normal OUT/FOUT side panel for a 30% wider raw
waveform display; the 1 Vpp/5 Vpp scope pages and ECG/SpO2 pages are unchanged.
The 1-5 s views are averaged to 250 Sa/s for low-frequency/physiological
signals and therefore intentionally do not preserve wideband raw noise.
Their display/measurement copy applies a five-sample median filter, removing
isolated glitches up to 8 ms wide without changing a 1-2 Hz square-wave
plateau. The 2-40 ms raw path remains unfiltered.

In scope `RUN`, the displayed historical lag advances within the samples that
are actually available and then reverses at the boundary. Every frame is one
chronological acquisition window; samples are never rotated inside a frame,
so square-wave plateaus cannot acquire a synthetic moving seam edge.

ECG and simulated PPG use a fixed left-to-right sweep cursor. Each new 250 Hz
sample contributes to one display-column bucket; the renderer clears and
updates only the emitted column (including its min/max envelope) and leaves a
one-column gap at the cursor. A wrap starts at the left without connecting the
right edge back to the left. The right parameter column and bottom status row
are rewritten only when the corresponding value changes. `KEYD` freezes the
view; resume restarts a clean sweep in the waveform area.

After reset, Scope displays `WAIT` for the first 500 ms of ADC acquisition and
does not draw that startup interval. This prevents front-end/power-on
transients from appearing as an abnormally tall first waveform. The settle
gate runs once per reset and is not repeated when cycling modes.

## Hardware contract

- MCU: GD32E230C8T6.
- Analog input: PA3 / ADC channel 3, strictly 0 V through 3.3 V.
- PWM reference output: PA2 / TIMER14_CH0.
- Frequency input capture: PA6 / TIMER2_CH0.
- Display: 160 x 128 SPI TFT.

Use an ECG simulator or isolated analog-front-end output with common ground.
Do not connect a patient, an unisolated electrode circuit, negative voltage or
more than 3.3 V directly to PA3. This project is not a medical device.

For the self-generated ECG loopback, connect the board's `PWM` output
(`PA2`, `H3` or `TP9`) to the BNC center/VIN input and select the appropriate
input-switch position. The PCB netlist shows that this route passes through the
existing analog front end before reaching ADC/PA3; it does not contain a
dedicated PWM low-pass. AGND and digital GND are already joined on this board,
but verify the ADC-side node remains within 0-3.3 V.

The PWM-DAC carrier is therefore 1 MHz / 59 = approximately 16.95 kHz instead
of 20 kHz. TIMER0 samples every 50 us (20 kSa/s); using the former 50-tick
carrier sampled the same PWM phase repeatedly and produced the dense aliasing
seen on the ECG page. The coprime 59-tick carrier lets each 80-conversion,
250 Hz ECG sample average across the carrier phases and recover duty without an
added RC. Duty encodes instantaneous P-QRS-T amplitude; the 1000/750 ms
envelope period, not carrier frequency or duty alone, sets heart rate.

The SpO2 page supports two explicitly distinguished laboratory paths. For the
CH1-only path, change CH1 from `Pulseilogram`/`ResSpeed` to `Square`, select
exactly 1.000 Hz or 2.000 Hz, and set duty on CH1 (not the disconnected CH2).
It shows `DUT`, measures a 500-sample window, and applies
`SIM% = 70 + 30*duty/100`.
Thus 25%, 50% and 75% duty display 78%, 85% and 93%. With genuine paired
RED/IR samples on PA3 and a valid PA6 wavelength tag (`LOW=RED`, `HIGH=IR`),
the page changes to `2CH` and displays the ratio-derived result.

Both inputs must remain inside 0-3.3 V and share ground. A disconnected PA6 is
held low internally. For a 2CH test, do not use the photographed CH2 setting of
1 Vpp with +1 V offset: its 0.5-1.5 V range is not a reliable 3.3 V GPIO high.
Use a physically connected 0-3 V square tag with at least 20 ms dwell per
wavelength, and provide genuinely time-multiplexed RED/IR analog data on PA3.
See
`Spec/DG1032Z_SPO2_SIM_MODE_SPEC.md` for the receiver contract.

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
- Program size: Code 21246 B, RO data 6582 B.
- Image RAM: RW 28 B plus ZI 6732 B = 6760 B. The ZI total includes the
  configured 1536 B stack, leaving 1432 B of the 8 KB SRAM unallocated.
- The call-graph estimate uses at most 400 B of stack, with the usual
  caveat for indirect calls and interrupt nesting; it fits inside the reserved
  stack.
- HEX SHA-256:
  `F6B0B0D21B4F05423A8D3C545429F8F357EB047641A6737681E2230A267554C7`.

These values come from a full rebuild. Physical-board acceptance is still
required.
