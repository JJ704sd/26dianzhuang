# DG1032Z Simulated SpO2/PPG Mode Specification

## Purpose

The third Demo15 mode calculates a laboratory simulation value from either a
CH1 duty-coded square wave or optional tagged RED/IR samples. It is not a
clinical pulse oximeter.

The supplied instrument photograph is evidence of the generator model only;
it does not contain firmware instructions.

## Equipment contract

- Generator: RIGOL DG1032Z, CH1 output for the `DUT` test; CH2 is
  optional only when it is physically wired as a valid wavelength tag.
- Analog sample input: BNC analog front end routed to PA3 / ADC channel 3.
- Wavelength tag input: PA6, `LOW=RED`, `HIGH=IR`.
- Generator load setting: `High Z`.
- Generator and board must share signal ground.
- Keep the complete external BNC waveform between 0 V and 3.3 V. Never apply
  a negative input.
- PA6 must also stay between 0 V and 3.3 V. Do not connect a negative or 5 V
  logic tag.

The DG1032Z supports two channels, basic sine/square/pulse waveforms and built-in
arbitrary waveforms including HaverSine and ECG. Demo15 uses only the low-
frequency portion of those capabilities.

## Receiver protocol

There is no single fixed physiological parameter set. For every simulated
sample presented on PA3, PA6 identifies the wavelength:

- PA6 low: accumulate PA3 as a RED sample;
- PA6 high: accumulate PA3 as an infrared sample.

The tag should remain stable long enough for the 4 ms acquisition block. Use a
20-100 ms dwell per wavelength; a longer one-sided run is discarded as stale
when pairing starts. The receiver requires
500 accepted samples of each wavelength and invalidates an unbalanced stream
after 2000 total sample opportunities.

The DG1032Z built-in `Pulseilogram` is one PPG shape and does not by itself
provide two wavelengths. If the generator's blood-oxygen mode already outputs
paired RED/IR data, route its wavelength-sync output to PA6. Otherwise use a
synchronized custom arbitrary waveform plus a 0-3.3 V tag output.

The photographed setup has only CH1 physically connected. CH2 is configured as
1 Vpp with +1 V offset (approximately 0.5-1.5 V), which is not a reliable GPIO
high level and cannot serve as the tag. For a dual-channel test, use a connected
0-3 V square wave at a non-harmonic low rate such as 25 Hz, and synchronize it
with genuinely alternating RED/IR samples on CH1.

When no usable tag/pair is present, the firmware automatically enters the
explicit `DUT` path. Use a CH1 square wave at exactly 1.000 Hz or 2.000 Hz with
a safe 0-3.3 V range. The firmware estimates high-time duty from a 500-sample,
250 Sa/s averaged ADC-code window before external-voltage reconstruction and
maps it to a simulation percentage. A valid tagged pair changes the label to
`2CH` and uses the ratio-of-ratios result.

## Calculation

For the default duty-coded path:

- `duty = normalized high-time area of the received square wave`;
- `SIM (%) = 70 + 30*duty/100`;
- 25%, 50% and 75% duty therefore display 78%, 85% and 93% after rounding.

This is an explicit signal-generator encoding chosen for repeatable electrical
training. It is not a physiological conversion. Both signal levels must occur
in the window; 0% and 100% are therefore invalid. A near-flat window or any
ADC rail contact produces `---` and clears the previous value.

For each wavelength in the optional paired path:

- `DC = mean(PPG)`;
- `AC = RMS(PPG - DC)`;
- `PI = AC / DC`.

Then:

- `R = PI_RED / PI_IR`;
- `SpO2 (%) = 110 - 25R`.

The result is limited to the teaching display range 70-100%. Sustained rail contact,
missing RED/IR samples, nearly-flat AC, invalid DC or an out-of-range ratio
causes `---` instead of a stale percentage. The linear equation follows TI
SLAA655; TI also states that accurate SpO2 needs empirical calibration for the
specific optical design.

## Display contract

The simulated SpO2 mode displays:

- acquired PA3 waveform;
- calculated simulated SpO2 percentage or `---`;
- run/hold state;
- 1, 2, 4 or 5 s timebase;
- pulse rate in BPM;
- waveform peak-to-peak amplitude;
- `OK` or `WAIT` signal state;
- permanent `SIM` plus `WAIT`, `DUT` or `2CH` source labels.

The percentage remains permanently identified as `SIM`; `DUT` must not be
presented as a dual-wavelength result or clinical accuracy. `OK/WAIT` follows
the percentage validity; BPM may independently remain `---` while `DUT` is OK.

## Reused implementation

- The 20 kSa/s PA3 DMA remains the only ADC acquisition path.
- The shared stream is decimated to 250 Hz for ECG and simulated PPG display.
- The ECG and PPG pages share the same five-second history to preserve the 8 KB
  SRAM budget.
- The existing qualified-peak detector is reused internally for the displayed
  pulse rate. Its HESS name is an implementation detail, not a third user mode.
- The existing startup animation is retained unchanged at the user's request.

## Acceptance boundary

Host tests verify 25/50/75% duty mapping, invalid-window clearing, fixed-point
AC/DC ratio and rejection paths. Physical testing must still check raw PA3
duty accuracy, PA6 tag polarity/timing, RED/IR pairing, pulse-rate accuracy and
display stability. This feature is for electrical training and is not a
medical device.

## Primary references

- RIGOL, `DG1000Z User's Guide`, model and waveform specifications:
  <https://int.rigol.com/ind/Images/DG1000Z_UserGuide_EN_tcm13-2800.pdf>
- RIGOL, `DG1000Z Series` product page, dual-channel and arbitrary-waveform
  capabilities: <https://www.rigol.com/en_IN/products/function-arbitrary-waveform-generator/DG1000Z.html>
- Texas Instruments, `How to Design Peripheral Oxygen Saturation (SpO2) and
  Optical Heart Rate Monitoring Systems Using AFE4403`, SLAA655:
  <https://www.ti.com/lit/an/slaa655/slaa655.pdf>
- Analog Devices, `MAX3010x EV Kits Recommended Configurations and Operating
  Profiles`, RED/IR ratio-of-ratios:
  <https://www.analog.com/media/en/technical-documentation/user-guides/max3010x-ev-kits-recommended-configurations-and-operating-profiles.pdf>
