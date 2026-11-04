# Authorship, attribution and verification

```
Resident Evil: Degeneration — PS Vita port
Port code Copyright (C) 2026 Leandro Ferrete (MIT).
Original game © Capcom. Marmalade SDK / S3E © Marmalade Technologies.
Third-party libraries retain their own copyrights (see AUTHORS.md/LICENSE).
```

## Verification

- Log banner: `Port by Leandro Ferrete (v1.0)` and the copyright line in
  `ux0:data/redeg/logs/redeg_NNN.log`.
- SFO (`sce_sys/param.sfo`): `PORT_AUTHOR=Leandro Ferrete`,
  `PORT_VERSION=1.0`, `TITLE_ID=REDEGEN01`.
- VPK: `sce_sys/provenance.txt`.
- Release checksum: `build/SHA256SUMS.txt`.
- Source: `source/port_author.h`, `source/main.c`, `CMakeLists.txt`,
  `build.sh`, `make.sh` and `tools/`.

## Binary contents

The released VPK contains only the port executable (`eboot.bin`), SFO,
LiveArea artwork and `provenance.txt`. It does not contain any Capcom code or
data. The game executable, `main.dz`, `.icf` files, `.nib`, videos and saves
are supplied by the user from their own legally obtained, decrypted IPA.
