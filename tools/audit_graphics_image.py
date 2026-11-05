#!/usr/bin/env python3
# Port code Copyright (C) 2026 Leandro Ferrete (MIT).
"""Compare an opt-in pre-entry Vita image dump with its exact original Mach-O.

The supported RE Degeneration 1.03 image maps file offset 0 at VM 0x1000
through __TEXT; explicitly select other ranges only after verifying mapping.
This tool never modifies either input. Dumps contain game code: keep private.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("original", type=Path)
    p.add_argument("loaded", type=Path)
    p.add_argument("--start", type=lambda n: int(n, 0), default=0x35600)
    p.add_argument("--end", type=lambda n: int(n, 0), default=0x112BE8)
    p.add_argument("--constants", action="store_true")
    p.add_argument("--output", type=Path)
    args = p.parse_args()
    original = args.original.read_bytes()
    loaded = args.loaded.read_bytes()
    if struct.unpack_from("<I", original)[0] != 0xFEEDFACE:
        p.error("expected thin little-endian Mach-O")
    if not (0x1000 <= args.start < args.end <= min(len(original), len(loaded)) + 0x1000):
        p.error("range outside the provided files")
    changes = []
    for vm in range(args.start, args.end - 3, 4):
        offset = vm - 0x1000
        before = struct.unpack_from("<I", original, offset)[0]
        after = struct.unpack_from("<I", loaded, offset)[0]
        if before == after:
            continue
        if args.constants and not (before & (before - 1) == 0 or before & (before + 1) == 0):
            continue
        changes.append(dict(vm=f"0x{vm:08x}", before=f"0x{before:08x}", after=f"0x{after:08x}"))
    result = dict(original_sha256=hashlib.sha256(original).hexdigest(),
                  loaded_sha256=hashlib.sha256(loaded).hexdigest(),
                  range=[hex(args.start), hex(args.end)], changes=changes)
    report = json.dumps(result, indent=2)
    if args.output:
        args.output.write_text(report + "\n", encoding="utf-8")
    else:
        print(report)


if __name__ == "__main__":
    main()
