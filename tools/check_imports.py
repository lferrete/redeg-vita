#!/usr/bin/env python3
"""Static import audit for the Resident Evil: Degeneration port.

Reads data/game/residentevil (thin 32-bit little-endian Mach-O), prints the
undefined symbols and checks which ones are provided by the S() table in
source/shims/symbols.c. Run from the project root after prepare_data.ps1:

    py -3 tools/check_imports.py
"""
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parent.parent
BINARY = ROOT / "data" / "game" / "residentevil"
SYMBOLS_C = ROOT / "source" / "shims" / "symbols.c"


def read_macho_undefined(path: Path):
    data = path.read_bytes()
    magic, cputype, cpusub, filetype, ncmds, sizeofcmds, flags = struct.unpack_from("<IiiIIII", data, 0)
    if magic != 0xFEEDFACE:
        raise SystemExit(f"Not a thin 32-bit little-endian Mach-O: magic=0x{magic:08x}")

    off = 28
    symtab = None
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", data, off)
        if cmd == 2:  # LC_SYMTAB
            symoff, nsyms, stroff, strsize = struct.unpack_from("<IIII", data, off + 8)
            symtab = (symoff, nsyms, stroff, strsize)
        off += cmdsize

    if not symtab:
        raise SystemExit("LC_SYMTAB not found")

    symoff, nsyms, stroff, strsize = symtab
    strtab = data[stroff:stroff + strsize]
    undefined = set()
    for i in range(nsyms):
        strx, ntype, nsect, ndesc, value = struct.unpack_from("<IBBH I".replace(" ", ""), data, symoff + i * 12)
        if ntype & 0x0E != 0:  # N_UNDF
            end = strtab.find(b"\0", strx)
            name = strtab[strx:end].decode("utf-8", "replace")
            if name:
                undefined.add(name)
    return undefined


def read_shim_symbols(path: Path):
    text = path.read_text(encoding="utf-8", errors="replace")
    return set(re.findall(r'S\("([^"]+)"', text))


def main():
    if not BINARY.exists():
        sys.exit("Missing data/game/residentevil; run tools/prepare_data.ps1 first.")
    if not SYMBOLS_C.exists():
        sys.exit(f"Missing {SYMBOLS_C}")

    undefined = read_macho_undefined(BINARY)
    provided = read_shim_symbols(SYMBOLS_C)
    missing = sorted(undefined - provided)
    extra = sorted(provided - undefined)

    print(f"Mach-O undefined symbols: {len(undefined)}")
    print(f"symbols.c S() entries:    {len(provided)}")
    print(f"Undefined without an S() entry: {len(missing)}")
    for name in missing:
        print("  UNMAPPED", name)
    if extra:
        print(f"S() entries not imported by this binary: {len(extra)} (normal)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
