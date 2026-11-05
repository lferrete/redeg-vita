#!/usr/bin/env python3
# Port code Copyright (C) 2026 Leandro Ferrete (MIT).
"""Replay the exact supported Mach-O sine/cosine routines with Unicorn.

Compare an opt-in pre-entry hardware image dump to the original executable
for all 4096 angle units. This is a focused CPU regression, not a game emulator.
Requires unicorn. Game inputs/dumps stay private and are never redistributed.
"""
import argparse
import hashlib
import json
from pathlib import Path

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_LR

GAME_SHA256 = "9e451d0bbdf4aff2e5ad40eabb577e474464ca855dc41375845e4a54f73a5267"


def replay(blob, base, delta):
    machine = Uc(UC_ARCH_ARM, UC_MODE_ARM)
    machine.mem_map(base, (len(blob) + 4095) & ~4095)
    machine.mem_write(base, blob)
    stop = 0x50000000
    machine.mem_map(stop, 4096)
    results = {}
    for name, vm in (("sin", 0x733A0), ("cos", 0x73340)):
        values = []
        for angle in range(4096):
            machine.reg_write(UC_ARM_REG_R0, angle)
            machine.reg_write(UC_ARM_REG_LR, stop)
            machine.emu_start(vm + delta, stop, count=100)
            value = machine.reg_read(UC_ARM_REG_R0)
            values.append(value - (1 << 32) if value >= 1 << 31 else value)
        results[name] = values
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original", type=Path)
    parser.add_argument("loaded", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    original, loaded = args.original.read_bytes(), args.loaded.read_bytes()
    if hashlib.sha256(original).hexdigest() != GAME_SHA256:
        parser.error("unsupported executable SHA-256; rediscover offsets for this version")
    expected = replay(original, 0x1000, 0)
    actual = replay(loaded, 0x98000000, 0x97FFF000)
    mismatches = []
    for name in expected:
        for angle, (before, after) in enumerate(zip(expected[name], actual[name])):
            if before != after:
                mismatches.append(dict(function=name, angle=angle, expected=before, actual=after))
    assert expected["sin"][0] == 0 and expected["sin"][1024] == 4096
    assert expected["cos"][0] == 4096 and expected["cos"][2048] == -4096
    report = dict(original_sha256=GAME_SHA256,
                  loaded_sha256=hashlib.sha256(loaded).hexdigest(),
                  cases=8192, mismatch_count=len(mismatches),
                  first_mismatches=mismatches[:32], passed=not mismatches)
    encoded = json.dumps(report, indent=2)
    if args.output:
        args.output.write_text(encoded + "\n", encoding="utf-8")
    print(encoded)
    return 1 if mismatches else 0


if __name__ == "__main__":
    raise SystemExit(main())
