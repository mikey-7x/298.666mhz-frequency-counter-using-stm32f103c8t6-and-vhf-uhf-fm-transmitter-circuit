#!/usr/bin/env python3
"""Verify the author's counter build using only the Python standard library."""
from pathlib import Path
import hashlib
import re
import struct
import sys

ROOT = Path(__file__).resolve().parent.parent
FOLDER = ROOT / 'incoming-counter-build'
BASE = 0x08000000
LIMIT = 0x08010000  # nominal 64 KiB Blue Pill F103C8 flash


def require(condition, message):
    if not condition:
        raise ValueError(message)
    print('PASS ', message)


def read_hex(path):
    memory, base, eof = {}, 0, False
    for number, line in enumerate(path.read_text(encoding='ascii').splitlines(), 1):
        require_line = f'Intel HEX line {number}'
        if not line or not line.startswith(':') or eof:
            raise ValueError(f'{require_line}: bad record or data after EOF')
        try:
            record = bytes.fromhex(line[1:])
        except ValueError as e:
            raise ValueError(f'{require_line}: invalid hex') from e
        if len(record) < 5 or len(record) != record[0] + 5 or sum(record) % 256:
            raise ValueError(f'{require_line}: length/checksum mismatch')
        n, addr, typ = record[0], int.from_bytes(record[1:3], 'big'), record[3]
        data = record[4:4+n]
        if typ == 0:
            for i, byte in enumerate(data):
                address = base + addr + i
                if not BASE <= address < LIMIT or address in memory:
                    raise ValueError(f'{require_line}: overlap or outside F103C8 flash')
                memory[address] = byte
        elif typ == 1:
            if n != 0 or addr != 0: raise ValueError(f'{require_line}: invalid EOF')
            eof = True
        elif typ in (2, 4):
            if n != 2 or addr != 0: raise ValueError(f'{require_line}: invalid address record')
            base = int.from_bytes(data, 'big') << (4 if typ == 2 else 16)
        elif typ in (3, 5):
            if n != 4 or addr != 0: raise ValueError(f'{require_line}: invalid start address record')
        else:
            raise ValueError(f'{require_line}: unsupported record type')
    require(eof and bool(memory), 'HEX checksums, records and EOF are valid')
    require(min(memory) == BASE, 'HEX starts at 0x08000000 and remains within 64 KiB')
    return memory


def read_elf(path):
    data = path.read_bytes()
    require(len(data) >= 52 and data[:7] == b'\x7fELF\x01\x01\x01', 'ELF is 32-bit little-endian')
    e_type, machine = struct.unpack_from('<HH', data, 16)
    require(e_type == 2 and machine == 40, 'ELF is an ARM executable')
    phoff = struct.unpack_from('<I', data, 28)[0]
    entsize, count = struct.unpack_from('<HH', data, 42)
    require(entsize >= 32 and phoff + entsize*count <= len(data), 'ELF program headers are within the file')
    memory = {}
    for i in range(count):
        typ, offset, _, physical, filesz, _, _, _ = struct.unpack_from('<IIIIIIII', data, phoff+i*entsize)
        if typ != 1 or filesz == 0: continue
        require(offset+filesz <= len(data), 'ELF load segment is within the file')
        require(BASE <= physical and physical+filesz <= LIMIT, 'ELF load segment fits F103C8 flash')
        for j, byte in enumerate(data[offset:offset+filesz]):
            if physical+j in memory: raise ValueError('ELF flash segments overlap')
            memory[physical+j] = byte
    require(bool(memory) and min(memory) == BASE and len(memory) == max(memory)-BASE+1, 'ELF flash segments are contiguous from 0x08000000')
    return bytes(memory[BASE+i] for i in range(len(memory)))


def main():
    files = [f'incoming-counter-build/v23gen2.{ext}' for ext in ('hex', 'bin', 'elf', 'map')]
    lines = (FOLDER / 'SHA256SUMS').read_text(encoding='ascii').splitlines()
    records = {}
    for line in lines:
        match = re.fullmatch(r'([0-9a-f]{64})  (.+)', line)
        require(bool(match), 'SHA256SUMS record format')
        digest, name = match.groups()
        require(name in files and name not in records, 'manifest path is expected and unique')
        records[name] = digest
    require(set(records) == set(files), 'manifest lists exactly four build files')
    for name in files:
        require(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == records[name], f'SHA-256 matches {name}')
    binary = (FOLDER/'v23gen2.bin').read_bytes()
    require(len(binary) > 0 and len(binary) <= LIMIT-BASE, 'BIN fits nominal F103C8 flash')
    hex_memory = read_hex(FOLDER/'v23gen2.hex')
    require(max(hex_memory) == BASE + len(binary) - 1 and all(binary[a-BASE] == b for a, b in hex_memory.items()), 'all HEX-addressed bytes match BIN')
    holes = [i for i in range(len(binary)) if BASE+i not in hex_memory]
    require(holes == [0x10c, 0x10d, 0x10e, 0x10f] and all(binary[i] == 0 for i in holes), 'HEX omits four alignment bytes; BIN contains zero padding')
    require(read_elf(FOLDER/'v23gen2.elf') == binary, 'ELF loadable flash bytes match BIN exactly')
    source = (ROOT/'firmware/frequency_counter/frequency_counter.ino').read_text(encoding='utf-8')
    bitmap = re.search(r'OLED_LOGO_BITMAP\[128u \* 8u\][^=]*=\s*\{(.*?)\};', source, re.S)
    require(bitmap is not None, 'counter source contains startup logo bitmap')
    logo = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', bitmap.group(1)))
    require(len(logo) == 1024 and binary.count(logo) == 1, 'source logo appears exactly once in compiled image')
    report = (FOLDER/'v23gen2.map').read_text(encoding='utf-8', errors='replace')
    require('v23gen2.ino.cpp.o' in report and '0x08008c66' in report and '0x080014c0' in report, 'MAP identifies expected source and checked symbol addresses')
    print('Precompiled counter verification passed; file checks are not hardware or calibration tests.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, struct.error) as exc:
        print(f'FAIL  {exc}', file=sys.stderr)
        sys.exit(1)
