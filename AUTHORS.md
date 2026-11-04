# Authors and credits

## PS Vita port

**Leandro Ferrete** — Mach-O loader, ObjC runtime, iOS framework shims,
EAGL/GLES1 integration over vitaGL, input (physical controls + front touch),
audio (`AudioQueue` → `sceAudioOut`), save/data handling, documentation and
hardware testing.

Copyright (C) 2026 Leandro Ferrete. The original port code is distributed
under the MIT license (see `LICENSE`). The LiveArea artwork is derived from
the original game and remains the property of its rights holders; the port
attribution lives in the source, log banner and VPK metadata.

## Original game

**Resident Evil: Degeneration** © Capcom. All rights reserved. The game was
published for iOS by Capcom; the original engine uses the Marmalade SDK /
S3E runtime, © Marmalade Technologies and contributors.

This project does **not** redistribute the IPA, the `residentevil` executable,
`main.dz`, `.icf` configuration, `.nib` archives, videos, textures, audio,
saves or any other game data. The LiveArea artwork is derived from the
original game; users must provide a legally obtained, decrypted copy to play.

## Third-party components

- **vitaGL** — Rinnegatamante and contributors.
- **kubridge** — TheFloW.
- **vitaShaRK / SceShaccCg / ShaRKBR33D** — Rinnegatamante and contributors.
- **VitaSDK** — the VitaSDK contributors.
- **zlib** — Jean-loup Gailly, Mark Adler and contributors.
- **newlib / libstdc++** — their respective authors.
- **Marmalade SDK / S3E** — Marmalade Technologies and contributors (original
  engine; no engine code is redistributed).

## How to identify this port

- Log banner: `Port by Leandro Ferrete (v1.0)` and the copyright line in
  `ux0:data/redeg/logs/redeg_NNN.log`.
- SFO: `PORT_AUTHOR=Leandro Ferrete` and `PORT_VERSION=1.0`.
- VPK: `sce_sys/provenance.txt` with author, version and legal notice.
- Release: `build/SHA256SUMS.txt` with the VPK hash.
- Source: `source/port_author.h`, `source/main.c`, `CMakeLists.txt`,
  `build.sh`, `make.sh` and `tools/`.
