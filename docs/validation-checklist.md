# Validation checklist

**POWERED BY [mikey:hexoid-sys]**  
Created by **mikey-7x** — Yogesh R. Chauhan

Use this checklist whenever the firmware, board, oscillator or test wiring changes. Record failures as displayed; do not adjust `CAL_PPM` or stability rules to force a pass.

## Identity and disconnected state

- [ ] OLED splash shows `POWERED BY` and `[mikey:hexoid-sys]`.
- [ ] OLED splash shows `SELF PASS`.
- [ ] Actual system clock recorded.
- [ ] Actual maximum range recorded.
- [ ] Splash shows `BY mikey-7x`.
- [ ] PA0 disconnected settles to `NO SIGNAL`.
- [ ] No disconnected state displays a frequency.
- [ ] Measurement header reads `mikey-7x COUNTER`.

## Generator regression

Record each step in `test-record.csv`.

- [ ] 1 Hz
- [ ] 10 Hz
- [ ] 100 Hz
- [ ] 1 kHz
- [ ] 10 kHz
- [ ] 100 kHz
- [ ] 1 MHz
- [ ] 2 MHz
- [ ] 4 MHz
- [ ] 8 MHz
- [ ] 12 MHz
- [ ] 18 MHz
- [ ] 24 MHz
- [ ] 36 MHz
- [ ] Optional 40 MHz
- [ ] Optional 48 MHz
- [ ] Optional 56 MHz
- [ ] Optional 64 MHz

For every point, confirm that the raw count is consistent with the gate duration and that a moving/step transition is not accidentally recorded as a settled measurement.

## Independent RF checks

- [ ] FM-band point confirmed by a lawful receiver method.
- [ ] Supply voltage and `CA`/`CB` values recorded.
- [ ] A previously stable point near 182.87 MHz repeated, if reproducible.
- [ ] Slow dwell through 192 MHz recorded with gate label and `N`.
- [ ] 224 MHz design-boundary behaviour recorded.
- [ ] Above 224 MHz, unit marker is `MHz EXP`.
- [ ] At least two gate durations compared above 224 MHz.
- [ ] Unexpected harmonic-looking jumps investigated as waveform/mode changes.
- [ ] Any result near 298.666 MHz repeated after a power cycle.
- [ ] Results above the displayed ceiling show `OVER RANGE` and raw `N`.

## Publication checks

- [ ] `LICENSE` is present and unmodified.
- [ ] `NOTICE.md` retains the author, logo and powered-by line.
- [ ] Both firmware headers retain author, project title and licence notices.
- [ ] No public file presents 64 MHz generator output as 65 MHz.
- [ ] No result above 224 MHz is described as guaranteed or certified.
- [ ] No software-only 1 GHz capability is claimed.
- [ ] RF operation notes include legal and dummy-load/shielding warnings.
