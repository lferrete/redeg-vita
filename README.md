# Resident Evil: Degeneration — PS Vita (unofficial port)

Unofficial native port of the iOS game **Resident Evil: Degeneration**
(Capcom, v1.03.00, bundle `jp.co.capcom.residentevil.en`, ARMv6) to the
**PS Vita**, using a custom Mach-O loader, a minimal Objective-C runtime, and
iOS framework shims over [vitaGL](https://github.com/Rinnegatamante/vitagl).

> **Legal notice:** this is an unofficial, non-commercial fan project. It is
> not affiliated with, endorsed by, or sponsored by Capcom, Sony Interactive
> Entertainment, Marmalade Technologies, or any of their affiliates. The
> repository and the released VPK **do not contain** the IPA, the game
> executable (`residentevil`), `main.dz`, `s3e.icf`/`app.icf`, videos, saves,
> or any other game data. You must provide a **legally obtained, decrypted
> copy** of the iOS game. Read [`LEGAL.md`](LEGAL.md) before installing or
> redistributing.

**Repository:** <https://github.com/lferrete/redeg-vita><br>
**Release:** <https://github.com/lferrete/redeg-vita/releases/latest>

---

## Status

- Tested with the iOS version **1.03.00** (IPA
  `Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa`, bundle
  `jp.co.capcom.residentevil.en`, ARMv6 softfp, `cryptid = 0`).
- Native **960×544** landscape rendering through vitaGL at **60 FPS**, with
  the original `480×320` GLES1 framebuffer scaled to full screen.
- Audio through the original `AudioQueue` API translated to
  `sceAudioOut`: 16 kHz mono PCM with digital gain.
- Physical PS Vita controls plus the front touch screen:
  - Left/right analog stick and D-pad move Leon.
  - Cross is the context action, fire button and menu confirm.
  - Circle runs forward in walk mode, triggers the quick turn with Down,
    and goes back/closes menus.
  - R aims; L enters knife mode.
  - Square reloads; Triangle switches target / toggles the laser.
  - Start/Select open the PDA and skip cutscenes.
  - The front touch screen is fully supported (HUD, menus and PDA).
- Saves work in `ux0:data/redeg/game/Documents/`.
- The VPK (`redeg.vpk`, Title ID `REDEGEN01`) is **loader-only**; the game
  data stays outside it.

## Requirements

1. PS Vita / PSTV with CFW (HENkaku/Enso).
2. [kubridge](https://github.com/TheOfficialFloW/kubridge/releases) kernel
   plugin.
3. [FdFix](https://github.com/TheOfficialFloW/FdFix/releases) (recommended;
   do not use together with rePatch).
4. `libshacccg.suprx` in `ur0:data/` — install via
   [ShaRKBR33D](https://github.com/Rinnegatamante/ShaRKBR33D/releases) or the
   [extraction guide](https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx).
5. A **legally obtained, decrypted IPA** of **Resident Evil: Degeneration
   1.03.00** (`jp.co.capcom.residentevil.en`, ARMv6).
6. About **50 MB free** in `ux0:` for the game data, saves, logs and the VPK.

## Installation (players)

### 1. Install the plugins

Copy `kubridge.skprx` and `fd_fix.skprx` to `ux0:tai/` and add them to
`ux0:tai/config.txt`:

```
*KERNEL
ux0:tai/kubridge.skprx
ux0:tai/fd_fix.skprx
```

> If you use `rePatch`, do not install `fd_fix.skprx`.

Reboot the console. Ensure `libshacccg.suprx` is present in `ur0:data/`.

### 2. Download the VPK

Get `redeg.vpk` from the repository's **Releases** tab and keep
`SHA256SUMS.txt` to verify the download. The VPK contains only `eboot.bin`,
the SFO, the LiveArea artwork and `sce_sys/provenance.txt` — **not** the game.

### 3. Prepare the game data

The port expects the contents of `Payload/residentevil.app/` from your legal
IPA under `ux0:data/redeg/game/`. Do **not** extract or rename `main.dz`; the
game engine reads it as a DZip container.

```
ux0:data/redeg/
  game/
    residentevil          <- Mach-O executable from Payload/residentevil.app/
    main.dz               <- keep the original file, do not extract
    s3e.icf
    app.icf
    intro.mp4
    MainWindow.nib
    Info.plist
    PkgInfo
    Default.png
    Icon.png
    Documents/            <- created automatically; saves live here
  logs/                   <- created automatically
  shader_cache/           <- created automatically
```

The `residentevil` executable, `main.dz`, `s3e.icf`, `app.icf` and
`MainWindow.nib` are mandatory. `intro.mp4` is required for the intro
cutscene.

On Windows, `tools/prepare_data.ps1` automates the extraction:

```powershell
powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 `
  -Ipa "C:\path\to\Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa"
```

It writes the layout to `data/`. Copy the contents of `data/` to
`ux0:data/redeg/` (for example with VitaShell or FTP).

> The repository does **not** include the IPA, `residentevil`, `main.dz`,
> `.icf` files, `.nib`, videos, saves, or any other game asset. Do not ask
> for, post, or link those files in issues.

### 4. Install and launch

Install `redeg.vpk` with VitaShell and launch **Resident Evil: Degeneration**
(`REDEGEN01`) from the LiveArea. The first boot creates `logs/`,
`shader_cache/` and the save folder.

## Controls

| PS Vita input | In-game action |
|---|---|
| Left or right analog stick / D-pad | Move Leon |
| Cross (`×`) | Context action / fire / interact / confirm |
| Circle (`○`) | Run forward (walk mode), back/close menus; Circle + Down = quick turn |
| R trigger | Aim (tap to toggle, hold to aim; hold and release to holster) |
| L trigger | Knife mode; L + Cross attacks with the knife |
| Square (`□`) | Reload weapon; PDA map zoom out |
| Triangle (`△`) | Switch target / toggle laser; PDA map zoom in |
| Start / Select | Open the PDA; skip cutscenes |
| L1 / R1 | PDA previous / next tab |
| Front touch screen | HUD, menus and PDA |
| D-pad / left stick in menus | Move the cursor |
| Cross / Circle in menus | Confirm / back |

## Audio

- The port translates `AudioQueue` to `sceAudioOut` at 16 kHz mono.
- The port does not change the system volume or mute state.
- If there is no audio, check the Vita volume, the in-game options, and the
  log lines tagged `[audio]`.

## Saves

- Saves are written to `ux0:data/redeg/game/Documents/` (the engine uses
  `HOME/Documents`).
- The tested build creates `savedata2.i3d`.
- Back up that folder before reinstalling or deleting the game data.
- The repository never includes save files.

## Troubleshooting

- **Black screen or return to LiveArea:** check kubridge and FdFix in
  `ux0:tai/config.txt`, `ur0:data/libshacccg.suprx`, and that
  `ux0:data/redeg/game/residentevil` exists.
- **"residentevil not found" / boot loop:** the IPA was not extracted to
  `ux0:data/redeg/game/` or the executable was renamed.
- **Missing `main.dz`:** copy it from `Payload/residentevil.app/` without
  extracting it.
- **Encrypted IPA / crash before the first frame:** the port needs a
  decrypted copy (`cryptid = 0`). A retail App Store IPA is encrypted and
  will not work.
- **No audio:** check the Vita volume, in-game audio options, and
  `ux0:data/redeg/logs/redeg_NNN.log`.
- **Wrong orientation or stretched image:** delete the shader cache once and
  reboot; make sure no display/overlay plugin is forcing a resolution.
- **Old LiveArea art cached:** reboot the console or refresh the LiveArea.

Logs are written to:

```
ux0:data/redeg/logs/redeg_NNN.log
```

When reporting a bug, include the relevant log excerpt and your console/CFW
version. **Never** attach the IPA, `residentevil`, `main.dz`, `.icf` files,
saves or other game data.

Detailed guides: [`docs/INSTALL.md`](docs/INSTALL.md) and
[`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md).

## Building from source

Requirements: a softfp VitaSDK, MSYS2 with `make`/`cmake`/`bash`, and the
vendored libraries in `lib/` (vitaGL, kubridge, vitaShaRK/SceShaccCg).

```bash
cd redeg-vita
bash build.sh     # configure CMake in build/
bash make.sh      # compile build/eboot.bin
```

Pack the VPK with:

```powershell
powershell -ExecutionPolicy Bypass -File tools\make_vpk.ps1
# build/redeg.vpk + build/SHA256SUMS.txt
```

The LiveArea art is tracked in `extras/livearea/` and is used by default.
`tools/make_livearea.py` can regenerate it from the `iTunesArtwork` inside
**your own legally obtained IPA**; review the image diff before committing.
The VPK build does not include the artwork source file.

## Data and privacy

- `data/`, `build/`, saves, logs, the IPA and the extracted game are in
  `.gitignore`.
- The release VPK does not contain game data.
- The port has no telemetry and collects no data.

## Credits

- **Port, Mach-O loader, ObjC runtime, iOS shims, input/audio, documentation:**
  Leandro Ferrete.
- **Original game:** Capcom. All rights reserved.
- **Marmalade SDK / S3E:** Marmalade Technologies and contributors.
- **vitaGL:** Rinnegatamante and contributors.
- **kubridge:** TheFloW.
- **vitaShaRK / SceShaccCg / ShaRKBR33D:** Rinnegatamante and contributors.
- **VitaSDK, zlib, newlib, libstdc++:** their respective authors.

See [`AUTHORS.md`](AUTHORS.md) and [`LICENSE`](LICENSE).

## Documentation

- [`docs/INSTALL.md`](docs/INSTALL.md) — step-by-step installation.
- [`docs/BUILDING.md`](docs/BUILDING.md) — build and package.
- [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) — common problems.
- [`docs/AUTHORSHIP.md`](docs/AUTHORSHIP.md) — authorship and verification.
- [`docs/PUBLISHING.md`](docs/PUBLISHING.md) — repository settings and
  releases.
- [`LEGAL.md`](LEGAL.md) — legal notice and takedown.
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — contribution rules.
