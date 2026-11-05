---
name: android-to-vita-port
description: Port Android NDK games to PS Vita with vitaGL, so_util, FalsoJNI, AudioTrack/sceAudioOut, input/touch/HUD and VPK packaging. Use for APK/ELF analysis, loader/JNI shims, black screen, silent audio, bad resampling, save/config reverse engineering and on-device debugging.
---

# Android → PS Vita port

Use this skill when porting an Android game (APK + native `.so`) to PS Vita, or
when debugging an existing Vita port: boot/loader, JNI, vitaGL/EGL, input,
touch/HUD, audio, saves, performance or packaging.

The detailed playbook is in `references/` (see the index at the end). Read the
relevant reference before making structural changes.

---

## 0. Hard rules

1. **Legal:** use the user's legally owned APK/data. Never redistribute the
   game, `.so`, assets or saves in the VPK.
2. **Log first.** No on-device log = no debugging. Add file logging, a watchdog
   and breadcrumbs before chasing bugs.
3. **One hypothesis per experiment.** Change one thing, compare the log.
4. **Measure, do not trust comments/headers/docs.** Example: the VitaSDK header
   lists 44100 Hz, but `SCE_AUDIO_OUT_PORT_TYPE_MAIN` rejects it at runtime.
5. **Never change system volume without explicit permission.**
6. **Never touch firmware/`PSP2UPDATE`/`os0:`/`vd0:`.**
7. **Back up saves/configs before editing.** Close the game before replacing a
   save.
8. **Keep the screen awake during development.** A ~110 s "freeze" is usually
   the display turning off.
9. **Preserve credits and attribution.** Keep the original authors' notices;
   add the porter's copyright in source headers, log banner, SFO/provenance and
   release checksums. Do not put visual watermarks on the game screen unless
   the user explicitly asks for them.

10. **Never reuse another port's LiveArea art.** Generate `icon0`, `pic0`,
    `bg0` and `startup` from this game's APK icon/key art. Delete/ignore
    `extras/livearea_personal/`, stale `source_icon.*` and `loading*.rgb565`
    left over from another port. Use `icon0` 128×128, `pic0` 960×544 and
    `bg0` 840×500 **full-bleed**, `startup` 280×158 (16:9), all 8-bit indexed
    PNG (`P`). See `references/09-packaging-and-release.md`.

---

## 1. Workflow

### Phase 0 — Intake

- Unzip the APK; identify ABI (`readelf -h`), `DT_NEEDED`, exports/imports,
  assets and large data packs.
- Prefer `armeabi` (ARMv5TE) or `armeabi-v7a`; Vita is 32-bit ARM.
- Classify every import: libc/libm/pthread/EGL/GLES/zlib/Android/JNI/game.
- Map the boot flow: `main`/`ZInternalMain`/`JNI_OnLoad` → init → loop.
- Decide: **loader + shims** (normal), hybrid (loader + patches), or
  reimplementation (rare).

Reference: `references/01-intake-and-analysis.md`.

### Phase 1 — Loader

- Load libs at fixed bases with `so_util`; resolve imports by dependency order
  (not alphabetically); run constructors in order.
- Build a fake Activity/ANativeActivity with the exact layout the game's JNI
  lib expects (callbacks/version words matter).
- Call the Android entry (`ZAndroidEntry`/`JNI_OnLoad`) after constructors.
- Pass the Android-style command line: `<exe> -c <content_root>`.
- Stub service/licensing, clipboard, persistent-storage reads and bionic
  atomics if they block boot.
- Use the right `metadata.xml` (application manifest, not the data container).

Reference: `references/02-loader-and-environment.md`.

### Phase 2 — JNI / FalsoJNI

- Register unknown methods by **bare name** (`write`, `play`, `getState`), not
  `Class/method`; only `<init>` carries the class prefix.
- Implement every return type used by the game (`void/int/float/boolean/object`).
- `JavaDynArray` stores contiguous elements: 1 byte for `byte[]`, 2 for
  `short[]`, 4 for `int[]`. Never assume a stride.
- Return plausible values: platform string `"android"`, language, storage
  state, package name, file dirs, empty lists/certs.
- Log registrations and calls; a missing method usually shows as the last
  `registered ...`/`GetMethodID ... not found`.

Reference: `references/03-jni-and-android-apis.md`.

### Phase 3 — Graphics / first frame

- EGL/GLES2 over vitaGL; `libshacccg.suprx` + vitaShaRK for runtime GLSL.
- Hook display size/mode functions to 960×544; fill the zeroed display setup.
- Force `zpgfxDisplayIsValid` to 1 if the engine never marks it valid.
- Use this hook to poll input on the main thread every frame.
- Patch code only after verifying opcodes; write with
  `kuKernelCpuUnrestrictedMemcpy` and flush caches. If opcodes differ, skip.

Reference: `references/04-graphics-and-display.md`.

### Phase 4 — Input / touch / HUD

- Prefer engine-level queues: `zinpGamepadPushButtonState` (keys) and
  `zinpTouchPushState` (touch); do not call `GameClass::OnInputEvent` from a
  poll thread.
- Poll on the main thread (display-valid hook); dedicated threads can block
  after slow boots.
- Touch struct: flags `1=down, 4=move, 2=up`; centered normalized positions at
  `+20/+24` (`x_px=(x+0.5)*width`).
- Discover HUD rectangles via `CTouchScreen::AddButton`/`MouseDown` logs.
- Fight HUD is often touch-only: map attacks to synthetic touches; movement to
  a drag inside the virtual pad.
- Combine analog + D-pad into one vector; zero stick drift below the deadzone;
  normalize diagonals. Keep key events for menus/pause.
- **Pause menus may use a different confirm key.** Example: main menu confirms
  with A (Cross), but `gui/pause.ui` confirmed with Select. Detect
  `Entering state: GamePause`, translate Cross→Select there, disable fight HUD
  touches while paused (synthetic taps can hit pause buttons), and use a
  RESUME tap or B for Circle depending on the pause submenu.
- Auto-tap only on "touch to continue" states.

Reference: `references/05-input-touch-hud.md`.

### Phase 5 — Audio

- Find the path: OpenSL ES, AudioTrack JNI, or native driver.
- If OpenSL is stubbed and the game falls back to `AudioTrack`, implement the
  JNI methods with the correct signatures:
  `getMinBufferSize(III)I`, `<init>(IIIIII)V`, `getState()I=1`,
  `getPlayState()I=3`, `play/pause/stop/release`, `setStereoVolume(FF)I`,
  `write([BII)I` (**byte[]**, offset in bytes), `flush`, `getMax/MinVolume()F`.
- Open `sceAudioOut` MAIN at **48000 Hz only** (44100 returns `0x80260008`),
  grain 512, volume `SCE_AUDIO_OUT_MAX_VOL` (32768).
- Resample 44100→48000 with a **polyphase Kaiser FIR**, not linear/cubic
  (cubic aliases 15 kHz to -13.8 dB). Reference design: 1024 phases × 48 taps,
  `fc=0.42`, Kaiser beta 10, normalize each phase, ring buffer, pre-pad K.
- Measure PCM peak/RMS; apply digital gain only if the mix is quiet, with
  clamp. Do not change system volume.
- Diagnose in order: port opened → `write` called → PCM non-zero → rate
  correct → volume/quality.
- Remove PCM dumps/per-sample logs from release builds.

Reference: `references/06-audio.md`.

### Phase 6 — Save/config/filesystem

- Redirect `/sdcard`, `/data/data/<pkg>`, `/content0`, `/game`, `/save` to
  `ux0:data/<game>`.
- Reverse the save/config format: identify magic, key table, value records,
  floats. Back up, edit with the game closed, rezip preserving compression.
- Re-launch and confirm the game read the new value (config dump/log).

Reference: `references/07-save-config-filesystem.md`.

### Phase 7 — On-device debugging

- One log file per run; dedupe repeated lines; log boot, hooks, display,
  input, audio, errors.
- Use `vita_wait` for log/file/crash instead of fixed sleeps; keep-awake on.
- If FTP hangs, reboot. If `promote` fails, overwrite `eboot.bin` via FTP.
- Input automation: send one press, verify `[pad] buttons=`, then continue.
  `SCE_CTRL_INTERCEPTED` (`0x00010000`) means the system swallowed it.
- Check crash dumps, watchdog marks, FPS/frame time, GL errors.

#### Phase 7.1 — VitaMCP loop (use every iteration)

```
build -> keepawake -> close app -> install VPK -> launch
      -> vita_wait(log, pattern) -> input (one press, verify in log)
      -> pull log -> analyze -> edit -> repeat
```

- `vita_keepawake(enabled=true)`; `vita_close_app(all)` before install.
- `vita_install_vpk(vpk)`; if `promote` fails, verify/overwrite
  `ux0:/app/<TITLEID>/eboot.bin`.
- `vita_launch_app(TITLEID)`; don't send input before the UI is ready.
- `vita_wait(for:"log", target:".../<run>.log", pattern:"...")`; the log
  number changes per launch — list `logs/` and pick the newest if unsure.
- `vita_input` one press at a time; confirm `[pad] buttons=` and the state
  transition. `0x00010000` = intercepted, retry.
- `vita_pull` the log (or FTP fallback) and grep the last state/error.
- On FTP hang: `vita_ftp_status` → `vita_ftp_ensure`/`vita_reboot(wait=true)`.
- Leave the console in a known state (title/menu, sound on), keep-awake on.

Reference: `references/12-vitamcp-workflow.md`.

### Phase 8 — Packaging/release

- VPK contains `eboot.bin` + `sce_sys` + LiveArea, not the game data.
- LiveArea art from **this** game's APK icon/key art: `icon0` 128×128,
  `pic0` 960×544 full-bleed, `bg0` 840×500 full-bleed, `startup` 280×158
  (16:9), all 8-bit indexed PNG (`P`). No leftovers from another port.
- When replacing art manually, update `ux0:/app/<ID>/sce_sys`,
  `ux0:/appmeta/<ID>` and `ur0:/appmeta/<ID>` if present, then reboot.
- Use `vita-mksfoex` + `vita-pack-vpk` if the CMake macro breaks on spaces.
- Test on a clean console; verify home bubble and LiveArea art, boot/menus/
  save/load/gameplay/pause/audio/controls/30 min stability. Remove debug dumps.

Reference: `references/09-packaging-and-release.md`.

---

## 2. Top pitfalls (symptom → cause → fix)

| Symptom | Cause | Fix |
|---|---|---|
| `.so` won't load | kubridge/base/memory/path | check loader log, bases, file presence |
| `Unresolved import` | missing shim / wrong dependency order | add to `dynlib` or fix order |
| Stops in system init | fake Activity/JNI/command line/metadata | inspect last JNI line, `-c`, manifest |
| No packages found | virtual path/pak mapping | fix `opendir`/`content_dir` |
| Black screen, loop alive | display-valid/viewport/format | force valid, 960×544, GL state |
| Controls dead after boot | dedicated poll thread blocked | poll on main thread |
| Touch lands wrong | raw pixels vs centered normalized | use `(px/w)-0.5` |
| Attacks do nothing | HUD is touch-only | send synthetic touches |
| Audio port opens, silence | `write` not dispatched / track discarded | handle bare names + `getState=1` |
| `write` called, PCM zero | game muted/config/callback | enable sound, fix volume, check callback |
| Audio quiet | mix low / no gain | measure peak, apply gain with clamp |
| Audio noisy/harsh | linear/cubic resampling aliasing | polyphase FIR |
| Clicks/cuts | underrun/I/O in audio thread/grain | remove dumps, check rates/grain |
| "Freeze" ~110 s | display off | keep-awake/power ticks |
| Automated input ignored | `SCE_CTRL_INTERCEPTED` | single press, verify log, retry |
| `vita_wait` times out | wrong log file / pattern already passed | list newest log, pull and grep |
| Install looks OK but code is old | `promote` failed | verify `eboot.bin` date, overwrite via FTP |
| Save ignored | edited while game ran / wrong path | close app, fix path/format |
| Flickering/corrupt client-memory meshes after zero-copy optimization | GPU may consume arrays after CPU overwrites/frees them | copy client vertex/index arrays into storage retained through GPU completion; preserve normal VBO tracking. Index count and mapped memory do not prove lifetime |
| Animated meshes explode and CPU quaternions exceed their fixed-point unit scale | a heuristic loader may have rebased a packed numeric math table as pointers | compare the pre-entry loaded image to the original table, replay the engine math helpers and preserve verified numeric ranges; see case 17. Enlarging/clamping skinning output can hide a secondary symptom |
| A delayed vertex trace shows implausible coordinates | the trace may hold a pointer into a reused scratch buffer | snapshot inputs/outputs at the producer and compare positions at draw time before concluding CPU corruption |
| Character has corrupt reflections or alpha | possible missing split-alpha texture or incomplete cubemap mip chain | inspect actual shader inputs/import hooks; do not infer a clamp exists from documentation or force alpha globally |

For TASM, read the 2026-11-04 audit in
`references/13-case-study-gameloft-tasm.md` before reusing offsets or claiming
input/graphics fixes. The baseline lacked the documented 45 ms key hold,
touchscreen `+2` offset and cubemap clamp. Never use `count > 64` to authorize
zero-copy client buffers. Record the ELF digest, build flags and tested scene;
menu FPS is not a moving-gameplay benchmark. Rebuild all vitaGL objects when
flags change because its Makefile does not track `CFLAGS` changes.

---

## 3. Code recipes

### 3.1 Safe ARM branch patch

```c
static uint32_t arm_branch(uintptr_t from, uintptr_t to) {
    int32_t off = (int32_t)(to - (from + 8));
    return 0xEA000000u | (((uint32_t)(off >> 2)) & 0x00FFFFFFu);
}
/* verify old opcode, then: */
kuKernelCpuUnrestrictedMemcpy(dst, &insn, 4);
kuKernelFlushCaches(base, size);
```

### 3.2 AudioTrack JNI essentials

```c
/* methods are registered by bare name! */
if (!strcmp(n, "write")) {
    jbyteArray arr = va_arg(args, jbyteArray);
    int offset = va_arg(args, int);
    int size   = va_arg(args, int);
    JavaDynArray *jda = (JavaDynArray *)arr;
    const uint8_t *pcm = (const uint8_t *)jda->array + offset;
    feed_audio((const int16_t *)pcm, size / (2 * channels));
    return size;
}
if (!strcmp(n, "getState"))     return 1;
if (!strcmp(n, "getPlayState")) return 3;
```

### 3.3 FIR resampler (44.1→48)

- 1024 phases, 48 taps, `fc=0.42`, Kaiser beta 10.
- Build `coef[p][k] = sinc(2fc*(k-frac)) * kaiser((k-frac)/K)`, normalize sum.
- Ring buffer 8192 frames, pre-pad K zeros, `step = in_rate/out_rate`.
- Process while `out_pos + K < total_in`; fetch with `idx & (RING-1)`.
- Output 512-frame grains to `sceAudioOut`.

See `references/06-audio.md` for the full code.

### 3.4 Touch event

```c
typedef struct {
    int device; int flags; unsigned short id, pad;
    float unused_x, unused_y;
    float pos_x, pos_y;   /* centered normalized: px = (x+0.5)*width */
} zp_touch_state_t;
/* flags: down=1, move=4, up=2; types: 7/6/8 */
```

### 3.5 Analog + D-pad vector

```c
float dx = (stick_x - 128)/127.0f, dy = (stick_y - 128)/127.0f;
if (dx*dx + dy*dy < 0.1225f) { dx = dy = 0; } /* 0.35 deadzone */
if (btn & LEFT) dx -= 1; if (btn & RIGHT) dx += 1;
if (btn & UP)   dy -= 1; if (btn & DOWN)  dy += 1;
float m2 = dx*dx + dy*dy;
if (m2 > 1) { float inv = 1/sqrtf(m2); dx *= inv; dy *= inv; }
```

---

## 4. Command cheat sheet

```bash
# ELF
arm-vita-eabi-readelf -h/-d/-sW game.so
arm-vita-eabi-nm -D --defined-only game.so | sort
arm-vita-eabi-nm -D -u game.so | sort
arm-vita-eabi-objdump -d --start-address=0x... --stop-address=0x... game.so
arm-vita-eabi-strings -t x game.so | less

# APK
unzip -l game.apk
unzip game.apk -d apk

# Build/package (MSYS2)
bash build.sh && bash make.sh
# out: build/<game>.vpk
```

On-device: FTP 1337, command 1338, logs `ux0:data/<game>/logs/`,
`vita_wait`, `vita_keepawake`, `vita_pull/push`, `vita_crashes`.

---

## 5. Definition of done

- Boot to menus on a clean install.
- Menus/save/load/options work.
- Gameplay, pause/resume, all physical + touch controls.
- Audio: music/SFX, correct volume, no clipping/noise.
- Stable 30+ min; no crash; save not corrupted.
- VPK installs/promotes; data path documented.
- Debug dumps/log spam removed; credits/license respected.

---

## 6. References

- `references/17-redeg-graphics-investigation.md` — Resident Evil:
  Degeneration iOS/Mach-O investigation, hardware experiments and evidence.
  Read when resuming REDEGEN01, auditing heuristic rebasing of numeric pools,
  or distinguishing malformed CPU skinning from asynchronous GPU buffers.
  This case uses a custom Mach-O/ObjC loader, not Android so_util/JNI.

- `references/15-tasm-complete-work-record.md` — full recoverable TASM investigation: every documented experiment, commands, errors, results, evidence paths and explicitly pending work. Read to resume this port or audit a previous decision. Updated with `py tools/update_port_knowledge.py` in tasm-vita; this also mirrors the skill and refreshes evidence hashes.
- `references/16-tasm-inherited-notes.md` — complete archived implementation notes from the initial port. Historical claims and offsets can be wrong; current references 14/15 take precedence. Read only when reconstructing old loader/JNI/pthread/I/O/audio work.
- `references/14-tasm-mipmaps-and-frame-time.md` — TASM hardware findings: mipmaps, cubemap face stride, streaming/LOD, geometry compaction, rejected caches/copies, memory budgets, calibrated sticks and capture freshness. Read before reusing older TASM graphics/performance advice.

- `references/01-intake-and-analysis.md` — APK/ELF/imports/boot mapping.
- `references/02-loader-and-environment.md` — so_util, Activity, stubs, boot.
- `references/03-jni-and-android-apis.md` — FalsoJNI, signatures, arrays.
- `references/04-graphics-and-display.md` — vitaGL/EGL, modes, patches.
- `references/05-input-touch-hud.md` — input queues, HUD, D-pad, automation.
- `references/06-audio.md` — AudioTrack/sceAudioOut/FIR/volume.
- `references/07-save-config-filesystem.md` — paths, save format RE.
- `references/08-debugging-on-device.md` — logs, FTP, keep-awake, perf.
- `references/09-packaging-and-release.md` — VPK/SELF/LiveArea/release.
- `references/10-checklists.md` — printable checklists + symptom table.
- `references/11-case-study-bruce-lee.md` — full real-world case study (Digital Legends KRM / ZSystem).
- `references/12-vitamcp-workflow.md` — VitaMCP loop: install/launch/wait/
  input/pull log, FTP/promote/reboot, safety rules.
- `references/13-case-study-gameloft-tasm.md` — Gameloft engine case study (The Amazing Spider-Man HD: Bionic 4-byte pthread ABI, FalsoJNI `NewObject` `<init>` dispatch, Xperia Play + dual virtual touchpad sticks, FTP `TIME_WAIT` pacing).

> The case study is illustrative. **Never reuse offsets/coordinates** from it;
> rediscover them for each binary/version.
