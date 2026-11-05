# TASM inherited engineering notes — historical archive

Preserved in full from local records. Some identities, offsets, FPS/stability claims and recommendations are superseded. Read references 14 and 15 before applying any advice. These are inherited records, not new hardware validation.


---

## Original file: PORTING_PLAN.md

# Porting Plan & Technical Architecture — The Amazing Spider-Man (PS Vita)

> Living engineering document for `tasm-vita` (`TASMVITA1`).

## 1. Target Binary & Engine

- **APK:** `The-Amazing-Spider-Man-HD.apk` (v1.2.3)
- **Package Name:** `com.gameloft.android.ANMP.GloftAMHM`
- **Shared Library:** `lib/armeabi-v7a/libSpiderMan.so` (11,111,836 bytes, ELF32 ARMv7-A softfp, non-stripped with full C++ symbols).
- **Graphics API:** OpenGL ES 2.0 (`effects_nexus_s.gla` GLSL shader pack -> `vitaGL` + `SceShaccCg` with shader cache in `ux0:data/tasm/shader_cache`).
- **Audio Engine:** `vox::DriverAndroid` (`android.media.AudioTrack` JNI -> `sceAudioOut` with 44100->48000 Hz polyphase FIR resampler).
- **UI Middleware:** Scaleform GFx (`menu/` and `menuXperia/` SWF movies + `swf.pak`).

## 2. Dynamic Imports & Symbol Resolution (`source/dynlib.c`)

`libSpiderMan.so` imports 179 dynamic symbols (`UND` in `.dynsym`). Aside from the standard `so_util` table, 11 additional symbols were required and added to `source/dynlib.c`:

- `__assert2` -> `assert2` (`source/reimpl/sys.c`, logs warning without aborting)
- `__dso_handle` -> `&__dso_handle_soloader`
- `openlog`, `syslog`, `closelog`, `perror` -> logging/no-op stubs
- `strptime` -> date parser stub
- `glCompressedTexImage3DOES`, `glCompressedTexSubImage3DOES`, `glTexImage3DOES`, `glTexSubImage3DOES` -> GLES 3D texture extension stubs
- **Important:** Do **not** override `_Znwj`, `_Znaj`, `_ZdlPv`, or `_ZdaPv` in `source/dynlib.c`; `libSpiderMan.so` defines its own internal C++ allocator and overriding those symbols corrupts `std::map<IShaderCode>` red-black tree nodes.

## 3. Bionic Pthreads, Worker Stack Size, & Condvars (`source/reimpl/pthr.c`)

1. **4-Byte Bionic Mutexes (`pthread_mutex_t_bionic`):**
   - `CriSec::__lock` (`0x4ca94c`) passes a 4-byte stack `int` to `pthread_mutexattr_init` and `pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)`.
   - `pthread_mutex_init_soloader` is lazy (stores `NULL` in `mutex->real_ptr`), and on first lock allocates a `pthr_mutex_obj_t` on the heap (capped at `PTHR_MAX_DEDICATED_LWMUTEX = 200` kernel recursive `SceKernelLwMutexWork` objects so `SceShaccCg` always has free kernel LwMutex slots).
2. **1 MiB Minimum Thread Stack & Clean Exit (`pthread_create_soloader`):**
   - Gameloft's `glf::Thread` worker (`0x9863f4c8`) requests a 64 KiB stack on Android (`attr->stack_size = 65536`) and compiles shaders 56..117 (`program=28..63`) via `SceShaccCg`, which requires >128 KiB of free stack (`sceKernelCheckThreadStack`).
   - `pthread_create_soloader` enforces a 1 MiB minimum stack size (`1024 * 1024`), and `soloader_thread_entry` calls `sceKernelExitDeleteThread(0)` on return so exited worker/telemetry threads immediately free their 1 MiB kernel stack.
3. **True Blocking `pthread_cond_wait_soloader` (`glf::Task`):**
   - `pthread_cond_wait_soloader` waits with `NULL` timeout (never returning early on a 100 ms timeout) and validates `pthr_cond_t` heap pointers (`0x81000000..0x98000000` with `0xC0DEC0DEu` magic) so `glf::Task::Done()` (`0x9865776c`) never spins waiting for a prematurely exited worker thread.

## 4. Boot Sequence, DRM Stub, & Device Profile (`source/main.c`, `source/loader.c`, `source/java_shim.c`)

1. **DRM Pointer Chain Bypass (`source/loader.c`):**
   - Writes `&s_drm_stub[0]` to `tasm_mod.text_base + 0x00BD9564` so `sub_9889fb58` in `Game_nativeInit` reads `*(base + 0x00BD9564) -> +0x24 -> +0x0C == 1`.
2. **Linux Kernel Atomic Helpers (`source/patch.c`):**
   - Patches callers of `__kuser_cmpxchg` (`0xffff0fc0`) and `__kuser_memory_barrier` (`0xffff0fa0`) to native ARMv7 `ldrex`/`strex`/`dmb sy` routines.
3. **`JNI_OnLoad(&jvm, NULL)` & Xperia Play Mode:**
   - Calls `Game_nativeSetXperia(1)` and `Game_nativeSetNavigationHidden(0)` to enable native Xperia Play gamepad support (`menuXperia/*.xxx`).
   - Passes `"Sony"` / `"Nexus S"` to `Game_nativeSetPhoneManufacturerModel`, selecting `config_android_nexus_s.xml` and `effects_nexus_s.gla`.
4. **7-Coefficient `vec4(rgb, *_alpha)` Shader Fix (`source/utils/glutil.c`):**
   - Rewrites `texture2D(*_alpha, *)` to `texture2D(*_alpha, *).r` when followed by `)` in `glShaderSource_soloader` so particle shader 33 compiles cleanly in `SceShaccCg`.

## 5. Memory Pool & Archive I/O Architecture (`lib/vitaGL/source/vgl.c`, `source/reimpl/io.c`)

1. **`vglMalloc` Pool Ordering (`lib/vitaGL/source/vgl.c`):**
   - `vglMalloc`, `vglMemalign`, and `vglCalloc` allocate from `VGL_MEM_RAM` (~138 MiB) -> `VGL_MEM_PHYCONT` (~26 MiB) -> `VGL_MEM_EXTERNAL` (150 MiB newlib heap).
   - Preserves >75 MiB of free newlib heap during open-world Manhattan streaming so libc `fopen("ux0:data/tasm/tempSaveFile.dat", "wb")` and `SceShaccCg` C++ allocations never run out of newlib heap.
2. **Fast-Path `.gla` / `.pak` Loose-File Probing & Multi-Archive `shf_open` (`source/reimpl/io.c`):**
   - `io_expected_miss()` short-circuits loose-file override checks (`.vsh`, `.fsh`, `GLSL/file.map`, `.bdae`) before touching the SD card.
   - `shf_open` caches up to 16 `.xxx` / `.pak` archives with LRU file-descriptor eviction (`SHF_MAX_OPEN_FDS = 6`) and 128 virtual handles, reducing menu and level load times from ~140s to ~1–5s.

## 6. Controls Architecture (`source/main.c`)

- **Physical Buttons (`Game_nativeOnKeyDown` / `Game_nativeOnKeyUp`):**
  Disassembly of `Xperia::Update(int)` (`0x261174`) revealed the exact Android keycodes expected by the engine:
  - Cross (`SCE_CTRL_CROSS`) -> `23` (`KEYCODE_DPAD_CENTER`: Confirm / Jump & Web-Swing)
  - Circle (`SCE_CTRL_CIRCLE`) -> `1004` (`0x3ec`, Xperia Play Back/O: Cancel / Close Pause Menu / Dodge)
  - Square (`SCE_CTRL_SQUARE`) -> `99` (`KEYCODE_BUTTON_X`: Melee Attack)
  - Triangle (`SCE_CTRL_TRIANGLE`) -> `100` (`KEYCODE_BUTTON_Y`: Web-Shoot / Context Action)
  - D-Pad -> `19` (Up), `20` (Down), `21` (Left), `22` (Right)
  - L1 / R1 -> `102` / `103` (Camera / Web-Rush / Target Lock)
  - Start / Select -> `108` (Pause Menu) / `109` (City Map & Upgrades)
- **Dual Analog Sticks (`Game_nativeOnTouch`):**
  `Game_nativeOnTouch` (`0x25fb88`) feeds `XperiaTouchpad`:
  - Left Analog Stick -> Virtual touchpad 0 (`center = (180, 180)`, `radius = 100`, `ptrId = 0`).
  - Right Analog Stick -> Virtual touchpad 1 (`center = (480, 180)`, `radius = 100`, `ptrId = 1`).
- **Front Touchscreen (`GameGLSurfaceView_nativeOnTouch`):**
  Scales Vita front touch (`1920×1088` -> `960×544`) and forwards multi-touch events (`ACTION_DOWN=0`, `ACTION_UP=1`, `ACTION_MOVE=2`).


---

## Original file: PORT_GUIDE.md

# Technical Porting Guide — The Amazing Spider-Man HD (`TASMVITA1`)

This document records the reverse-engineering findings, architecture, and hardware-verified fixes implemented for the PS Vita port of **The Amazing Spider-Man HD** (`com.gameloft.android.ANMP.GloftAMHM`, v1.2.3, `libSpiderMan.so`).

---

## 1. Binary & Engine Overview

- **Target binary:** `lib/armeabi-v7a/libSpiderMan.so` (11,111,836 bytes, ELF32 ARMv7-A, `softfp`, unstripped C++ symbols).
- **Load base:** `0x98000000` (`LOAD_ADDRESS` in `source/loader.c`).
- **Subsystems:**
  - **Graphics:** OpenGL ES 2.0 (`effects.gla` GLSL shader pack) → `vitaGL` + `libshacccg.suprx` (`vitaShaRK`) with persistent binary shader caching (`ux0:data/tasm/shader_cache`).
  - **UI:** Scaleform GFx (`menu/` and `menuXperia/` SWF files + `swf.pak`).
  - **Audio:** `vox::DriverAndroid` (`android.media.AudioTrack` via FalsoJNI) → `sceAudioOut` with a 44.1→48 kHz polyphase FIR resampler (`source/java_shim.c`).
  - **Input:** `Xperia::Update(int)` (`0x00261174`) + `XperiaTouchpad` (`Game_nativeOnTouch` at `0x0025fb88`) + front touchscreen (`GameGLSurfaceView_nativeOnTouch` at `0x00260920`).

---

## 2. Key Technical Fixes

### 2.1 Spider-Man Suit Translucent / "Chrome Mirror" Fix (`source/utils/glutil.c`)

- **Symptom:** Spider-Man's 3D model rendered like a translucent mirror, showing background buildings through his suit.
- **Root Cause:**
  1. The character skinning shaders (`skn_spidey.vsh` / `skn_spidey.fsh`) sample `uniform samplerCube LightMap` weighted by a Fresnel term and write `gl_FragColor.rgb`, leaving `gl_FragColor.a` unwritten or holding the Fresnel mask while `GL_BLEND` (`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`) remains active.
  2. `LightMap` is configured with `GL_LINEAR_MIPMAP_NEAREST`, and sampling incomplete cubemap mip levels on `sceGxm` produces bright uninitialized reflection values.
- **Fix:**
  1. In `glTexParameteri_hook`, clamp `GL_NEAREST_MIPMAP_*` and `GL_LINEAR_MIPMAP_*` on `GL_TEXTURE_CUBE_MAP` to `GL_LINEAR`.
  2. In `glShaderSource_hook`, inject `gl_FragColor.a = 1.0;` before the final `}` of any fragment shader referencing `LightMap`.

### 2.2 Physical Gamepad, Dual Analog Sticks & Touchscreen Coexistence (`source/main.c`)

- **Xperia Play Mode:** Calling `Game_nativeSetXperia(1)` and `Game_nativeSetNavigationHidden(0)` activates `g_bIsUseController = 1` and `Xperia::m_bIsPadOpened = 1`, loading `menuXperia/*.swf` with native PlayStation button prompts.
- **Minimum Hold Debounce (`45 ms`):** Button taps shorter than 1 frame are dropped during scene transitions; `tasm_poll_input()` holds `Game_nativeOnKeyUp` until at least `45,000 us` (`45 ms`) have elapsed since `Game_nativeOnKeyDown`.
- **Dual Virtual Touchpad (`Game_nativeOnTouch`):**
  - Left Analog Stick → `ptrId = 0`, `center = (186, 180)`, `radius = 100` (`x = 64..308`, `y = 360 - raw_y`).
  - Right Analog Stick → `ptrId = 1`, `center = (710, 180)`, `radius = 100` (`x = 588..832`, `y = 360 - raw_y`).
  - Sends `ACTION_DOWN (1)` on initial deflection, `ACTION_MOVE (2)` while deflected, and a single `ACTION_UP (0)` on return to center.
- **Left-Stick D-Pad Equivalence (`g_stick_dpad_mask`):** Deflecting the left analog stick past `±0.45` also drives `KEYCODE_DPAD_UP/DOWN/LEFT/RIGHT` (`19..22`), allowing smooth left-stick navigation in all Scaleform GFx menus.
- **Front Touchscreen Pointer Isolation:** Touchscreen pointer IDs passed to `GameGLSurfaceView_nativeOnTouch` are offset by `+2` (`slot + 2`) so they never collide with `XperiaTouchpad` `ptrId = 0` or `ptrId = 1`.

### 2.3 Bionic 4-Byte `pthread_mutex_t` & `pthread_mutexattr_t` ABI (`source/reimpl/pthr.c`)

- Gameloft's `CriSec::__lock` (`0x004ca94c`) allocates a 4-byte `pthread_mutexattr_t` on the stack and embeds a 4-byte `pthread_mutex_t` in C++ objects with `PTHREAD_MUTEX_RECURSIVE`.
- Forwarding `pthread_mutexattr_init` to Newlib's larger struct corrupts caller stack frames. `source/reimpl/pthr.c` handles `pthread_mutexattr_*` and `pthread_condattr_*` on 4-byte `int *` slots and backs all mutexes with recursive `SceKernelLwMutexWork` instances.

### 2.4 Memory Budget & `libshacccg` Heap Headroom (`source/utils/glutil.c`)

- `libSpiderMan.so` uses ~18.3 MB of static image space plus ~30 MB of `newlib` heap (`CLODStreaming` 25 MB pool).
- `libshacccg.suprx` (`vitaShaRK`) allocates temporary compilation buffers from `newlib` heap; starving `newlib` causes `shark_compile_shader_extended` to fail and fall back to a white dummy shader.
- `vglInitExtended(0, 960, 544, 12 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE)` + `vglSetCircularPoolSize(8 * 1024 * 1024)` leaves ample `newlib` heap while keeping ~51 MB VRAM and ~15 MB PHYCONT free in open-world Manhattan.

### 2.5 Open-World Performance: `DeviceConfiguration` & Zero-Copy 3D Mesh Draws

1. **Runtime `vita_config.xml` (`source/reimpl/io.c`):**
   - `Nexus S` device model selects `config_android_lowphone.xml` with `isI9300 = 0` (loading `swf.pak` and `effects.gla`).
   - `remap_gameloft_sdcard_path` redirects `config_android_lowphone.xml` to an auto-generated `ux0:data/tasm/logs/vita_config.xml` that keeps `Use2XMap=1`, `LowRes=0`, and `Streaming_BufferSize=25` (required by the 2X HD asset pack) while tuning the rest for Vita.
   - The **v1.1 visual-correctness profile** keeps `MaxTextureSize=1024` with one skipped mip level (`Atlas/LowLod/Building/Prop/Other=1`, `HighLod/Lightmap/Vehicle` full or half), restores `RemoveAlpha*=0`/`Use5652DTexture=0`, and enables fog, full-quality lighter/sky shaders, lens flare, bloom/radial blur/heat/color shift, dust, smoke, birds and speed lines. The old `MaxTextureSize=512` + two-level skip sampled 128 px facades, producing the reported flat/untextured buildings and boiling textures. The old RGB565 2D path discarded alpha (black HUD plates, missing particles/shadows).
   - **Object/LOD budget** (`BuildingDistHigh/Mid/Low=4000/18000/70000`, `GameObjectRangeScale=15`, `SkipCarLod0=1`, `NPCPoolNum=6`, `CarPoolNum=10`, `MaxVisibleNPC/Car=2`, `RemoveStaticNpc=1`): the engine is CPU-bound on the single-core ARM and scales with visible objects, not draw count. This keeps gameplay at **~58 fps** and the pause/menu at ~40–50 fps.
   - **Runtime override:** `ux0:data/tasm/vita_config_override.xml` (same XML) is copied verbatim over the built-in profile, so users can trade quality for speed without a rebuild.
2. **Zero-Copy 3D Mesh Draws in `vitaGL` (`lib/vitaGL/source/custom_shaders.c` & `lib/vitaGL/source/draw.c`):**
   - Streamed city batch meshes (`CLODStreaming<CStreamedBatchMesh>`) reside in `newlib` heap memory, which `vgl_mem_init` ALREADY maps into `sceGxm` via `sceGxmMapMemory` (`SCE_GXM_MEMORY_ATTRIB_RW`), and are drawn with `target_vbo == NULL` and `gpu_buf == NULL`.
   - Under `DRAW_SPEEDHACK=2 INDICES_SPEEDHACK=1 TEXTURES_SPEEDHACK=1`, for 3D meshes (`count > 64` indices, whereas transient 2D UI quads have `count == 6`):
     - `_glDrawElements_CustomShadersIMPL` skips the CPU `for (int i = 0; i < count; i++)` `top_idx` scan and passes `ptrs[0] = (void *)base_off` directly to `sceGxmSetVertexStream` without `gpu_alloc_mapped_temp` or `vgl_fast_memcpy`.
     - `setup_elements_indices` in `draw.c` passes `ptr = src` directly to `sceGxmDraw` without copying the index buffer into the circular pool.
   - This reduced CPU time inside `glDrawElements` by **61%** (from `1,192–1,898 ms` per 5s down to `735 ms` per 5s, `63 us/draw`) and eliminated circular pool pressure.

### 2.6 Shader-Manager Map Corruption & Boot Stability (v1.1)

- **Symptom:** intermittent `Data abort` during boot/level load, always in `strcmp` called from
  `std::map<...IShader...>::insert` (`LR = libSpiderMan+0x4c6984`) with `DFAR = 0x00000001`.
  Roughly half of cold boots died at program 4–20; the rest reached gameplay.
- **Root cause:** the engine's shader-manager red-black tree can contain a sentinel `SName`
  whose internal `char *` is `1` (and it mutates the same map from main and worker threads).
  `strcmp(1, valid)` data-aborts before the engine can recover.
- **Fixes:**
  1. `dynlib.c` resolves the imported `strcmp` to `tasm_strcmp_safe` (`reimpl/mem.c`), which
     treats pointers below `0x1000` as `""`. Valid strings compare byte-for-byte identically.
  2. `main.c` raises the main thread to priority `0x40`; `pthr.c` creates all engine workers at
     `0xB0` so they cannot preempt a map insert mid-rotation.
  3. `pthr.c` mutex objects now carry a `0xC0DEC0DE` magic and NULL mutex handles fall back to
     one shared recursive lock instead of silently returning `EINVAL`.
- **Result:** 5/5 consecutive cold boots and full menu/level loads without the `strcmp` abort.
- **Do not enable `vita_stream.suprx` for `TASMVITA1`:** its display hook reintroduces the
  crash during boot/loading (the stream docs already warn it is unsafe for retail games).



---

## Original file: docs/CASE_STUDY_TASM.md

# Case Study: The Amazing Spider-Man HD (Gameloft Engine) — PS Vita Port

## 1. Overview

- **Game:** The Amazing Spider-Man HD (v1.2.3, `com.gameloft.android.ANMP.GloftAMHM`)
- **Binary:** `libSpiderMan.so` (11,111,836 bytes, ARMv7-A softfp, non-stripped C++ symbols)
- **Title ID:** `TASMVITA1`
- **Data Path:** `ux0:data/tasm/`
- **Display:** 960×544 native fullscreen (`vitaGL` + `SceShaccCg` shader cache at `ux0:data/tasm/shader_cache`)
- **Audio:** `vox::DriverAndroid` (`android.media.AudioTrack` JNI → `sceAudioOut` with 44.1→48 kHz polyphase Kaiser FIR resampler)
- **Controls:** Native Xperia Play controller mode (`menuXperia/*.xxx`) + dual virtual touchpad analog sticks + front multi-touch panel

---

## 2. Complete Hardware Findings & Engineering Solutions

### 2.1 DRM Pointer Chain in `Game_nativeInit` (`0x981b5f88`)
- **Symptom:** Immediate Data Abort in `Game_nativeInit` at `0x981b5f88` (`ldr r3, [r3]` where `r3 == 0`).
- **Root Cause:** `sub_9889fb58` (called early in `Game_nativeInit`) reads a 3-level pointer chain `*(base + 0x00BD9564) -> +0x24 -> +0x0C` that is normally populated by Gameloft's Java DRM/licensing handshake.
- **Fix (`source/loader.c`):** Allocate a static `s_drm_stub[32]` structure where `s_drm_stub[9]` (`+0x24`) points to `&s_drm_stub[16]` and `s_drm_stub[19]` (`+0x0C`) is `1`, and write `&s_drm_stub[0]` to `tasm_mod.text_base + 0x00BD9564` right after `so_initialize()`.

### 2.2 Linux Kernel User-Helper Page (`0xffff0fc0` / `0xffff0fa0`)
- **Symptom:** Prefetch Abort at `PC = 0xffff0fc0` called from `libSpiderMan.so + 0x875bd0`.
- **Root Cause:** Android NDK ARMv7 code jumps directly to the Linux kernel user-space atomic helpers `__kuser_cmpxchg` (`0xffff0fc0`) and `__kuser_memory_barrier` (`0xffff0fa0`), which are unmapped on PS Vita.
- **Fix (`source/patch.c`):** Patch callers of `0xffff0fc0` (`0x00875BAC`, `0x0087FC68`) and `0xffff0fa0` (`0x0087FC94`) in `libSpiderMan.so` to jump to native ARMv7 `ldrex`/`strex`/`dmb sy` implementations (`kuser_cmpxchg` and `kuser_memory_barrier`).

### 2.3 Bionic 4-Byte `pthread_mutex_t`, `pthread_mutexattr_t`, and Kernel LwMutex Limits (`source/reimpl/pthr.c`)
- **Symptom 1:** Stack corruption in `CriSec::__lock` (`0x4ca94c`), which passes a 4-byte stack `int` to `pthread_mutexattr_init` and `pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)`.
- **Symptom 2:** Heap corruption (`0xDEADBEF7` / `0xDEADBEF3`) when writing a 32-byte `SceKernelLwMutexWork` directly into a 4-byte Bionic `pthread_mutex_t_bionic` field embedded inside C++ structs (`glf::Task`, `CriSec`).
- **Symptom 3:** `SceShaccCg` failing with `E] fatal internal error on line -1` because the game called `pthread_mutex_init` >400 times during boot, exhausting the PS Vita's per-process limit of 256 kernel LwMutexes so `SceShaccCg` could not create `"SceShaccMutex"`.
- **Fix (`source/reimpl/pthr.c`):**
  1. Handle `pthread_mutexattr_*_soloader` and `pthread_condattr_*_soloader` on 4-byte `int *`.
  2. Make `pthread_mutex_init_soloader` **lazy** (only stores `NULL` in the 4-byte `mutex->real_ptr`), and on first lock allocate a `pthr_mutex_obj_t` on the heap and store its pointer in `mutex->real_ptr`.
  3. Cap dedicated kernel LwMutexes at `PTHR_MAX_DEDICATED_LWMUTEX = 200` (with recursive user-space atomic fallback beyond 200) and free them in `pthread_mutex_destroy_soloader`, leaving >50 kernel LwMutexes free for `SceShaccCg`.

### 2.4 C++ `operator new` / `delete` Symbol Collision Between `libSpiderMan.so` and `SceShaccCg` (`source/dynlib.c`)
- **Symptom:** Data Abort in `strcmp` (`0x8109f9ca`) called from `libSpiderMan.so + 0x004c7e3c` (`std::map<IShaderCode>::insert`) right after `glCompileShader(1)` returned.
- **Root Cause:** `libSpiderMan.so` defines and exports its own custom C++ allocator (`_Znwj`, `_Znaj`, `_ZdlPv`, `_ZdaPv`). Overriding those symbols in `default_dynlib` redirected `libSpiderMan.so`'s internal C++ allocations while internal calls inside the `.so` still used its own allocator metadata, corrupting the `std::map` red-black tree nodes.
- **Fix (`source/dynlib.c`):** Remove `_Znwj`, `_Znaj`, `_ZdlPv`, and `_ZdaPv` from `default_dynlib` so `libSpiderMan.so` uses its own self-contained C++ allocator.

### 2.5 `SceShaccCg` Lifecycle & Worker Thread Stack Size (`lib/vitaGL/source/custom_shaders.c`, `source/reimpl/pthr.c`)
- **Symptom 1:** Calling `sceShaccCgReleaseCompiler()` + `shark_end()` after every shader compilation crashed inside `SceShaccCg` on shader 35 (`PC = 0x82f4f0e0`, `DFAR = 0xdeadbef7`) due to a dangling pointer bug in Sony's `libshacccg.suprx` teardown.
- **Symptom 2:** During `Android_Loading.xxx`, Gameloft's async loader worker thread (`0x9863f4c8`, created with a 64 KiB stack via `pthread_attr_setstacksize`) compiled shaders 56..117 (`program=28..63`). `SceShaccCg` checks remaining thread stack via `sceKernelCheckThreadStack()` and aborts with `E] fatal internal error on line -1` when free stack is under ~128 KiB.
- **Fix:**
  1. Keep `vitaShaRK` initialized for the lifetime of the process in `compile_shader` (matching upstream `vitaGL`).
  2. Enforce a **1 MiB minimum stack size** in `pthread_create_soloader` (`source/reimpl/pthr.c`) and call `sceKernelExitDeleteThread(0)` in `soloader_thread_entry` so exited threads immediately free their 1 MiB kernel stack.

### 2.6 GLSL 7-Coefficient `vec4(rgb, *_alpha)` Patch (`source/utils/glutil.c`)
- **Symptom:** Fragment shader 33 failed to compile with `E] 'float4' requires exactly 4 coefficients. Found 7 on line 117`.
- **Root Cause:** Gameloft's Glitch-engine particle shader wrote `vec4(texture2D(MapDiffuseSampler, vCoord0).rgb, texture2D(MapDiffuseSampler_alpha, vCoord0))`, passing `vec3 + vec4` (7 floats) to `vec4(...)`.
- **Fix (`source/utils/glutil.c`):** In `glShaderSource_soloader`, scan for `_alpha,` followed by `))` and rewrite `texture2D(*_alpha, *)` to `texture2D(*_alpha, *).r`.

### 2.7 `glf::Task::Done()` Spin After `program=64` Link (`source/reimpl/pthr.c`)
- **Symptom:** When starting a New Game (`level_01`), the main thread linked `program=64` and then spun forever in `glf::Task::Done()` (`0x9865776c`).
- **Root Cause:**
  1. `pthread_cond_wait_soloader` had a 100 ms timeout (`100000` µs) and returned `0` on timeout, causing `glf::Thread` worker threads waiting for tasks to wake up spuriously every 100 ms on an empty queue and exit!
  2. `pthr_cond_get` checked `raw >= 0x40000000u && raw < 0x50000000u`, which accidentally matched heap pointers in `0x40000000..0x4FFFFFFF` inside uninitialized `glf::Task` objects and treated them as valid kernel `SceUID`s (`sceKernelSignalSema` failed with `0x80028003`).
- **Fix (`source/reimpl/pthr.c`):** Remove the timeout from `pthread_cond_wait_soloader` (wait until signaled via `NULL` timeout) and validate `pthr_cond_t` heap pointers in `0x81000000..0x98000000` with a `0xC0DEC0DEu` magic header.

### 2.8 Fast-Path for `.gla` / `.pak` Loose-File Overrides (`source/reimpl/io.c`)
- **Symptom:** Loading `Android_mainmenu.xxx` or `Android_GamePlay.xxx` took ~140 seconds because the engine probed `ux0:data/tasm/<shader>.vsh`, `<shader>.fsh`, `file.map`, and `*.bdae` via `fopen` + `stat` thousands of times before reading from `effects_nexus_s.gla` or `.pak` archives, and `shf_open` only cached a single archive handle (`Android_sound.xxx` + `Android_mainmenu.xxx` thrashed the cache 12,000 times).
- **Fix (`source/reimpl/io.c`):**
  1. Short-circuit `io_expected_miss(vpath)` (`.vsh`, `.fsh`, `GLSL/file.map`, `.bdae`) in `fopen_soloader` and `stat_soloader` **before** touching the SD card (`return NULL` / `-1` with `errno = ENOENT`).
  2. Expand `shf_open` to a 16-archive table with LRU file-descriptor eviction (`SHF_MAX_OPEN_FDS = 6`) and 128 virtual handles.
  - **Result:** `Android_mainmenu.xxx` and `Android_GamePlay.xxx` load in **~1–3 seconds** instead of 140 seconds!

### 2.9 `vglMalloc` Pool Ordering & Newlib Heap Preservation (`lib/vitaGL/source/vgl.c`)
- **Symptom:** After entering `level_01` and opening the in-game pause menu (`Android_igm.xxx`), saving `ux0:data/tasm/tempSaveFile.dat` crashed in `_fwrite_r` (`0x8109a93a`, `DFAR = 0x0000000c`) because `fopen("ux0:data/tasm/tempSaveFile.dat", "wb")` returned `NULL` (`0x0`).
- **Root Cause:** `vglMalloc`, `vglMemalign`, and `vglCalloc` in `lib/vitaGL/source/vgl.c` allocated from `VGL_MEM_EXTERNAL` (the 150 MiB newlib heap) **first** before using `VGL_MEM_RAM` (~138 MiB) and `VGL_MEM_PHYCONT` (~26 MiB). Once `level_01` filled all 150 MiB of `VGL_MEM_EXTERNAL`, `vglMalloc` spilled over to `VGL_MEM_RAM` (which still had 80 MiB free), leaving **0 bytes** in the newlib heap for newlib's internal `_fopen_r -> _malloc_r`!
- **Fix (`lib/vitaGL/source/vgl.c`, `source/reimpl/io.c`):**
  1. Reorder `vglMalloc`, `vglMemalign`, and `vglCalloc` to allocate from `VGL_MEM_RAM` → `VGL_MEM_PHYCONT` → `VGL_MEM_EXTERNAL`. This keeps >75 MiB of newlib heap free even during full open-world Manhattan streaming, so `fopen("ux0:data/tasm/tempSaveFile.dat", "wb")` succeeds every time.
  2. Add NULL-pointer guards (`if (!f) return ...;`) to all `f*_soloader` stdio wrappers in `source/reimpl/io.c`.

### 2.10 Graphics-Quality Regression & Fix (v1.1, `source/reimpl/io.c`)
- **Symptom:** the port was functional but "full of graphical glitches, everything loading badly": large building facades rendered as flat untextured boxes, textures boiled/moiré'd in motion, HUD plates/particles/shadows were black or missing, and the frame rate collapsed to 18–30 fps in dense areas.
- **Root cause:** the generated runtime profile had been tuned too aggressively for memory:
  `MaxTextureSize=512` (downscales 1024/2048 assets), `*MipmapSkip=2` (the engine starts large textures at mip level 2, i.e. 128 px facades), `RemoveAlphaEffect=1`, `RemoveAlphaObj=1`, `Use5652DTexture=1` (RGB565 has no alpha), and object/LOD ranges cut to a fraction of the Android low-phone baseline.
- **Fix:** the v1.1 profile (`source/reimpl/io.c`) restores `MaxTextureSize=1024`, one-step mip skipping for facades/props/atlases (full for hero/vehicle/lightmap), alpha blending and RGBA 2D textures, fog, full-quality lighter/sky shaders, lens flare and the original post-processing/atmosphere set. Object pools and LOD distances are tuned for the single-core ARM (`BuildingDistHigh/Mid/Low=4000/18000/70000`, `GameObjectRangeScale=15`, `NPCPoolNum=6`, `CarPoolNum=10`), which yields ~58 fps in gameplay.
- **Runtime override:** `ux0:data/tasm/vita_config_override.xml` replaces the built-in profile verbatim for on-device A/B testing.

### 2.11 Shader-Manager `strcmp` Data Abort & Boot Stability (v1.1)
- **Symptom:** intermittent `Data abort` on roughly half of cold boots at program 4–20, always in `strcmp` from `std::map<...IShader...>::insert` (`LR = libSpiderMan+0x4c6984`, `DFAR = 0x00000001`).
- **Root cause:** the engine's shader-manager map can hold a sentinel `SName` whose internal `char *` is `1`; its worker threads also mutate the map concurrently.
- **Fix:**
  1. `source/reimpl/mem.c` provides `tasm_strcmp_safe`, wired to the engine's imported `strcmp` in `source/dynlib.c`; pointers below `0x1000` are treated as `""`.
  2. `source/main.c` raises the main thread to priority `0x40` and `source/reimpl/pthr.c` creates engine workers at `0xB0`, removing preemption during map rotations.
  3. `pthr.c` mutexes are magic-tagged (`0xC0DEC0DE`) and NULL mutex pointers fall back to one shared recursive lock instead of silently returning `EINVAL`.
- **Result:** 5/5 cold boots and full level loads passed on hardware.
- **Warning:** keep `vita_stream.suprx` disabled for `TASMVITA1`; its display hook reintroduces boot/load crashes.



---

## Original file: CHANGELOG.md

# Changelog

## v1.1 — 2026-11-04 — Graphics & stability pass

- **Restored the texture pipeline** (`source/reimpl/io.c`): runtime profile now uses
  `MaxTextureSize=1024` and one-step mip skipping instead of `512` + two skipped
  levels. This is what fixed the "untextured flat buildings", blurry facades and
  boiling/moiré textures in motion. Building/prop maps are effectively 512 px
  (1 mip skip) while characters/vehicles/lightmaps stay at full resolution.
- **Restored alpha and colour depth**: `RemoveAlphaEffect=0`, `RemoveAlphaObj=0`
  and `Use5652DTexture=0`. The old RGB565 2D path discarded alpha, which is why
  HUD plates, particles and shadow/transparent objects rendered as black or
  disappeared.
- Re-enabled fog, the full-quality lighter/sky shaders, MC brightness, lens
  flare and the original low-phone post-processing set (Bloom/RadialBlur/Heat/
  ColorShift) plus dust, manhole smoke, birds and speed lines.
- Rebalanced object/LOD budgets for the single-core CPU: `BuildingDistHigh/Mid/
  Low=4000/18000/70000`, `GameObjectRangeScale=15`, `NPCPoolNum=6`,
  `CarPoolNum=10`, `MaxVisibleNPC/Car=2`. Stable ~58 fps in gameplay (was 18–30
  fps with the max-quality profile), with the menu ~50 fps.
- **Runtime tunable profile**: dropping `ux0:data/tasm/vita_config_override.xml`
  (same XML shape) replaces the built-in profile verbatim, no rebuild needed.
- **Fixed the intermittent boot/level-load data abort** (`strcmp` at
  `std::map<IShader>::insert`). The engine's shader manager leaves a sentinel
  `SName` whose internal `char *` is `1`; the imported `strcmp` now resolves to
  `tasm_strcmp_safe` (`source/reimpl/mem.c`), which treats invalid pointers as
  the empty string. Combined with raising the main thread above the engine
  workers (`0x40` vs `0xB0`), 7 consecutive cold boots passed.
- Hardened the Bionic pthread shim: magic-tagged mutex bookkeeping objects,
  a shared NULL-mutex fallback and no use-after-free on destroy.
- Fixed the `vpk` CMake target (it passed `-SourceDir/-OutVpk` to a script that
  only accepts `-BuildDir/-OutFile`, so `make vpk` failed).
- Added clock/priority and 5 s pool-usage heartbeats (`[clk]`, `[gl-mem]
  heartbeat`) for future diagnostics.
- **Warning:** `vita_stream.suprx` must stay disabled for `TASMVITA1`. Its
  display hook makes this port crash during boot/level loading (documented in
  the stream docs as unsafe for retail games). Screenshots via `pngshot`
  (PS+Start) also pause the game because the game receives Start.

## v1.0 — 2026-11-03

- First public release of the PS Vita port (`TASMVITA1`), verified end-to-end on real PS Vita hardware.
- Native 960×544 fullscreen 3D rendering via vitaGL (`UNIFORM_MAX_PROGRAMS=256`, `scenesPerFrame=8`, shader cache at `ux0:data/tasm/shader_cache`).
- Patched Linux kernel atomic user-helpers (`__kuser_cmpxchg` at `0xffff0fc0` and `__kuser_memory_barrier` at `0xffff0fa0`), `Game_nativeInit` DRM pointer chain (`+0x00BD9564`), and 7-coefficient `vec4(rgb, *_alpha)` particle shader in `effects_nexus_s.gla`.
- Implemented lazy Bionic 4-byte recursive `pthread_mutex_t` (`PTHR_MAX_DEDICATED_LWMUTEX=200`), 1 MiB minimum worker thread stacks with `sceKernelExitDeleteThread(0)` cleanup, and true-blocking `pthread_cond_wait` for `glf::Task` worker synchronization.
- Added fast-path loose-file miss filtering (`io_expected_miss`) and 16-slot LRU multi-archive `shf_open` caching (`SHF_MAX_OPEN_FDS=6`), reducing menu and open-world level load times from ~140s to ~1–5s.
- Reordered `vglMalloc` memory pools (`VGL_MEM_RAM` → `VGL_MEM_PHYCONT` → `VGL_MEM_EXTERNAL`) to preserve >75 MiB of newlib heap for stdio save-game writes (`tempSaveFile.dat`) and `SceShaccCg`.
- Audio through `android.media.AudioTrack` → `sceAudioOut` with a 44.1→48 kHz polyphase Kaiser FIR resampler.
- Complete physical PS Vita controls (Xperia Play gamepad mode + dual virtual touchpad analog sticks) and multi-touch front panel:
  - Left Analog Stick moves Spider-Man; Right Analog Stick controls the camera.
  - Cross confirms / jumps & web-swings; Circle goes back / closes pause menu / dodges; Square attacks; Triangle web-shoots.
  - L1/R1 web-rush & camera; Start pauses; Select opens the city map & upgrades.
- Loader-only VPK; game data must be provided by the user.
