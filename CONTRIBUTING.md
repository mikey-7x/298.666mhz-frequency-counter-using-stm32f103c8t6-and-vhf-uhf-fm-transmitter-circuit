# Contributing and reporting results

**POWERED BY [mikey:hexoid-sys]**  
Project owner: **Yogesh R. Chauhan (mikey-7x)** — https://github.com/mikey-7x

Thank you for helping improve the documentation, firmware or hardware evidence. This repository is **source-available under a strict personal and educational licence**, not an open-source project. Read `LICENSE`, `NOTICE.md` and `AUTHORS.md` before submitting anything.

## Before opening a pull request

1. Open an issue first for a measurement, safety, RF or behaviour change.
2. Explain the problem with evidence—not only the expected number.
3. Preserve all author, copyright, title, logo and powered-by notices.
4. Do not remove or alter the identity strings in either firmware file.
5. Do not introduce an external Arduino library without prior discussion.
6. Do not relax a stability check, snap a result to a table, or use calibration to disguise dropped counts.
7. Do not claim a 65 MHz generator step or a software-only 1 GHz counter mode; both contradict the documented hardware limits.

## Hardware reports must include

- Blue Pill/MCU marking and a clear board photograph if behaviour differs.
- Counter splash `SYSCLK` and `MAX` lines.
- Source type and expected frequency.
- Displayed value and unit, including `EXP` when shown.
- Raw `N=...` count and bottom gate label.
- Disconnected-input result.
- Supply voltage and, for the transmitter circuit, `CA`/`CB` values.
- Independent receiver/instrument result for important RF claims.

Use the **Hardware test report** issue form so these details are not lost.

## Firmware change checklist

- The Arduino folder and `.ino` basename still match.
- `python3 tools/verify_repository.py` passes.
- Counter compiles at 112, 96 and 72 MHz with `--warnings all`.
- Generator compiles in normal, experimental-top and fixed-64 modes.
- A disconnected counter still reports `NO SIGNAL` and never invents a frequency.
- OLED splash still shows the powered-by lockup and `BY mikey-7x`.
- The ELF identity string still contains the author, GitHub links and licence.
- New measurements are recorded in `docs/test-record.csv` format.

## Commercial requests are not contributions

Do not open a normal contribution or pull request to obtain commercial rights. Use the **Commercial licence enquiry** issue form and wait for separate signed written permission from Yogesh R. Chauhan (mikey-7x) before any business, manufacturing or industrial use.
