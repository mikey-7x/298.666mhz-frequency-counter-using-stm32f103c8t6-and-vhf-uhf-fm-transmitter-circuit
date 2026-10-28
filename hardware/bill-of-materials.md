# Bill of materials

**POWERED BY [mikey:hexoid-sys]**  
Created by **mikey-7x** — Yogesh R. Chauhan

The author's original hand-drawn sheet in `complete-circuit-diagram.png` remains the construction reference. This table is a readable transcription to make the repository easier to search and review.

## Frequency counter

| Qty | Item | Firmware/hardware role |
|---:|---|---|
| 1 | STM32F103C8T6 Blue Pill board | Measurement controller, input on PA0 |
| 1 | 0.96-inch SSD1306 I2C OLED, 128×64 | Display on PB6/PB7 or PB8/PB9 |
| 1 | 47 Ω resistor | Series resistor between input terminal and PA0 |
| 1 | Input connector/terminal | Signal centre and common ground |
| — | Short ground and signal wiring | Controlled connections for VHF testing |

## Frequency generator

| Qty | Item | Firmware/hardware role |
|---:|---|---|
| 1 | STM32F103C8T6 Blue Pill board | TIM1 source controller |
| 1 | Normally-open push button, optional | PB12 to GND for manual step and pause/resume |
| — | Common ground wiring | Shared with counter |
| — | Connection to counter's 47 Ω input network | PA8 output to counter PA0 |

The PC13 onboard LED is used for status patterns. No external LED is required.

## FM-transmitter test oscillator

| Qty | Item | Value or note from author's sheet |
|---:|---|---|
| 1 | Electrolytic capacitor | 47 µF, 50 V |
| 1 | Supply decoupling capacitor | 100 nF |
| 1 | Audio coupling capacitor | Marked 100 nF in the drawing |
| 1 | Resistor | 47 kΩ |
| 1 | Capacitor | 22 pF |
| 1 | Tank capacitor | 15 pF |
| 1 | Air-core coil | 4 turns, approximately 3.5 mm diameter |
| 1 | `CA` | Selected/variable capacitor; 10 pF or 18 pF notes for FM range |
| 1 | `CB` | Selected/variable capacitor; 10 pF, 18 pF, 3.9 pF and 3.3 pF examples appear in the notes |
| 1 | NPN transistor | 2N2222; verify the package pinout from its datasheet |
| 1 | Emitter resistor | 100 Ω |
| 1 | Output coupling capacitor | 100 pF |
| 1 | Signal diode | 1N4148 |
| 1 | Adjustable or fixed bench supply | Sheet is marked +12 V to +14 V |
| 1 | Output terminal | Controlled bench connection only |

No voltage rating should be guessed from the drawing. Select every component for the actual supply and RF voltages in the finished circuit, and verify transistor orientation and capacitor markings before power is applied.

## Recommended test accessories

- Properly rated RF dummy load or shielded test enclosure.
- Oscilloscope probe or other instrument suitable for checking PA0 voltage and waveform quality.
- Independent receiver, spectrum analyser or calibrated reference counter for important high-range verification.
- Short, mechanically stable test leads and a reliable common ground.
