# Demo14 ECG Monitor UI Specification

## Pages

- `MONITOR`: compact ECG trace plus BPM, RR, RMSSD, signal quality, gain,
  2/5-second window, run/freeze state and event marker.
- `WAVE`: a 156 x 93 enlarged ECG trace using the same 250 Hz acquisition
  history. It starts at two seconds so a normal 1 Hz-class heartbeat is visible.

There is no generator or frequency page in Demo14. The UI must not require a
PWM output, frequency-capture input or high-speed oscilloscope DMA stream.

## Controls

- `SW1` short press: toggle live acquisition and freeze.
- `SW2` short press: cycle x1, x2 and x4 vertical gain.
- `SW2` double press: toggle `MONITOR` and `WAVE`, then redraw immediately.
- `SW3` short press: add an event marker; double press: reset measurements.
- Rotary encoder: select a two- or five-second display window.

## Rendering contract

- Both pages consume the signed one-byte-per-sample ECG ring buffer.
- The enlarged plot is bounded by x=2..157 and y=17..109.
- The display mapper centers each visible frame, preserves a two-pixel margin
  and marks automatic gain limiting with `F`.
- A changed live waveform is rendered at the 40 ms scheduler cadence with one
  SPI address window per scanline. Numeric fields redraw only when changed.
- The renderer tracks the latest 32-bit acquisition revision. It must not keep
  a second full `int16_t` plot solely for change detection.
- Chinese text uses explicit GBK byte arrays; ASCII text uses supported 16- or
  24-pixel fonts. All coordinates remain inside 160 x 128.

## Acceptance

Host tests cover bounds, null handling, refresh work, source contracts and the
ECG core. Physical acceptance additionally requires a safe ECG simulator or
isolated analog-front-end output on PA3, common ground, verification of the
two-second and five-second views, key polarity/double-click timing, gain/FIT,
quality status and visible refresh smoothness.
