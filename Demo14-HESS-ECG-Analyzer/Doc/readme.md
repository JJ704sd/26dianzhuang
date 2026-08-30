# Demo14 HESS ECG Analyzer

Demo14 is an ECG-only GD32E230C8T6 demonstration. It samples PA3 at 250 Hz,
detects R peaks, calculates BPM/RR/RMSSD and shows input quality on the 160 x
128 TFT. The previous square-wave generator, frequency page and 40 kSa/s DMA
oscilloscope path are not linked or started by this project.

## Display and controls

- `MONITOR`: ECG trace, BPM, RR, RMSSD, signal quality, gain, time window,
  run/freeze and event state.
- `WAVE`: enlarged 156 x 93 ECG trace. It uses the same 250 Hz ECG history and
  defaults to a two-second window, making a normal heartbeat visible instead of
  showing an almost flat five-millisecond fragment.
- `SW1`: run/freeze.
- `SW2` short press: x1/x2/x4 gain.
- `SW2` double press: toggle `MONITOR` and `WAVE`.
- `SW3` short press: event marker; double press: reset measurements.
- Rotary encoder: select two or five seconds.

`F` beside the gain means automatic FIT limited the requested scale so the
complete trace remains inside the plot.

## Safe signal and connection

Connect PA3 only to a 0 V through 3.3 V ECG simulator or isolated analog-front-
end output, with common ground. A signal centered near 1.65 V and about 1.0 Vpp
is a safe starting point. Do not connect negative voltage, more than 3.3 V, a
patient or an unisolated electrode circuit directly to the development board.
This firmware is not a medical device.

## Build, test and flash

Run the host checks from this directory:

```powershell
& .\Tests\run_tests.ps1
```

Then open `Project/Demo14_HESS_ECG_Analyzer.uvprojx`, rebuild, require zero
errors and warnings, and flash
`Project/Objects/Demo14_HESS_ECG_Analyzer.hex`.

After flashing:

1. Confirm the HESS splash transitions to `MONITOR`.
2. Feed a safe ECG signal to PA3 and wait for the history to fill.
3. Double-press `SW2`; confirm `WAVE` shows the ECG over `2s`.
4. Rotate to `5s`, cycle x1/x2/x4 and verify run/freeze and event/reset.
5. Check BPM, RR, RMSSD, quality and refresh smoothness on the physical TFT.

## Performance and memory design

- TIMER0 triggers one ADC conversion every 4 ms; no high-rate ADC DMA interrupt
  load remains.
- The five-second ring history uses 1250 bytes instead of the former 2500-byte
  ten-second history.
- The 800-byte DMA buffer, 200-byte fast frame and 312-byte duplicate plot cache
  are removed from the linked application.
- The UI scheduler remains 40 ms (25 FPS). A changed 156 x 93 plot is composed
  into final-color scanlines and sent in 93 SPI address windows; unchanged or
  frozen plots are skipped by comparing an acquisition revision.
- The firmware uses fixed allocation and integer display math.
- Host tests cover the ECG core, acquisition controls/freeze/event/reset,
  bounded UI refresh, Chinese glyphs and the linked-firmware contract.

Host tests and a successful Keil build cannot validate PA3 scaling, analog
front-end polarity, realistic noise, physical refresh timing or key polarity;
those remain board-level acceptance items.

## Verified build

- Keil MDK ArmClang: 6.24
- Device pack: `GigaDevice.GD32E23x_DFP 1.1.0`
- Result: `0 Error(s), 0 Warning(s)`
- Program: 13742 bytes code, 7550 bytes RO data
- ROM image: 21304 bytes (20.80 KB)
- Static RAM: 12 bytes RW data plus 3348 bytes ZI data = 3360 bytes
- Maximum analyzed stack: 928 bytes plus untraceable function pointers
- Static RAM plus analyzed stack: 4288 bytes, leaving 3904 bytes of the 8 KB
  SRAM before untraceable-call and interrupt-stack overhead
- HEX SHA-256:
  `C3FC56D6D282BDB3B81063BCF2067AE2D99E72D688BD33F6F73A0523A4D94B4C`

These figures come from the final full rebuild. Physical-board acceptance is
still required for the ECG input and visible refresh behavior.

See `Spec/Demo14_HESS_ECG_ANALYZER_SPEC.md` and
`Spec/Demo14_HOSPITAL_MONITOR_UI_SPEC.md` for the firmware and UI contracts.
