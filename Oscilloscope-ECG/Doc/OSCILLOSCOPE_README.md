# Oscilloscope Feature

## Scope

The oscilloscope path samples PA3 with ADC and single-frame DMA, converts one
frame to display coordinates, selects a stable 100-sample window, and renders
the waveform on the TFT. The implementation uses only static storage and
integer trigger calculations, so it remains suitable for GD32E230C8T6.

## Auto Trigger

`osc_window_find_auto` improves display stability without adding a buffer:

- derives trigger levels from the current frame minimum and maximum;
- uses 25 percent hysteresis around the frame midpoint to reject midpoint
  noise;
- retains 25 samples before the trigger so the edge is visible in context;
- returns the first complete window for flat or non-triggering frames;
- clamps a late trigger so the selected window never exceeds the frame.

The existing `osc_window_find` entry point remains available for callers that
need an explicit threshold.

## Checkpoints

1. Run `Tests/run_tests.ps1` and confirm `osc_window tests passed`.
2. Confirm the firmware contract still requires single-frame DMA.
3. Rebuild `Project/Oscilloscope.uvprojx` with the GD32E230C8T6 target and
   require zero errors and zero warnings.
4. Flash the generated HEX and verify PA3 with a low-amplitude, offset input,
   then verify a noisy midpoint does not make the trace jump horizontally.
5. Verify Run/Pause, encoder horizontal scaling, peak-to-peak voltage, input
   frequency, and PWM controls on the target board.

Host tests validate bounds and trigger behavior but do not replace the final
TFT, ADC front-end, key polarity, and signal-integrity checks on hardware.
