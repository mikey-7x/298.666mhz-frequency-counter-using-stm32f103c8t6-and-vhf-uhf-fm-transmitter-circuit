# VHF/UHF FM transmitter bench notes

**POWERED BY [mikey:hexoid-sys]**  
Created by **mikey-7x** — Yogesh R. Chauhan

These notes transcribe and explain the test-oscillator section of the author's hand-drawn sheet. They are included so the circuit used to exercise the counter remains part of the project record.

## Author's FM-radio verification video

[Watch the demonstration on YouTube](https://youtu.be/2F3CqLfh4eI). The author compares the OLED counter reading of the FM transmitter with reception on a keypad phone's FM radio. The phone's tuning steps and receiver accuracy limit the precision of that check; it does not validate the counter's upper experimental range.

## Safety and legality first

An FM transmitter can interfere with licensed radio services. Do not connect an antenna or radiate a signal unless the operator, equipment, frequency, bandwidth, power and emissions are authorised in the place of operation. For counter development, use a properly rated dummy load or a shielded, controlled bench setup and keep the output contained.

Before connecting any experimental oscillator output to PA0:

- confirm there is no unsafe DC level for the STM32 input;
- confirm peak voltage stays within the MCU pin limits;
- establish the intended common-ground arrangement;
- inspect for excessive ringing or multiple threshold crossings;
- start with a known lower frequency and a stable supply.

## Values transcribed from the sheet

| Circuit item | Value |
|---|---:|
| Supply rail | +12 V to +14 V |
| Reservoir electrolytic | 47 µF, 50 V |
| Supply bypass | 100 nF |
| Audio input coupling | Marked 100 nF |
| Base feed resistor | 47 kΩ |
| Base-to-ground capacitor | 22 pF |
| Fixed tank capacitor | 15 pF |
| Tank coil | 4 turns, approximately 3.5 mm diameter |
| Transistor | 2N2222 |
| Emitter resistor | 100 Ω |
| Output coupling capacitor | 100 pF |
| Output diode | 1N4148 |

`CA` and `CB` are selected or variable capacitors identified in the drawing. Read their positions from the original sheet; do not interchange them based only on this summary.

## Author's tuning notes

### FM range, 88–108 MHz

The sheet instructs:

- set `CB = 10 pF` fixed;
- use `CA = 18 pF` or `10 pF`;
- another note permits `CA = 10 pF` or `18 pF` and `CB = 10 pF`, `18 pF`, or another selected value.

An FM receiver check around 92 MHz was used during project development as an independent identity check at one tuning point.

### High-range examples with `CA` not attached

| `CB` | Author's recorded counter result |
|---:|---:|
| 3.9 pF | approximately 264.333 MHz |
| 3.3 pF | approximately 266.000 MHz |

These are recorded bench observations, not guaranteed design equations. A free-running LC oscillator at these frequencies is strongly affected by:

- exact coil diameter, spacing and lead length;
- transistor junction and package capacitance;
- trimmer minimum capacitance and mounting;
- supply voltage and decoupling;
- nearby hands, tools, wires and enclosure metal;
- temperature and mechanical vibration.

## Practical measurement method

1. Mechanically fix the coil and trimmer before judging counter stability.
2. Use the shortest practical controlled connection to the measurement setup.
3. Tune in very small movements and wait for a complete gate at each dwell.
4. Record the formatted result, gate label, raw `N`, supply voltage and `CA`/`CB` settings together.
5. Repeat the same setting with 100 ms, 250 ms and 1 s gates where the counter path provides them.
6. If a sudden jump to an integer-looking multiple occurs, inspect the waveform for ringing or parasitic content before naming a new fundamental.
7. Confirm important RF frequencies with a lawful receiver setup, spectrum analyser or calibrated instrument.

## Construction cautions

- Verify the exact 2N2222 pinout for the manufacturer and package in hand; compatible-looking transistors are not all pinned identically.
- A capacitor marked with a nominal pF value may have substantial tolerance at VHF/UHF.
- Keep tank leads extremely short and rigid if repeatability matters.
- Do not power the oscillator from an unregulated supply whose actual voltage exceeds the design assumption.
- Never assume the output terminal is safe for a 3.3 V logic input merely because a coupling capacitor is present.
