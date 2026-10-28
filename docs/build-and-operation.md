# Build and operation guide

**POWERED BY [mikey:hexoid-sys]**  
Created by **mikey-7x** — Yogesh R. Chauhan

## Build both sketches

From the repository root:

```sh
arduino-cli compile \
  --fqbn STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,usb=HID \
  firmware/frequency_counter

arduino-cli compile \
  --fqbn STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,usb=HID \
  firmware/frequency_generator
```

In Arduino IDE, select the Generic STM32F1 family and BluePill F103C8. Choose the upload method that matches your programmer. No project library needs to be installed from Library Manager.

## Counter settings

### `COUNTER_CLOCK_MHZ`

- `112` — default; highest counter range in this release and the setting used for the author's high-range tests.
- `96` — comparison setting; normal ceiling 192 MHz and experimental ceiling 256 MHz.
- `72` — in-specification system clock; normal ceiling 144 MHz and experimental ceiling 192 MHz.

Always read the startup splash. The firmware displays the actual verified clock and derived maximum, including any fallback.

### `CAL_PPM`

Leave at `0.0` until the board's reference oscillator has been measured against a trusted standard. `CAL_PPM` corrects a known reference error; it must never be used to compensate for dropped timer pulses, poor waveform shape or an unreachable target frequency.

## Generator settings

### Automatic sequence

With default settings the generator starts at 1 Hz and advances every 15 seconds through the normal-clock exact table ending at 36 MHz.

### Optional 40–64 MHz steps

Change only this setting:

```cpp
#define TOP_EXPERIMENT_64MHZ 1
```

The automatic sequence then appends 40, 48, 56 and 64 MHz. These use experimental core clocks. The 64 MHz step runs in SRAM while the core is at 128 MHz, then restores 72 MHz before flash execution resumes.

### Fixed diagnostics

For a stationary source, set exactly one of:

```cpp
#define FIXED_TEST_8MHZ 1
#define FIXED_TEST_36MHZ 1
#define FIXED_TEST_64MHZ 1  // also requires TOP_EXPERIMENT_64MHZ 1
```

The button is intentionally ignored in these modes.

## First power-on sequence

1. Flash the counter and leave PA0 disconnected.
2. Confirm the OLED powered-by splash and `SELF PASS`.
3. Record the actual `SYSCLK` and `MAX` lines.
4. Confirm `NO SIGNAL` is the normal idle state.
5. Flash the generator to the second board.
6. Connect common GND and PA8 → 47 Ω → counter PA0.
7. Work upward through the generator table and log the gate label and `N` at each point.

## Reading the counter display

A qualified numeric screen contains:

- formatted frequency;
- `Hz`, `kHz`, `MHz` or experimental `MHz EXP`;
- raw count `N=...` for the displayed gate;
- a bottom label identifying the measurement path and gate duration.

The raw count is the audit trail. For an ETR/8 gate:

```text
frequency ≈ N × 8 / gate seconds
```

The firmware uses measured elapsed time rather than assuming the nominal gate length, so small differences from a hand calculation are expected.

## Troubleshooting by displayed state

| State | First interpretation | Useful next step |
|---|---|---|
| `NO SIGNAL` | No qualifying edge stream was measured | Confirm source power, common ground and PA0 waveform level |
| `INPUT CHANGING` | Oscillator or source moved during the gate | Stop tuning and dwell; mechanically stabilise the oscillator |
| `HIGH SIGNAL LOST` | A recently high signal no longer qualifies | Tune slowly back through the region while tracking gates run |
| `COUNT SYNC LOST` | Coherent timer snapshots failed | Power-cycle and record the splash clock; repeat with a lower known source |
| `TIMER CONFIG FAILED` | Register read-back mismatch | Reflash the unmodified sketch and record board/core details |
| `OVER RANGE` | Count exceeds derived synchronizer ceiling | Record `N`, gate duration and actual SYSCLK; do not rescale it into a claim |
| Plausible but unexpected multiple | Waveform may cross threshold multiple times or contain a different mode | Inspect waveform and verify with an independent instrument |

Do not solve a high-range failure by relaxing stability tolerances, snapping to a table or changing `CAL_PPM`. Those changes make a number easier to obtain but destroy the distinction between a measurement and a desired value.
