# Installation — Resident Evil: Degeneration (PS Vita)

> Player guide. The VPK contains only the loader and LiveArea artwork; the
> game data comes from your legally obtained, decrypted iOS IPA. See
> [`../LEGAL.md`](../LEGAL.md).

---

## 1. What you need

- PS Vita or PSTV with CFW (HENkaku/Enso).
- [kubridge](https://github.com/TheOfficialFloW/kubridge/releases) kernel
  plugin.
- [FdFix](https://github.com/TheOfficialFloW/FdFix/releases) (recommended; do
  not use together with rePatch).
- `libshacccg.suprx` in `ur0:data/` (install it with
  [ShaRKBR33D](https://github.com/Rinnegatamante/ShaRKBR33D/releases)).
- A **legally obtained, decrypted** copy of:
  - **Resident Evil: Degeneration 1.03.00**
  - IPA: `Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa`
  - Bundle ID: `jp.co.capcom.residentevil.en`
  - ABI: ARMv6 softfp (`cryptid = 0`)
- About 50 MB free in `ux0:`.
- `redeg.vpk` (`REDEGEN01`) from the repository's **Releases** tab.

The port was developed and tested with that exact IPA. Other versions,
encrypted App Store IPAs and Android APKs are not supported.

## 2. Install the plugins

Copy `kubridge.skprx` and `fd_fix.skprx` to `ux0:tai/` and edit
`ux0:tai/config.txt`:

```
*KERNEL
ux0:tai/kubridge.skprx
ux0:tai/fd_fix.skprx
```

Reboot the console. Ensure `libshacccg.suprx` is in `ur0:data/`.

> Do not add the plugins under `*main`. Do not install `fd_fix.skprx` if you
> already use `rePatch`.

## 3. Download and verify the VPK

Download `redeg.vpk` and `SHA256SUMS.txt` from the latest release. Verify the
hash on Windows:

```powershell
Get-FileHash .\redeg.vpk -Algorithm SHA256
```

The value must match `SHA256SUMS.txt`. The VPK contains:

```
eboot.bin
sce_sys/param.sfo
sce_sys/icon0.png
sce_sys/pic0.png
sce_sys/livearea/contents/*
sce_sys/provenance.txt
```

It does **not** contain the IPA, `residentevil`, `main.dz`, `.icf` files,
videos, saves or any other game data.

## 4. Extract the iOS app

An IPA is a ZIP archive. Extract it with 7-Zip, WinRAR, or PowerShell:

```powershell
Expand-Archive -LiteralPath .\Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa `
  -DestinationPath .\redeg_ipa
```

The relevant folder is:

```
redeg_ipa/Payload/residentevil.app/
```

It contains the files the port needs:

```
residentevil      7.2 MB   Mach-O ARMv6 executable
main.dz           7.2 MB   Marmalade DZip content container
s3e.icf          12 KB     S3E runtime configuration
app.icf           9 KB     application configuration
intro.mp4         4.2 MB   intro video
MainWindow.nib    1 KB     UIKit main window archive
Info.plist
PkgInfo
Default.png
Icon.png
```

Do **not** extract, repack or rename `main.dz`. The game engine reads the
original DZip file at runtime.

## 5. Copy the game data

Create this layout on the Vita:

```
ux0:data/redeg/
  game/
    residentevil
    main.dz
    s3e.icf
    app.icf
    intro.mp4
    MainWindow.nib
    Info.plist
    PkgInfo
    Default.png
    Icon.png
  logs/
  shader_cache/
```

`logs/`, `shader_cache/` and `game/Documents/` are created automatically on
the first boot, but creating them in advance is harmless.

### Automated extraction (Windows)

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 `
  -Ipa "C:\path\to\Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa"
```

The script writes `data/game/` and the required empty folders. Copy the
contents of `data/` to `ux0:data/redeg/`.

You can also point it at an already extracted app:

```powershell
powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 `
  -AppDir "C:\path\to\Payload\residentevil.app"
```

### Manual copy

Copy the **contents** of `Payload/residentevil.app/` into
`ux0:data/redeg/game/`. Do not copy the `residentevil.app` folder itself as a
subfolder. The final path must be:

```
ux0:data/redeg/game/residentevil
```

## 6. Install and launch

1. Copy `redeg.vpk` to the Vita.
2. Install it with VitaShell.
3. Launch **Resident Evil: Degeneration** (`REDEGEN01`) from the LiveArea.
4. The first frame may take a few seconds while vitaGL creates the shader
   cache. Later boots are faster.

## 7. Saves and configuration

- Saves: `ux0:data/redeg/game/Documents/`
- The tested build creates `savedata2.i3d`.
- Logs: `ux0:data/redeg/logs/redeg_NNN.log`
- Shader cache: `ux0:data/redeg/shader_cache/`
- The port does not modify the original `s3e.icf`/`app.icf` on disk. Runtime
  overrides (64 MB memory, 32 MB DZip bucket, landscape) are applied in
  memory only.

Back up the `game/Documents/` folder before changing game data or
reinstalling.

## 8. Controls

| PS Vita input | Action |
|---|---|
| Left/right analog stick, D-pad | Move Leon |
| Cross (`×`) | Context action / fire / interact / confirm |
| Circle (`○`) | Run forward in walk mode; back/close in menus |
| Circle + Down | Quick turn (walk mode) |
| R trigger | Aim; tap to toggle, hold to aim, hold and release to holster |
| L trigger | Enter knife mode |
| L + Cross | Knife attack |
| Square (`□`) | Reload; PDA zoom out |
| Triangle (`△`) | Switch target / laser; PDA zoom in |
| Start / Select | Open the PDA; skip cutscenes |
| L1 / R1 | PDA previous / next tab |
| Front touch screen | HUD, menus and PDA |
| D-pad / left stick in menus | Move cursor |
| Cross / Circle in menus | Confirm / back |

## 9. First-boot checklist

- [ ] `ux0:data/redeg/game/residentevil` exists.
- [ ] `ux0:data/redeg/game/main.dz` exists and was not extracted.
- [ ] `s3e.icf`, `app.icf`, `MainWindow.nib` and `intro.mp4` are present.
- [ ] kubridge and FdFix are listed under `*KERNEL`.
- [ ] `libshacccg.suprx` is in `ur0:data/`.
- [ ] The IPA was decrypted before extraction (`cryptid = 0`).
- [ ] `ux0:` has at least 50 MB free.

If the game does not boot, read
[`TROUBLESHOOTING.md`](TROUBLESHOOTING.md) and check the newest log in
`ux0:data/redeg/logs/`.
