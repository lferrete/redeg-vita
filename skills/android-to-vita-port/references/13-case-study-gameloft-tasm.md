# Case Study — The Amazing Spider-Man HD (Gameloft Engine → PS Vita)

> Technical case study of the **The Amazing Spider-Man HD** (`com.gameloft.android.ANMP.GloftAMHM`, v1.2.3, `libSpiderMan.so`) port to PS Vita (`TASMVITA1`). Documents reusable patterns for Gameloft 2012–2014 engines (`vox::DriverAndroid`, `AndroidOS`, `Xperia`, `XperiaTouchpad`, Scaleform GFx `.swf`, and `.gla` GLSL shader packs).

**Audit 2026-11-04:** the baseline still reproduces visual corruption. Read
section 7 before reusing historical fixes or offsets. Sections not rechecked
against the identified ELF remain historical observations, not current hardware
validation. Corrections since that audit, bounded hardware validation and open
defects are recorded in references 14 and 15; those take precedence over the
baseline descriptions below. Full inherited notes are archived in reference 16.

---

## 1. Target Profile

- **APK:** `The-Amazing-Spider-Man-HD.apk` (v1.2.3)
- **Package:** `com.gameloft.android.ANMP.GloftAMHM`
- **Binary audited 2026-11-04:** `lib/armeabi-v7a/libSpiderMan.so` (12,933,332 bytes, ELF32 ARMv7-A, softfp, non-stripped C++ symbols)
- **SHA-256:** `c3de282d87fece00b797f23bb0b7cdfe778135cdbfd5678bee19484ce1b5d040`. Offsets are valid only for this identity when explicitly rechecked.
- **Graphics:** OpenGL ES 2.0 (`effects.gla` GLSL shader pack)
- **Audio:** `vox::DriverAndroid` (`android.media.AudioTrack` JNI)
- **UI:** Scaleform GFx (`menu/` and `menuXperia/` SWF files + `swf.pak`)

---

## 2. Bionic `pthread_mutex_t` & `pthread_mutexattr_t` ABI Mismatch

### Problem
Gameloft's `CriSec` critical section wrapper (`CriSec::__lock` at `0x4ca94c`) embeds Bionic's 4-byte `pthread_mutex_t` inside C++ objects and initializes recursive mutexes using a 4-byte `pthread_mutexattr_t` on the stack:
```c
int attr; // 4 bytes on Bionic!
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
pthread_mutex_init(&this->mutex, &attr);
pthread_mutexattr_destroy(&attr);
```
In VitaSDK (Newlib), `pthread_mutexattr_t` is a multi-word struct. Forwarding `pthread_mutexattr_init` directly to Newlib overwrites adjacent stack variables and corrupts the caller's stack frame. Furthermore, if `pthread_mutex_init` is lazily initialized on first `pthread_mutex_lock` without preserving the recursive attribute, `CriSec` deadlocks on re-entrant locks on the same thread.

### Solution (`source/reimpl/pthr.c`)
1. Implement `pthread_mutexattr_init_soloader`, `pthread_mutexattr_settype_soloader`, and `pthread_mutexattr_destroy_soloader` directly on a 4-byte `int *`.
2. Implement `pthread_condattr_init_soloader` and `pthread_condattr_destroy_soloader` on a 4-byte `int *`.
3. Back `pthread_mutex_*_soloader` with `SceKernelLwMutexWork` created with `SCE_KERNEL_MUTEX_ATTR_RECURSIVE`, storing the heap pointer directly in the 4-byte Bionic `pthread_mutex_t` slot (recognizing static initializers `0x00000000`, `0x00004000`, `0x00008000`).

---

## 3. FalsoJNI Constructor Dispatch (`NewObject`) & Dummy Token Guards

### Problem 1: `AudioTrack` Constructor Not Called via `NewObject`
In `FalsoJNI.c`:
- `GetMethodID(env, clazz, "<init>", sig)` formats the lookup name as `"<clazz>/<init>"` (e.g., `"android/media/AudioTrack/<init>"`), using the string returned by `FindClass`.
- `NewObject(env, clazz, methodID, ...)` dispatches through `methodObjectCall(methodID, args)` (`jni_shim_object_call`), **not** `methodVoidCall` (`jni_shim_void_call`), and returns the resulting `jobject` to the caller.
- If `<init>` is registered only as `"<init>"` or with `METHOD_TYPE_VOID`, `NewObject` returns `NULL` and `vox::DriverAndroid::Init` aborts audio initialization.

### Solution
Register `"android/media/AudioTrack/<init>"` with `METHOD_TYPE_OBJECT` and handle `MID_AT_CTOR` inside `jni_shim_object_call`, opening `sceAudioOut` and returning a non-NULL `jobject` token (`(jobject)0x42424242`).

### Problem 2: `GetStringUTFChars` on Dummy `jobject` Tokens
When `CallObjectMethod` returns `(jobject)0x42424242` for unmapped methods, native code may pass that token to `GetStringUTFChars` or `GetStringUTFLength`, which casts it to `JavaString *` and dereferences `0x42424242`.
- Guard `GetStringUTFChars` and `GetStringUTFLength` in `FalsoJNI.c`: if `!string || (uintptr_t)string == 0x42424242u || (uintptr_t)string < 0x10000u`, return `""` / `0`.

---

## 4. Device Profile & Shader Pack Selection (`Application::PostInit`)

Disassembly of `Application::PostInit` (`0x24c800`) and `Application::InitFileSystem` (`0x24d840`) showed:
1. `AndroidOS::GetPhoneModel()` is matched against device substrings (`GT-I9100`, `Galaxy Nexus`, `GT-I9300`, `Xperia Play`, etc.) to select `config_android_*.xml` and set quality flags (`isI9300`).
2. `config_android_avg.xml`, `config_android_medphone.xml`, `config_android_nexus.xml`, and `config_android_s3.xml` are identical (`MaxTextureSize=1024`, `BufferSize=25`, `Use2XMap=1`, `PostEffect_OnOff=1`).
3. However, if `isI9300 != 0` (e.g., `GT-I9300`), `Application::InitFileSystem` loads `2xswf.pak` (46 MB) instead of `swf.pak` (23 MB) and `effects2x.gla` instead of `effects.gla`.
4. Returning `"Galaxy Nexus"` from `GetPhoneModel()` selects `config_android_nexus.xml` while keeping `isI9300 = 0`, loading `effects.gla` and `swf.pak` and saving 23 MB of RAM.
5. Because `effects.gla` compiles >64 shader programs, increase `UNIFORM_MAX_PROGRAMS` in `source/utils/glutil.c` from `64` to `256`.

---

## 5. Native Xperia Play Gamepad + Dual Virtual Touchpad Analog Sticks

### 5.1 Enabling Xperia Play Mode
Calling `Java_com_gameloft_android_ANMP_GloftAMHM_Game_nativeSetXperia(&jni, obj, 1)` and `Game_nativeSetNavigationHidden(&jni, obj, 0)` before `Game_nativeInit` sets `g_bIsUseController = 1` and `Xperia::m_bIsPadOpened = 1`. The engine automatically loads `menuXperia/*.swf` with native PlayStation-style button icons.

### 5.2 Keycode Mapping (`Xperia::Update(int)` at `0x261174`)
Disassembly of `Xperia::Update(int)` revealed the exact Android keycodes expected via `Game_nativeOnKeyDown` / `Game_nativeOnKeyUp`:
- Cross (`×`) -> `23` (`KEYCODE_DPAD_CENTER`: Confirm / Jump & Web-Swing)
- Circle (`○`) -> `1004` (`0x3ec`: Back/Cancel in menus / Dodge in gameplay)
- Square (`□`) -> `99` (`KEYCODE_BUTTON_X`: Attack)
- Triangle (`△`) -> `100` (`KEYCODE_BUTTON_Y`: Web-Shoot / Context action)
- D-Pad Up/Down/Left/Right -> `19`, `20`, `21`, `22`
- L1 / R1 -> `102`, `103`
- Start / Select -> `108`, `109`

### 5.3 Dual Analog Sticks via `Game_nativeOnTouch` (`XperiaTouchpad`)
For the audited ELF, `Game_nativeOnTouch(env, obj, action, x, y, ptrId)` is at
`0x001b9c8c`, and `GameGLSurfaceView_nativeOnTouch` is at `0x001b9f68`.
The formerly listed `0x25fb88` / `0x260920` are incorrect for this binary.
`XperiaTouchpad` is at `0x001bb0d8`; its left and right regions differ, so do
not copy a generic center coordinate into both paths.

The baseline uses left center `(180,180)`, pointer 0, and right center
`(780,180)`, pointer 1. The right path queues UP/DOWN/MOVE each active frame.
Front touch scales `1920×1088` to `960×544` and uses slots starting at 0.
These describe baseline code, not validated correct controls.

---

## 6. PS Vita FTP Pacing for Many Small Files

When uploading a game data folder with >80 small files (e.g., `.xml`, `.swf`, `.bin`) over FTP in `PASV` mode, transferring >80 files in <15 seconds exhausts the PS Vita's `SceNet` `TIME_WAIT` socket pool and hangs the FTP server thread.
- **Rule:** Pace FTP uploads so each file takes at least `0.7s` (`if dt < 0.7: time.sleep(0.7 - dt)`), and make the upload script resumable by checking existing remote file sizes via `LIST` before uploading.

---

## 7. Graphics and Input Audit (2026-11-04)

### Baseline versus hypotheses

Workspace `_logs/tasm_066_baseline.log` accompanies a reproduced corrupt suit
with colored stippling and poor city textures in the initial rooftop tutorial,
character stationary at “Press X to Jump”. This is an open defect. A live
render loop does not validate image quality, and menu FPS must not be called
gameplay FPS. An idle tutorial also does not measure traversal or streaming.

The baseline did not contain the 45 ms minimum key hold or touchscreen `+2`
pointer isolation claimed by another local skill copy. It also did not contain
a cubemap min-filter clamp: `source/dynlib.c` imports `glTexParameteri/f`
directly. Treat those fixes as proposals until implemented and tested. The
actual suit shader patch preserves native `diff.a` and `specular.a`; do not
replace it with an indiscriminate opaque-alpha injection.

The vitaGL cubemap importer loads level zero only. A mipmapped filter can be
incompatible with the available chain; verify actual filters and isolate any
clamp experiment before assigning it as the cause of the current image.

### Client-array lifetime and reproducible builds

Never authorize zero-copy client vertices or indices using `count > 64`, a
size threshold, or heap mapping alone. GPU addressability does not prove that
the CPU will retain the bytes unchanged through GPU completion. Dynamic
skinning, particles, scratch buffers and streaming can violate that assumption.
Copy client arrays into storage retained through GPU completion; normal VBO
tracking may preserve its optimized path.

The first experiment being prepared removes the count-based bypasses from
`custom_shaders.c` and `draw.c`, uses `DRAW_SPEEDHACK=0`, and leaves
`TEXTURES_SPEEDHACK=1` unchanged to isolate geometry. Texture copy-on-write
is disabled by that remaining flag and must be tested independently. No
hardware result for these changes is yet recorded here.

The vitaGL Makefile does not track changes in `CFLAGS`. The build now generates
a flags dependency and uses make `-B` when the library is stale. Record the
installed build, flags and log before assigning before/after results.

### Camera reverse engineering

For the identified ELF, `FullScreenRotateJoystick::touchBegan`, `touchMoved`,
and `touchEnded` start at `0x00344b5c`, `0x00344bd4`, and `0x00344d30`.
Static disassembly of `touchEnded` shows active-state and pointer-ID checks
before clearing rotation fields. It does not establish that injected events
reach this path or that neutral sticks stop the camera.

REA evidence: `ev_9b3029b2f3a140dbf0a9c1455c5570b7b7c839e03a89f972cf39858fa7e0f052`,
saved in workspace `_logs/rea-camera-touch-ended.json`. IDA fetch and the
selected Windows Ghidra provider failed in this session. REA CLI with
`REA_OBJDUMP_COMMAND=C:/vitasdk/bin/arm-vita-eabi-objdump.exe` returned complete
symbol-scoped disassembly with matching before/after digest. These failures
do not establish universal limitations of the providers.

Pending checks include visual A/B, camera deflect/neutral/reverse, physical
buttons and multitouch, pause/resume, traversal and LOD, cold/cache loading,
save/reopen, multiple boots and a prolonged moving-gameplay session. Consult
`docs/DEBUG_SESSION_2026-11-04.md` in the source repository for the finding ledger.
