#!/usr/bin/env python3
"""ELF audit for Android -> PS Vita ports.

Usage:
    python elf_audit.py game.so [more.so ...]

Prints:
  - ELF class/machine/flags/entry
  - DT_NEEDED / SONAME
  - exported symbols (dynsym)
  - undefined imports, grouped by likely provider
  - JNI/Android/EGL/GLES hints

Requires pyelftools:  pip install pyelftools
"""
from __future__ import annotations

import sys
import collections
from typing import Iterable

try:
    from elftools.elf.elffile import ELFFile
    from elftools.elf.dynamic import DynamicSection
    from elftools.elf.sections import SymbolTableSection
except Exception as exc:  # pragma: no cover
    print("pyelftools is required: pip install pyelftools", file=sys.stderr)
    raise SystemExit(2)


GROUPS = {
    "jni": ("JNI_", "Java_", "jni"),
    "android": ("AAsset", "ANativeWindow", "android_", "__android"),
    "egl": ("egl",),
    "gles": ("gl",),
    "opensl": ("SL", "slCreate", "SLObject"),
    "pthread": ("pthread_", "sem_", "sched_"),
    "audio": ("AudioTrack", "AAudio", "opensl"),
    "game_zp": ("zp", "zc", "zpgfx", "zpinp", "zpadr"),
}


def group_of(name: str) -> str:
    low = name.lower()
    for group, needles in GROUPS.items():
        if any(n.lower() in low for n in needles):
            return group
    return "other"


def iter_symbols(elf: ELFFile):
    for section in elf.iter_sections():
        if not isinstance(section, SymbolTableSection):
            continue
        for sym in section.iter_symbols():
            yield section.name, sym


def audit(path: str) -> int:
    with open(path, "rb") as fh:
        elf = ELFFile(fh)
        hdr = elf.header
        print("=" * 78)
        print(path)
        print("-" * 78)
        print("class      :", hdr["e_ident"]["EI_CLASS"])
        print("machine    :", hdr["e_machine"])
        print("type       :", hdr["e_type"])
        print("entry      :", hex(hdr["e_entry"]))
        print("flags      :", hex(hdr["e_flags"]))

        needed: list[str] = []
        soname = None
        for section in elf.iter_sections():
            if isinstance(section, DynamicSection):
                for tag in section.iter_tags():
                    if tag.entry.d_tag == "DT_NEEDED":
                        needed.append(tag.needed)
                    elif tag.entry.d_tag == "DT_SONAME":
                        soname = tag.soname
        print("soname     :", soname or "-")
        print("needed     :", ", ".join(needed) if needed else "-")

        defined: list[str] = []
        undefined: list[str] = []
        seen_def: set[str] = set()
        seen_und: set[str] = set()
        for section_name, sym in iter_symbols(elf):
            name = sym.name
            if not name:
                continue
            shndx = sym["st_shndx"]
            if shndx == "SHN_UNDEF":
                if name not in seen_und:
                    seen_und.add(name)
                    undefined.append(name)
            elif sym["st_info"]["type"] in ("STT_FUNC", "STT_OBJECT",
                                            "STT_GNU_IFUNC", "STT_NOTYPE"):
                if name not in seen_def:
                    seen_def.add(name)
                    defined.append(name)

        print(f"exports    : {len(defined)}")
        print(f"undefined  : {len(undefined)}")

        groups: dict[str, list[str]] = collections.defaultdict(list)
        for name in undefined:
            groups[group_of(name)].append(name)

        print("\nundefined by likely provider:")
        for group in sorted(groups):
            print(f"  [{group}] {len(groups[group])}")
            for name in sorted(groups[group])[:40]:
                print("    ", name)
            if len(groups[group]) > 40:
                print(f"     ... ({len(groups[group]) - 40} more)")

        hints = [n for n in undefined
                 if any(k in n for k in ("JNI_OnLoad", "Java_", "AAsset",
                                         "ANativeWindow", "slCreate",
                                         "AudioTrack"))]
        if hints:
            print("\nJNI/Android/OpenSL hints:")
            for name in sorted(hints):
                print("  ", name)

        print("\nexports (first 80):")
        for name in sorted(defined)[:80]:
            print("  ", name)
        if len(defined) > 80:
            print(f"  ... ({len(defined) - 80} more)")

    return 0


def main(argv: Iterable[str]) -> int:
    paths = list(argv)
    if not paths:
        print(__doc__)
        return 1
    rc = 0
    for path in paths:
        try:
            rc |= audit(path)
        except Exception as exc:
            print(f"{path}: {exc}", file=sys.stderr)
            rc = 1
    return rc


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
