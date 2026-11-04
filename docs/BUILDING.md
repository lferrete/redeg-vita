# Building the port

> Developer guide. The repository does not include the IPA, the game
> executable, content containers, saves or any other game data.

---

## 1. Requirements

- **VitaSDK** built with the softfp ABI (`arm-vita-eabi`).
- **MSYS2** with `make`, `cmake`, `bash` and the usual Unix tools.
- PowerShell on Windows for `tools/make_vpk.ps1`.
- The vendored dependencies in `lib/`:
  - `vitaGL` (softfp build, invoked by CMake);
  - `kubridge` headers/stubs;
  - `vitaShaRK`/`SceShaccCg`;
  - `zlib`, `newlib`, `libstdc++` from VitaSDK.
- Python 3 + Pillow only if you want to regenerate LiveArea art.

The port does **not** use `so_util` or FalsoJNI: the game is an iOS Mach-O
ARMv6 binary, loaded by the custom loader in `source/macho/`.

## 2. Configure and build

From the repository root, in MSYS2:

```bash
export VITASDK=/c/vitasdk
export PATH="$VITASDK/bin:/usr/bin:$PATH"

bash build.sh     # rm -rf build; configure CMake
bash make.sh      # make -j8
```

Output:

```
build/redeg
build/eboot.bin
build/eboot.bin.self
```

## 3. Pack the VPK

```powershell
powershell -ExecutionPolicy Bypass -File tools\make_vpk.ps1
```

Output:

```
build/redeg.vpk
build/SHA256SUMS.txt
```

The script calls `vita-mksfoex` and `vita-pack-vpk` with native Windows
quoting (the CMake `vita_create_vpk` helper can break on paths containing
spaces). It packs:

```
eboot.bin
sce_sys/param.sfo
sce_sys/icon0.png
sce_sys/pic0.png
sce_sys/livearea/contents/bg0.png
sce_sys/livearea/contents/startup.png
sce_sys/livearea/contents/template.xml
sce_sys/provenance.txt
```

It does **not** pack `residentevil`, `main.dz`, `.icf`, `.nib`, videos,
saves, textures, audio or any other game data.

## 4. LiveArea artwork

The tracked artwork lives in `extras/livearea/`:

```
extras/livearea/icon0.png     128×128
extras/livearea/pic0.png      960×544
extras/livearea/bg0.png       840×500
extras/livearea/startup.png   280×158
extras/livearea/template.xml
```

The art is derived from the original game's `iTunesArtwork` and remains the
property of its rights holders. It is included only to identify the
unofficial port.

To regenerate it from **your own legally obtained IPA**:

```powershell
powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 `
  -Ipa "C:\path\to\Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa"
py -3 tools\make_livearea.py
```

`prepare_data.ps1` extracts `iTunesArtwork` to
`extras/livearea/source_icon.png`. The generated files replace the tracked
artwork; review the image diff before committing. The source artwork is
ignored by git.

### Art rules (do not repeat the Bruce Lee mistake)

This project originally inherited `extras/livearea/` and
`extras/livearea_personal/` from a previous port, so the Vita bubble showed
Bruce Lee. A second issue was a **blank white bubble**: the icon was saved as
32-bit RGBA, while the LiveArea expects an **8-bit indexed PNG (`P`)**.

- Generate all art from **this game's** `iTunesArtwork`/`Icon.png`.
- Delete or ignore `extras/livearea_personal/`, stale `source_icon.*` and
  old `loading*.rgb565` before building.
- Use `icon0` 128×128, `pic0` 960×544 and `bg0` 840×500 **full-bleed**
  (cover crop, not a square 1:1 image centred on a gradient), and `startup`
  280×158 (16:9). Do not letterbox the key art; it reads as 4:3 with bars.
- Save all four as 8-bit indexed PNG (`P` mode), 256 colours.
- Check the sizes/modes before building:

```powershell
py -3 -c "from PIL import Image; [print(n, Image.open('extras/livearea/'+n).size, Image.open('extras/livearea/'+n).mode) for n in ['icon0.png','pic0.png','bg0.png','startup.png']]"
```

When replacing art manually on the console, update
`ux0:/app/REDEGEN01/sce_sys`, `ux0:/appmeta/REDEGEN01` and
`ur0:/appmeta/REDEGEN01` (if present), then reboot. The LiveArea icon path in
`ur0:/shell/db/app.db` may point at `ur0:appmeta/REDEGEN01/icon0.png`, not
`ux0:`. If the bubble is blank, check the PNG mode first.

## 5. Local game data for testing

Use your own decrypted IPA:

```powershell
powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 `
  -Ipa "C:\path\to\Resident_Evil__Degeneration_1.03.00_ios_2.2.1.ipa"
```

The result is written to `data/` (git-ignored). Copy it to
`ux0:data/redeg/` on the Vita.

## 6. Minimum release test

- [ ] Clean boot with a fresh `ux0:data/redeg/` (no old shader cache).
- [ ] Main menu, New Game, Continue and Load Game.
- [ ] 3D gameplay at 960×544, no missing geometry or corrupted UVs.
- [ ] Audio (music, SFX, voice) through `sceAudioOut`.
- [ ] Left/right stick, D-pad, face buttons, triggers and touch.
- [ ] PDA, communications, cutscenes and QTEs.
- [ ] Save and reload; check `game/Documents/savedata2.i3d`.
- [ ] 30+ minutes without a crash.
- [ ] `unzip -l build/redeg.vpk` shows no game data.
- [ ] `SHA256SUMS.txt` matches the final VPK.

## 7. Layout

```
source/macho/      Mach-O parser, segments, relocations, rebase
source/objc/       minimal ObjC runtime + Foundation/UIKit/EAGL classes
source/shims/      Darwin libc, CoreFoundation, CoreGraphics, audio, GL
source/utils/      crash-safe logs, vitaGL init/present
lib/               vendored dependencies
tools/             prepare_data, make_livearea, make_vpk, import audit
extras/livearea/   tracked LiveArea art + template
docs/              player and developer documentation
```

Internal reverse-engineering notes (offset maps, port log, status) are
**not** part of the public repository and are listed in `.gitignore`.
