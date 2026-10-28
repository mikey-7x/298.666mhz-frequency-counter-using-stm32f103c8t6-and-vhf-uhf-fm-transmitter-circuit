# Precompiled counter build

**POWERED BY [mikey:hexoid-sys]** · Created by [mikey-7x](https://github.com/mikey-7x)

This folder retains the author's four files exactly as uploaded. It was initially a temporary unverified intake; the files have since been checked. The folder name is retained so no upload or existing link is removed.

| File | Purpose |
|---|---|
| [`v23gen2.hex`](v23gen2.hex) | Intel HEX for the **counter**; preferred file for the ZFlasher/ST-LINK procedure |
| [`v23gen2.bin`](v23gen2.bin) | The same raw flash bytes; load address `0x08000000` is not stored in the BIN |
| [`v23gen2.elf`](v23gen2.elf) | ARM executable with symbols for analysis/debugging |
| [`v23gen2.map`](v23gen2.map) | Linker map/report; not a firmware image |
| [`SHA256SUMS`](SHA256SUMS) | SHA-256 integrity values for the four files |

The ELF loadable flash contents match the BIN bytes, beginning at `0x08000000`. Every address present in the HEX matches the BIN. The HEX omits four alignment bytes at `0x0800010C–0x0800010F`; the BIN contains zero padding there. Actual values in those omitted locations depend on the programmer/erase operation. The embedded 1024-byte OLED bitmap matches the counter source. The HEX and BIN also match a controlled build of the repository counter with mikey:hexoid's extracted compile function, Arduino CLI 1.5.2-rc.1, STM32duino 3.0.0 and BluePill F103C8 with `usb=HID`. The MAP contains matching build addresses and source basename. These checks do not prove the build's complete provenance or frequency accuracy; differences in non-loadable ELF metadata and MAP paths are expected.

For phone download and flashing steps, see [mikey:hexoid and ZFlasher guide](../docs/mikey-hexoid-and-zflasher.md). Use this image on the **counter** board only. The default 112 MHz clock is beyond the MCU's rated 72 MHz maximum. The MAP contains build-machine paths, already publicly uploaded by its author.

From the repository root, validate all four files and their correspondence with the source bitmap:

```sh
python3 tools/verify_precompiled.py
```
