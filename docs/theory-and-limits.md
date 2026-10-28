# Theory and defensible range limits

**POWERED BY [mikey:hexoid-sys]**  
Created by **mikey-7x** — Yogesh R. Chauhan

## Signal path used by the counter

```text
PA0
  │
  ├─▶ TIM2 ETR asynchronous prescaler (/8)
  │       │
  │       └─▶ ETRP ─▶ two-flip-flop resynchronizer ─▶ TIM2 CNT
  │                                                    │
  │                                                    └─ update TRGO
  │                                                            │
  └─▶ TIM2 TI1F cross-check path                               ▼
                                                      TIM3 ITR1 CNT

Displayed gate frequency = measured TIM3:TIM2 edges × 8 / measured time
```

TIM3 extends TIM2 in hardware. Firmware does not need an interrupt or a race-prone software extension to preserve an overflow.

## Two different ceilings

The ETR prescaler is before the resynchronizer. If the incoming frequency is `fIN`, then:

```text
fETRP = fIN / 8
```

### Conservative design ceiling

The reference-manual design rule used by this project is:

```text
fETRP <= TIM2CLK / 4
fIN   <= 8 × TIM2CLK / 4
fIN   <= 2 × TIM2CLK
```

### Resynchronizer boundary

ST's timer application guidance expresses the two-flop synchronizer relationship as:

```text
TIM2CLK >= 3 × fETRP
fETRP   <= TIM2CLK / 3
fIN     <= 8 × TIM2CLK / 3
```

The region between the two results is displayed numerically but marked `MHz EXP`.

| TIM2CLK | `2 × TIM2CLK` design ceiling | `8 × TIM2CLK / 3` synchronizer ceiling |
|---:|---:|---:|
| 72 MHz | 144 MHz | 192 MHz |
| 96 MHz | 192 MHz | 256 MHz |
| 112 MHz | 224 MHz | 298.666... MHz |
| 128 MHz arithmetic example | 256 MHz | 341.333... MHz |

The public firmware intentionally offers 72, 96 and 112 MHz counter clocks. A 128 MHz arithmetic ceiling is not enabled because reliable whole-sketch flash/OLED execution at that clock cannot be assumed for the target board.

## Why stability slices matter

A frequency counter that averages across a tuning sweep can produce a plausible-looking number for a signal that never settled at that frequency. This firmware therefore divides each gate into consecutive slices:

- Primary and normal precision gates use 100 ms slices.
- High-range tracking gates use 25 ms slices inside a 100 ms gate.
- Slice rates must stay within the firmware's 0.5% stability band, with a low-rate quantization floor.
- A changing gate can retain high-range state, but its average is not displayed.

At 192 MHz, even the short tracking gate has about 2.4 million prescaled counts, so count quantization is only about 80 Hz per count. The stability test, not raw count size, is what rejects a moving oscillator.

## Why the synchronous prescaler cannot create a GHz mode

TIM2 PSC is encountered after the asynchronous ETRP stream has been synchronized into the timer clock domain. Increasing PSC can divide pulses that survived synchronization; it cannot reconstruct pulses that were missed before that point.

For a 1 GHz input:

```text
ETRP after /8                         = 125 MHz
TIM2CLK required by /3 relationship   >= 375 MHz
TIM2CLK required by /4 design rule    >= 500 MHz
```

Those clocks are unavailable in an STM32F103. A GHz claim based on occasional, dropped or aliased counts would not be an accurate frequency measurement.

## What a counted-edge rate does—and does not—prove

The counter proves how many qualifying logic edges crossed PA0 during a measured interval, subject to the timer limitations above. It does not by itself prove that an RF oscillator's radiated fundamental equals that edge rate. Any of the following can alter the edge stream before PA0:

- oscillator mode jumping;
- parasitic oscillation;
- harmonic-rich or clipped waveforms;
- ringing that crosses a logic threshold more than once per cycle;
- excessive amplitude or DC offset;
- intermittent contacts and mechanically unstable tank components.

Repeated agreement between 100 ms, 250 ms and 1 s gates is useful evidence, and an independent receiver or spectrum measurement is the correct confirmation for RF identity.
