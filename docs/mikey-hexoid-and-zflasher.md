# Compile with mikey:hexoid and flash with ZFlasher STM32

**POWERED BY [mikey:hexoid-sys]** · Created by [mikey-7x](https://github.com/mikey-7x)

This project uses STM32duino/Arduino sketches. [mikey:hexoid v5.0.0](https://github.com/mikey-7x/mikey-hexoid-v5.0.0/tree/main) compiles them; **ZFlasher STM32 and an ST-LINK/V2** write a selected firmware image to a Blue Pill. The compiler does not itself flash the board in the workflow described here. See also the [ZFlasher STM32 official guide](https://zdevs.ru/en/zf-stm/user_guide.html).

## Quick path: the author's precompiled counter

1. The counter bundle is in [`incoming-counter-build/`](../incoming-counter-build/README.md). On GitHub open [`v23gen2.hex`](../incoming-counter-build/v23gen2.hex), choose **Download raw file** and save it on your phone. Keep the `.hex` extension; downloading GitHub's HTML page is not a firmware file.
2. Connect your known-working ST-LINK/V2 to the **counter** Blue Pill and the phone through USB OTG. Power the target correctly and use your established SWD connection; do not short supplies.
3. Open **ZFlasher STM32**, grant USB permission if asked, and select the operation to write/program the MCU. Select the downloaded `v23gen2.hex` firmware file. App wording can differ between versions.
4. Check that the detected MCU is your intended STM32F103C8 target and that the selected file is the counter HEX. Flashing replaces existing program contents; back up any firmware you want to keep and do not erase option bytes or alter protection settings unnecessarily.
5. Tap **Go!**. Wait for the app to report success. If a separate verification operation is available, use it; do not assume programming succeeded if the app reports an error. Reset the counter and observe the logo and self-test; with no input, check for `NO SIGNAL`.

This HEX is for the **counter**, not the PA8 frequency generator. A successful write or `SELF PASS` is not a frequency calibration. User observation confirms the logo and `SELF PASS`; independent high-frequency accuracy still requires a known reference.

The ELF is for debugging/analysis and the MAP is a linker report, **not flashing choices**. The BIN is a raw flash image without an embedded load address; the validated image starts at `0x08000000`, but for this bundle use the address-bearing HEX unless you know exactly how your app handles BIN addresses. Do not change option bytes. If a bootloader or other data occupies the addressed flash region, flashing can overwrite it.

## Rebuild with mikey:hexoid

The steps below describe the public `mhex_e.sh` engine examined at commit `b87efaad689cd9a100a96cd1bfdd7c1835c79d90`. Menu names and board ID can change in a later tool revision. You already have the tool: start it with `mhex`; installation instructions are in the [tool repository](https://github.com/mikey-7x/mikey-hexoid-v5.0.0/tree/main). Do not reinstall merely to build this project.

### 1. Put separate copies of the sketches in the project list

Extract the complete project ZIP, or download and extract this repository, then locate the extracted project root. In the same shell/environment used to run `mhex`, set `REPO` to that actual path. For example, if you extracted the long-named project folder **directly** inside Android Download:

```sh
REPO="/storage/emulated/0/Download/298.666mhz-frequency-counter-using-stm32f103c8t6-and-vhf-uhf-fm-transmitter-circuit"
test -f "$REPO/firmware/frequency_counter/frequency_counter.ino" && echo "Counter source found" || echo "Not found: correct REPO before continuing"
test -f "$REPO/firmware/frequency_generator/frequency_generator.ino" && echo "Generator source found" || echo "Not found: correct REPO before continuing"
```

If either file is not found, correct `REPO` to the folder containing `firmware/`; do not continue with a guessed path. Once both are found, create uniquely named working copies:

```sh
for pair in "frequency_counter:mikey_counter_copy" "frequency_generator:mikey_generator_copy"; do
  src=${pair%%:*}; name=${pair#*:}; dst="$HOME/mikey-hexoid/projects/$name"
  if [ -e "$dst" ]; then
    echo "Already exists; not overwritten: $dst"
  else
    mkdir -p "$dst" && cp "$REPO/firmware/$src/$src.ino" "$dst/$name.ino"
  fi
done
```

These are isolated copies: the project folder and `.ino` basename match, and neither the extracted repository nor an existing project is overwritten. To update an existing copy later, compare it manually before replacing it. This engine deletes the selected project's `build/` directory and any `main.c` before compiling and may temporarily inject an LED definition into an `.ino`; do not point it at a project containing files you need to keep. The tool restores the sketch after a normal compile attempt, but an interruption could leave its temporary edit behind.

### 2. Match the known board configuration

The uploaded counter MAP records `STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,usb=HID`. The upstream engine's ID `40` defaults to the same board **without** `,usb=HID`. Both configurations compiled in a controlled Linux test, but they produced different binaries; the HID selection matched this bundle byte-for-byte in that test. The installed Arduino CLI/core versions and other settings can also change build output.

- In your installed `~/mikey-hexoid/mhex.py`, check the line for board ID `40`. If it already ends in `,usb=HID`, leave it alone.
- To match the known build when the line lacks it, follow the [author's HID configuration instructions](https://github.com/mikey-7x/mikey-hexoid-v5.0.0#enable-the-usb-hid-profile-for-the-blue-pill-in-the-compilation-flags): edit only the ID `40` FQBN to `STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,usb=HID`, save, and relaunch `mhex`. Re-running the `mhex_e.sh` installer regenerates `mhex.py` and may reset this edit; check again afterward.
- This USB compile option is used to reproduce the tested build; it does **not** mean ZFlasher uses HID. Flashing here uses ST-LINK over SWD. No HID edit is needed just to flash the supplied HEX.

### 3. Compile the selected sketch

1. Run `mhex` in your existing shell. Choose **2. Compile Project (Universal)**.
2. Select `mikey_counter_copy` by the number shown in your own list. Enter board ID **40** for **BluePill F103C8**.
3. Wait for **Success! Outputs**. On Android shared storage, the observed output path is `/storage/emulated/0/mikey-hexoid-outputs/BluePill_F103C8/mikey_counter_copy/`. The engine prints the actual path; Linux without Android shared storage uses `~/mikey-hexoid-outputs/`.
4. For the generator, repeat with `mikey_generator_copy`. Its output is for the **generator board**, not the counter.
5. Select the appropriate `.hex` in ZFlasher, as above. Keep each board's file separate. These sketches need no external Arduino libraries, although the tool itself may install components for its broader supported platforms.

Do not assume a rebuild is identical merely because the filenames match. For this repository's default sketch, the published HEX and BIN matched an independent build using Arduino CLI 1.5.2-rc.1, STM32duino 3.0.0, board ID 40 with `usb=HID`, and the extracted `mhex_e.sh` compile function. Other versions/settings may produce different files. The exact STM32 core/CLI version used on the author's phone was not established from the four files.

## Configuration and limits

- Counter: `LOGO_SPLASH_MS` defaults to `3000u`; the bitmap is built into the sketch. Change a setting in the working copy, recompile and reflash to use it. Published HEX remains unchanged until rebuilt.
- `COUNTER_CLOCK_MHZ` is `112` by default; 112 and 96 MHz exceed the STM32F103's 72 MHz specified system-clock maximum. The actual startup screen reports clock/range and may fall back. `CAL_PPM` is `0.0`; change only against a trusted reference.
- Generator: the default nominal table ends at 36 MHz. Optional 40–64 MHz steps use overclocking; PB12 cannot interrupt the SRAM-isolated 64 MHz step. See [build and operation](build-and-operation.md) for the hold/reset limitation and unsupported diagnostic frequencies.
- Keep RF testing controlled and lawful, and protect the 3.3 V input from excessive voltage. No file or self-test guarantees accuracy at the experimental range ceiling.
