# TASM: mipmaps, camera and measured frame time (2026-11-04)

Scope: `libSpiderMan.so` SHA-256
`c3de282d87fece00b797f23bb0b7cdfe778135cdbfd5678bee19484ce1b5d040`.
These are bounded hardware observations, not proof of a complete game.

## Distinguish texture aliasing from geometry corruption

The initial rooftop suit looked covered in red/blue/white pixels. Removing
the `count > 64` zero-copy bypass did **not** remove that symptom. Neither
cubemap min-filter clamping nor changing 2D filters alone was sufficient.

In log 074, a diagnostic shader rendered diffuse-only and normal-only in the
same scene. The normal image was coherent; the diffuse image retained the
high-frequency pattern. Do not infer a broken ETC1 decoder or change lighting
formulas from the appearance alone.

Log 075 identified the actual bound material textures:

- Diffuse: uncompressed RGBA, 1024x1024, one mip level.
- Normal and specular: ETC1, 128x128, existing mip chains.
- Generating the missing diffuse mip chain and selecting trilinear minification
  removed the severe rooftop suit aliasing in a new hardware capture.

Target the material by its verified shader signature and sampler bindings.
Do not generate mipmaps for all UI/render-target/compressed textures. Preserve
the active texture unit. Track `glUniform1i` and `glUniform1iv`, invalidate the
per-texture result after level-zero replacement or ID reuse, and retain the
GPU lifetime rules of the texture allocator. Verify costumes, reloads and
transitions separately. Keep diagnostic shader modes disabled in normal builds.

## A mapped CPU array is not a GPU-owned snapshot

Retire the old `count > 64` lifetime heuristic for both vertices and indices.
Safe copies exposed an undersized circular pool: its configured size is split
among the in-flight display frames. The 8 MiB configuration therefore provided
about 2.67 MiB/frame. Logs 067/068 overflowed that budget and allocated extra
buffers; the largest observed excess was about 1.55 MiB.

Log 071 used 18 MiB total (6 MiB/frame), with zero overrun warnings in that run.
It retained about 7.4 MiB free RAM, 15 MiB PHYCONT and 43 MiB VRAM in the measured
scene. This is a title-specific budget, not a universal recommended size.
Validate streaming and shader compilation before increasing pools further.

Rebuild all vendor objects after flag changes; this Makefile does not track
`CFLAGS`. Do not compare builds containing mixed old/new objects.

## The profiler can cause the reported hitch

`vglMemFree` uses `sceClibMspaceMallocStats` in this build. Per-frame-loop memory
reports measured about 264–267 ms each in log 075. Remove periodic heap walks
from the render path; retain boot/failure reports and explicit dev requests.

File telemetry measured roughly 100–150 ms per five seconds. Framebuffer BMP
capture takes seconds. Exclude capture, compilation and transition windows
from steady-gameplay FPS comparisons. Pause file telemetry for a performance
measurement, then request a fresh state explicitly.

Count draw submission separately from framebuffer copies. Earlier logs labelled
both as `copytex`; later logs show `draw_submit` and true `copytex` independently.
An initial rooftop at 444 MHz measured about 26–28 FPS with safe copies; menu
58 FPS is not a gameplay claim. Performance remains a separate open issue.

## Camera and readiness

The test Vita reported idle stick values such as `(143,131;111,127)` and
`(136,130;104,135)`, not an exact 128 center. The latter right-stick displacement
has radial length about 25; a release zone of 24 can remain active after a real
movement. The revised radial thresholds are activation 40 / release 32, with
rescaled motion outside the release zone. Test with actual idle data.

The right virtual gesture uses one DOWN, MOVE-to-center then MOVE-to-delta per
input sample, and one UP on release; avoid a new UP/DOWN every frame. Front
screen IDs start at 2. Log 070 confirmed right-active/down/delta=28 on a later
rooftop and zero down/delta after release. This does not validate every menu or
photography/cinematic state.

`menu=HUD` and `camera_enabled=1` can occur during loading/cinematics while
camera input is gated. Confirm main-loop input samples are advancing and inspect
the actual scene. `CameraManager::DoTouchInput` has a gate at `+0x6e9`;
`FullScreenRotateJoystick::Update` also resets touch when Application `+0x3cc`
bit `0x20` is absent. Preserve these gates unless their behavior is established.

Only inspect a new capture after the harness `captures` counter advances.
A timed-out capture followed by downloading the existing file can return a
previous run's image. Delete stale state at harness startup and bind observations
to an installed VPK hash/log number, not one source object's compile timestamp.


## Right-stick sensitivity and native delta time

The old full-scale input sent 28 horizontal / 20 screen-vertical pixels per
frame, making turn speed excessive and frame-rate dependent. The replacement
uses elapsed time, a progressive axis curve, fractional accumulation and a
50 ms hitch cap. Keep movement-stick scaling separate. Test partial travel,
reversal, release and multiple frame rates; include simultaneous left/right
movement so one stick cannot reset the other stick's accumulator.

Correction to early static notes: Application +0x380 is delta milliseconds,
not screen width. DoTouchInput subtracts 0.05 * delta_ms from touch movement.
Compensate that native threshold only on a moving axis, preserve native camera
gates, and do not multiply fixed per-frame pixels by a guessed screen scale.
The optional camera_sensitivity.cfg has horizontal/vertical multipliers 0.25–2.0.

Boot 080 crashed while copying the shader collection vector: an IShader pointer
was 1, and refcount increment faulted at address 5. The same binary booted in
081. Priority changes and a low-pointer strcmp shim are not synchronization or
proof of shader-manager stability. The input ETC1 base textures (415 records)
and four cubemap sets decoded coherently on the host; downstream rendering and
mip levels remain separate questions. These observations do not prove 100% QA.


### Native touch gain and ETC1 experiment (085)

FullScreenRotateJoystick::touchMoved at +0x344c30 multiplies X by 2 and Y by 3
when native width is <=1024. Account for this gain at Vita 960x544 in addition
to Xperia's Y / 2 mapping. Do not infer effective deltas from injected coordinates
alone; inspect rotate-pad state after native event processing.

Hardware input offsets also mix axes during an active gesture. Calibrate a stable
neutral on MainMenu only (bounded samples, no buttons/interception); gameplay
must not adapt its center while the player holds a stick. Verify simultaneous
movement and camera and vertical-only motion on the actual hardware.

In 085, SAFE_ETC1=1 replaced the pool-rooftop rainbow facade pattern with coherent
windows/reflections. Turning off mip filtering had not fixed that pattern. This
localizes a useful compatibility path, not the exact failing GPU conversion.
Software decoding expands texture memory; measure pools plus the external newlib
heap. vglMemFree(EXTERNAL) is hardcoded zero here, not evidence of no headroom.
Do not declare streaming safe merely because the first scene loaded.


### Keep frame-rate denominators honest

A watchdog DelayThread(5 seconds) is not proof of a five-second reporting
interval: scheduling and file I/O can delay it. Measure monotonic elapsed time
between reports before computing FPS or comparing cumulative CPU categories.
The pre-088 logs used a nominal interval. Treat those FPS figures as approximate,
especially when measured CPU time appears to exceed the nominal interval.

The 087 U16 client-index experiment avoids copying unused vertex prefixes by
rebasing an owned index snapshot and copying only the referenced vertex range.
VBO, instancing, base-vertex and converted primitives retain their established
path. Portable tests compare actual strided vertex fetches, including sparse
ranges and index 65535. Hardware counters showed 35–100 MiB avoided per sample;
do not translate saved bytes directly into a sustained-FPS claim.

### Cubemap face stride: no-mips is a special descriptor (093)

Isolate world diffuse/lightmap/reflection before changing source images. In 089,
facade blocks disappeared in diffuse-only and returned in reflection-only.
RGBA conversion alone did not fix it. In 091, six uploaded RGBA cubemap faces,
read from GPU memory and unswizzled on the host, exactly matched decoded ETC1.
Correct pixel bytes do not prove correct sampling layout.

GXM logical mip count 0 encodes 0xF (no mip chains, contiguous cube faces).
Logical count 1 encodes 0: even with one visible level, cube face strides reserve
the full chain and alignment. The compressed-upload tail in this vitaGL revision
used count 1 for every non-mipmapped target; that overrode the cube allocator's
count 0. Preserve count 0 for cubemaps, retaining count 1 for 2D. Hardware 093
verification is pending; do not label the issue closed before viewing the same
reflective facades and checking the descriptor.
Primary implementation evidence: https://github.com/Vita3K/Vita3K/blob/master/vita3k/renderer/src/texture/cache.cpp
(`upload_texture`, cube branch with `mip_count != 0xF`).

093 hardware result: the native material on pool-rooftop glass facades showed
coherent reflections after preserving the no-mips cube descriptor. Trace reported
mip count 0 for cube IDs 701/122/181. Keep this fix separate from software ETC1
2D sampling and suit mip-chain fixes; all three addressed different symptoms.
RGBA-only conversion was reverted after it failed to solve the stride defect.
Remaining water/fading/sky observations are not covered by this validation.

### Reconcile spatial detail, copying and real scheduling (095–098)

Use the shipped configs as evidence. The TASM port had reduced BuildingDist
12000/120000/1200000 to 4000/18000/70000 and streaming far 600000 to 95000.
096 restored those spatial limits via a reversible XML override on the same
VPK and revealed detailed nearby glass facades/terraces earlier. It also raised
geometry-copy demand: overflow messages reached 12,991,010 bytes ABOVE a 6 MiB
per-frame limit (~18.4 MiB total). The text "Total of" in this vitaGL diagnostic
is misleading: inspect its pointer subtraction. This allocator falls back to
GPU-owned dirty allocations; overflow alone does not prove buffer corruption.

097 used 72 MiB total/24 MiB per frame and had zero overflow/allocation-failure
messages in the collected run. It still measured ~10.7–10.9 FPS in the detailed
initial view; ring capacity alone did not solve CPU copying. Treat 72 MiB and
full spatial distances as experiments, not universal port defaults. Inspect
memory across long streaming paths before adopting them.

Do not infer CPU affinity from a comment or affinity argument 0. Runtime thread
snapshots showed main on core 1 and a worker executing on core 2, with others
waiting on cond semaphores or delay. A "single-core" explanation and automatic
starvation conclusion are unsupported for this runtime. Priority is not a lock;
native shader insertion has a spinlock, and corruption remains open. First
thread snapshot CPU counts are cumulative; later counts are since the prior
snapshot, not the perf-report interval.

098 tests a 64-entry same-frame client snapshot cache: exact source pointer,
sufficient copied extent and a full byte comparison before reusing GPU-owned
bytes. Changed bytes create a new snapshot. New frame invalidates metadata;
eviction never modifies old submitted bytes. Host tests include mutation at the
last byte, prefix bounds, retained previous draws, frame invalidation and
metadata eviction. Hardware benefit is pending. Keep the CMake option OFF until
verified, and measure copied/reused bytes plus actual FPS; comparisons also cost
memory bandwidth. Do not substitute a pointer-only key or a hash alone.

On HUD exit, release a calibrated stick using its calibrated center, not raw
128/128. 095 verified right_active/down reset when opening pause with the stick
still held. Native action numbers, time thresholds, touch gains and allowed
menu modes remain specific to this ELF.

### Reject caches when comparisons dominate (098–103)

098 exact same-frame snapshots had no hits. 099 subrange hits saved copies but
measured ~8 FPS. 100 persistent generic allocations could spill to slow CPU-read
VRAM; 101 forced external mapped heap but full comparisons still dominated.
102 NEON boolean equality passed actual Vita tests, yet comparison of ~315 MiB
cost ~3.5 s/5 s and the detailed initial view remained ~6.7 FPS. Both caches are
OFF in 103. A correct cache can be a performance regression; bytes saved are
not an acceptance criterion without frame-time measurements in the same scene.

103 tested sparse packed U16 record compaction, using
owned remapped indices and owned vertex records. Exact fetch-sequence host tests
passed, including index 65535, all indices, generation wrap and output bounds.
Do not generalize to shaders observing gl_VertexID, base-vertex, instanced draws
or mixed VBO arrays. Validate the ARM copy branch and hardware geometry/FPS
before accepting it for a new title. Actual Vita self-test passed; the detailed
initial TASM view improved from ~10.8 to ~15.7 FPS, avoiding ~420 MiB/5 s of
vertex copies. Traversal to the pool and combat showed coherent geometry in
those limited views. Long streaming/game completion are still unverified.

MainMenu calibration now learns independent left/right centers after 32 stable
samples within a 48-unit radial admission limit, never during gameplay. Hardware
neutral changed to left (161,131), right (83,123), and both became inactive in
101. MCP injection adds offsets: older raw compensation commands are obsolete
after calibration. Confirm app-read samples and use movement/release regression.

Build variants link a per-build-directory libvitaGL.a copied after vendor make
-B. The vendor object directory is still shared; build variants sequentially.
Archive build/tasm (the actual ELF filename), flags and VPK before installation.

104 SHF read timing separates lock wait, physical read/open, main versus worker,
bytes and errors. Initial load has some 0.5–0.9 s reads; stabilized initial
view has almost no archive work except audio, so do not blame steady low FPS
on the file lock. Use trylock to snapshot/reset counters without blocking the
watchdog, report actual interval duration, and distinguish disk time from
decompression and integrating streamed objects. Existing lock also prevents
LRU descriptor close during pread; do not remove it without pinning lifetimes.

105 exact NEON contiguous copying passed Vita byte/bounds tests but was slower:
warm 8 MiB external->RAM CLIB ~9.6 ms versus NEON ~19.5 ms, and ~10 FPS in the
same initial view. NEON copying is OFF in 106. Vector code is not automatically
faster than the platform libc. Measured source bytes were RAM/PHYCONT/external,
not VRAM. vglMalloc/Memalign/Calloc in this fork already try external before
VRAM; the generic internal GPU CPU allocator has a different fallback order.
Inspect the actual path, not stale comments. New pool peak metric remains
available without HAVE_DEBUG_INTERFACE; its built-in debug counter was absent
and caused a link failure caught before installation.

106 reduced the circular pool from 72 to 36 MiB after measured peak fell under
6 MiB/frame with compaction. Initial view remained ~15 FPS, while external heap
usage dropped from ~68.5 to ~36 MiB. Traversal raised peak to ~8 MiB/frame;
12 MiB/frame is still an experiment requiring longer routes. Pool size changes
redistribute other game allocations between RAM/PHY/external, so copied bytes
and frame times can shift without a draw-count change.

107 AtlasMipmapSkip=0 exhausted memory with software ETC1 expansion (12 MiB
single texture allocation, native malloc failures). Restored 096 spatial XML.
CustomTexturePolicy::getMipmapCountToSkip identifies CityAtlas before the other
categories. Use exact shipped policy to scope a texture experiment, rather than
raising every setting at once. Treat stale BMP after capture timeout as invalid.

108/109 synthetic decoder probes initially drew no geometry. Equal black/clear
images are not success: add clear/readback control, verify shader/attributes,
and require a spatially varying known-good reference before comparing codecs.
110 used an owned VBO and indexed triangles, obtaining a valid reference.
Native ETC1 original bytes matched software exactly for 16x16/32x16; deliberate
byte/word permutations produced large errors. 111 tested valid differential
blocks, each stored mip 32x16..1x1, and actual LOD 0..5 using distinct solid
colors: all RGB differences zero. This validates those paths on that Vita,
not every game texture or the whole prior hardware-ETC1 hypothesis.

SAFE_ETC1 in this fork expands ETC1 to RGB888 (6x compressed storage). Its
non-native uploads at level>0 generate mipmaps and ignore the decoded supplied
level. This makes memory/fidelity materially different from native ETC1.
112 tests existing native 2D ETC1 with the already-fixed software cubemap stride.
Do not reintroduce all software expansion merely because old screenshots before
the cube-descriptor fix showed rainbow reflections; test the independent causes.

Diagnostic filter_original must reapply effective suit mip filters after restoring
native requested filters. Otherwise it undoes the correction while done flags
prevent restoration. This reproduced severe suit noise in 106 and suit_mips
cleared it; source now invalidates those done flags on filter restoration.

112 native ETC1 gameplay with fixed software cubemap stride showed coherent
initial suit/facades in the inspected views, ~40 MiB free VRAM and nearly empty
newlib heap, without improving steady ~14 FPS. 113 original city-atlas mip loaded
successfully using native compression; roof flooring improved, distant billboard
blur remained, external heap ~11.5 MiB. Full software expansion had exhausted
memory for the same atlas setting in 107. Native textures still need broader QA.

115 prepared two-pass compaction reduced copied bytes and pool peak, but initial
FPS ~13.5 and submission ~2.86 s/5 s did not improve. OFF in 116. Test fetch-byte
equivalence separately from performance; scanning/remapping has its own cost.

116 found a concrete guard regression: PROG_INVALID=0 was both link-failure
state and free-slot sentinel in glCreateProgram. A missing shader could make
glIsProgram false and let another allocation reuse a live name; glAttachShader
also requires PROG_UNLINKED, preventing recovery. Failed link must retain the
name. Vita regression test failed-link -> distinct new allocation -> attach
missing shader -> link recovered returned result=0, names 1/2, link=1 after fix.
Do not turn a defensive early return into object deletion. This does not prove
the root cause of missing shaders or the older native collection crash.
## DMA is not automatically a faster owned copy (119)

Use a synthetic source/destination with byte comparisons and before/after guards
before touching render geometry. Cached addresses rejected by a library's old
heuristic may still be accepted by the kernel API; acceptance proves neither
speed nor GPU lifetime. Here 12 tests covered RAM/PHY, offsets 0/1 and chunks
8/64/256 KiB across a 4 MiB working set. All returned zero with exact bytes and
guards, but DMA took 28.7–55.5 ms versus CLIB 16.5–20.2 ms. Keep the current copy.
Do not replace global memcpy, infer gains from bandwidth folklore or use menu
FPS as gameplay evidence. Keep allocation/correctness checks outside timing.

## Client vertices can use an EBO (120–122)

An element buffer does not imply a vertex buffer. Metadata from real world
draws showed EBO indices with interleaved client vertices. The old admission
guard excluded these draws from owned min/max rebasing and sparse compaction.
The EBO CPU pointer plus byte offset was already available; reading it into
owned remapped indices is safe under the same all-client, U16, native-primitive
guards. Keep vertex VBOs, instancing, base-vertex and converted modes unchanged.
Never mutate the original EBO or infer vertex lifetime from its existence.

Hardware regression rendered a colored quad into a 32x32 FBO and compared all
bytes against a VBO reference: EBO offset4/prefix64, sparse indices including
65535, planar strides8/12, and ordinary VBO+EBO. All four were exact, GLerror0,
binding preserved. Require the reference to draw/variate, avoiding black-image
false positives. Gameplay gain was small (~15.1–15.2 vs14.5–14.9 FPS), despite
copy savings. Preserve inclusive timing and distinguish game scene from menu.

## Verify contextual buttons from the actual HUD (122)

Do not copy labels from Android keycode names or another port. In TASM,
Cross23 jumps/swings in exploration and attacks in combat. Square99 without
shoulder modifiers accesses spray, so repeated Square was received but did not
prove melee failure. Native108 opened map and109 opened pause/options; the
inherited comments were inverted. REA ProcessMenuHUD plus actual screenshots
resolved the mismatch. State raw buttons only proves polling, not the resulting
game action. Tutorial locks and transient SCE_CTRL_INTERCEPTED must also be
separated from camera calibration failures.

## Distant city fidelity may be a separate mip policy (124)

TASM's CustomTexturePolicy selects AtlasMipmapSkip for CityAtlas, but
LowLodMipmapSkip for Map__ textures. A configured nonzero skip is incremented
again in the observed branch: setting1 may discard2 levels. Atlas0 alone did
not repair distant ads. Restoring LowLodMipmapSkip0 (also the original lowphone
value) made ads, distant facades and street markings visibly sharper with the
same spatial limits and native ETC1. This increased newlib usage (~51MiB in the
measured view). Verify streaming memory and failure logs before adopting it.
Do not patch the exported enableLowMemoryPhone flag based only on its BSS zero;
its runtime state and all consumers need independent evidence.

## Compact scratch and native buffer policy (125–129)

Generation16/remap16 in one uint32 entry reduced sparse-compaction scratch to
262148 bytes and passed host plus Vita/FBO equivalence, including65536 unique
vertices and generation rollover. Resetting generation alone over old tags is
invalid; clear the table on initialization/wrap. Initial-view gain was small.

Probe buffer factories by caller as well as type/usage; a per-enum quota can
be exhausted by menu buffers before the world loads. Verify the sret ABI and
config layout from this ELF, bound the metadata, preserve original arguments.
TASM World::LoadWorldScene allocates a25MiB external CPU vertex buffer (usage4).
An opt-in change of this one config to native dynamicVBO (usage1) copied the
initial buffer but lost detailed city geometry in build129. Reject the result
even if FPS rises. Existence of lock/unlock and dirty-range methods does not
prove that streaming calls them for every write. Global SubData counts do not
prove updates to that specific VBO. Do not generalize enum values across games.

For read-only diagnostics, inspect map/unmap implementation before using it:
vitaGL here resets last_frame on unmap, affecting buffer retirement. Prefer an
explicit non-mutating comparison on a live owned buffer, invalidate tracked
names on deletion/reallocation, and log metadata rather than game vertices.
Concurrent source writes limit a single comparison; repeat after loading.

Build130's25MiB VBO was initially byte-identical to the engine source, then
differed in all6400 pages without any SubData to that GL name. A diagnostic
GPU-idle full copy in131 later restored byte equality but did not repair the
image. Treat missing updates and attribute interpretation as separate checks;
byte-identical storage alone does not prove correct vertex fetches.

The packed VBO path wrote full GL byte offsets into SceGxmVertexAttribute.offset
(uint16_t). Build134's valid quad at0x2000c reproduced corruption:4064 framebuffer
bytes differed from the ordinary reference, GLerror0, while cases0..3 passed.
Move the common prefix into the stream pointer before narrowing relative
attribute offsets. Use separate streams when their separation cannot fit16bits.
Verify DrawElements, nonzero first DrawArrays, instancing, reversed attribute
order, planar equal strides and multidraw. Build135 passed all10hardwarecases
with byte-identical framebuffers/error0/binding preserved. Native trace136 then
showed real city offsets0x1033408 and0x12ee808. A GPU-idle sync plus the rebase
restored detailed buildings in that view. After traversal384pages diverged
again without SubData; the addressing fix alone does not repair native streaming.

REA of the writer revealed memcpy inCVertexDataAllocator::alloc, destination
from buffer+12 plus allocation offsets. A nearby buffer method51e3a4 is resize,
not dirty notification. Decode its argument use before hooking it. A page bitmap
of observed imported writes can guide coverage investigation, but zero uncovered
pages cannot prove every write/byte or later reuse was captured. Keep worker
notifications metadata-only and perform GL operations on the rendering thread.

TASM138 disproved memcpy-only coverage after traversal:1023divergentpages,
430marked,593unobserved.139's EABI additions still left275unobserved in another
period.140's I/O/zlib probe reported zero calls to those paths for the city
buffer; its240unobserved pages cannot be attributed to decompression. Internal
glf::Memcpy is only a tail branch to imported memcpy. Do not assume SIMD based
on its name. Examine byte counts/offsets in unobserved pages: an in-place arena
can change allocator headers through inline stores, which need a different
interpretation from vertex bytes. That distinction still requires evidence.

141 found219changed bytes in10unobservedpages, first offsets at a header's
0/4/8fields. REA of the allocation/free policy proved direct stores to next,
prev and size.142 hooked those verified functions only for this live city
arena, preserving r0and the fifth free argument. Runtime chunk size was1024B.
Marking the affected chunk conservatively covered1052divergentpages after a
later traversal (1065marked, zero uncovered). It also marks unchanged bytes;
do not infer copy bandwidth or equivalence solely from notification counts.

143's second opt-in marker tests dirty-page uploads once before the first draw
of a frame. Snapshot flags atomically, retain notifications arriving afterward,
wait glFinish before mapping/writing owned VBO storage, then unmap and restore
binding. Never let a worker issue GL or alias the engine's mutable CPU arena as
GPU storage. First full25MiB sync took111ms; a busy loading window added705ms,
so stable-view gains are not loading-time proof. Limited hardware checks showed
correct detailed city, zero differences after settling/traversal,24–25FPS in the
initial-like view vs prior15–16. Other views remained~20FPS. This experiment is
not release-approved; long streaming, combat, memory and saves remain open.

The app's cmd.json is a single slot. Use a request_id in each command and wait
until fresh state.request_id matches before sending the next.139 validated
IDs1..8 and command_count in hardware. This acknowledges consumption, not success
of unsupported commands. Waiting for an already-passed frame or an old log marker
does not acknowledge a newly submitted command. Await build exit0 before copying
or installing artifacts, even if a previous VPK already exists at the same path.

Vita input injection can report success while focus/interception prevents the
app from receiving buttons. Prove held/released raw state on a fresh frame.
A dev state file from a previous boot may satisfy MainMenu immediately: require
a fresh log/run and increasing frame counter before interpreting it. Recovery
of127 used a normal reboot; no system plugin or volume changes were needed.

Camera133 reduced defaults to0.65 horizontal/0.75 vertical without changing
time-based curves, neutral calibration or touch gains. Hardware full horizontal
delta dropped14->10; reversal, vertical isolation, both sticks, pause while held
and neutral resume passed. These are gesture checks, not angular measurements
or proof of the user's preferred sensitivity. Preserve an optional per-axis cfg.

Intermittent native corruption needs a controlled boot matrix.144 crashed before
first swap in a COLLADA resource std::map::find with key pointer1;143 OFF/ON and
144 OFF/ON later booted. That does not isolate a culprit or prove a fix. A core
may be gzip rather than raw ELF; preserve both hashes. The live LR can be a
scratch register: recover saved return addresses using the actual prologue and
included stack segments. Missing heap PT_LOAD data cannot be reconstructed by
guessing. Closest addr2line symbols without DWARF may name unrelated STL helpers.

Do not let a watchdog print a table the render thread is still filling.149
moved the contextual state report to the main-thread dev poll and deferred
report/rearm until sampled nested calls return. Loads may swap inside those
calls. A 5s request that expires with sampled_span0 admitted no samples; it is
not proof loading cost was zero. Heap-owned names stayed excluded rather than
weakening pointer checks just to improve diagnostic labels.

HUD can become current during a cinematic/loading transition. Confirm a fresh
image/player state before timing a route.148 reached real combat using jump250ms,
then jump+forward1800ms, release both, settle, Web-Strike250ms and attack taps.
The HUD confirmed4hits. This is a scene-specific reproduction, not universal
timing. Command counts are not hit counts. Wait HUD after resuming Pause before
the next attack, and avoid leaving the character exposed during slow screenshot
pull/analysis.148's combat still ran~17FPS and the mission was not completed.

150 reduced only the streaming add/safe radii while keeping LOD and mip policy.
The initial view remained~26FPS, close to149's25–26; no material improvement
justified reducing prefetch range.151's LowRes_Sea0 did not remove the pool's
flat-blue surface. Both changes were rejected/restored pending actual shader
and streaming evidence. A black frame early in151 later became a normal scene;
HUD state and compile-call durations alone cannot identify readiness/cache hits.

For XML profiles with group/prop names, use the packaged
[config helper](../scripts/make_runtime_config.py):

```powershell
py scripts/make_runtime_config.py baseline.xml experiment.xml --set Streaming.RadiusAdd=10000
```

Use the actual schema/values for the target game. It preserves the baseline,
refuses existing output/unknown or duplicate keys, prints semantic changes and
both hashes, and emits UTF8/LF deterministically. CRLF originals can have a
different hash with identical XML values. Keep the deployed bytes and hashes;
do not overwrite an earlier experiment merely to reproduce its semantics.
Separate repeated press events with a release interval the game's slowest poll
can observe. A release immediately followed by press may never reach native
input, even though the remote tool acknowledges both commands.

153 measured zero sched_yield calls through boot/load and the tested roof view,
despite glf::Thread::Yield tail-branching to that import. Do not change scheduling
just because an inherited no-op looks suspicious; its relevance needs runtime
counts. The optional delay experiment was never activated/adopted.

154 fixed a concrete logging fault in the port and FalsoJNI: pass an already
formatted buffer with printf("%s", buffer), never printf(buffer). Shader/JNI
messages and ordinary percentages can otherwise become a second variadic format.
The on-device literal test containing %s/%d/%n/%% returned normally in both sinks.
Also clamp a write length after appending snprintf's repetition suffix: snprintf
returns intended length even when the remaining buffer truncates it. Neither
finding alone establishes the cause of an earlier native container crash.

154 completed the first combat, NPC pickup/transport to the street and LEVEL2.
Settled city CPU/VBO comparison still showed0/6400 different pages after rescue.
This covers more than a stationary rooftop; many other frames were Pause, and
save persistence/long play remain unverified. Generic Vita savedata backup can
contain only sce_sys metadata for an Android loader. Find and explicitly back up
the game's actual redirected save files before editing/testing persistence.
An exported SaveAllBytesOfSaveFile can merely update RAM; follow the caller to
the actual disk transaction. Audit rename against the same namespace and file
cache used by fopen, and demonstrate a cross-API failure before attributing a
missing checkpoint. Never infer joystick UP rejection from coordinates alone:
TASM StickWalk::touchEnded checks pointer ID and clears its state, not UP bounds.

155 proved loss of LEVEL2 after a normal menu exit and restart. The Android
save directory held an empty glcloud.sav and a948-byte tempSaveFile.dat; the Vita
savedata tool had backed up only sce_sys.156 remapped both rename arguments,
matching fopen/remove/unlink. Runtime EndSave used absolute /data/data/... paths
and then successfully rotated glcloud->backup and temp->glcloud. Preserve the
temporary file before relaunch: a fresh game can overwrite the recoverable blob.

Test failure behavior as well as successful replacement. This linked VitaSDK's
_rename_r removes destination before attempting sceIoRename; a missing source
can destroy the prior destination.157 preflights source and serializes wrapper
calls. Hardware tests covered Android alias/relative paths, replacement, same
file aliases and missing source: result0, errnoENOENT, previous_preserved1.
This is a bounded correction, not a claim of general crash-atomic transactions.
Print operation status early when a logger deduplicates only a bounded prefix;
long paths can otherwise hide different return values. Keep unused legacy
FCACHE_ENABLED0 code separate from the active shared-archive path; do not blame
inactive cache code for runtime corruption or enable it blindly for speed.

158 recovered LEVEL2 by preserving the original temporary save and copying its
unchanged bytes to the primary/backup paths confirmed by native EndSave.
159 then restarted again without a manual copy and retained LEVEL2; native
saves were nonempty and advancing. Keep the original recovery blob intact.
160/161 also loaded LEVEL2. This validates the tested checkpoint transaction,
not every later mission or failure during power loss.

Measure instrumentation itself with runtime bypasses that still call each
original target exactly once. Keep frame/FPS counters independent, confirm the
mode in fresh state and preserve player snapshots. Exclude transition windows,
capture, texture upload and loading, and report sample counts/temporal drift.
160's ON/OFF/ON/OFF stationary test gave35.90 vs37.05FPS medians(n8/n7), about3.2%
in this pose. Default timing became OFF in161; explicit timing remains available.
Other poses and combat can have very different costs. Traversal still ran~18.5FPS,
so disabling probes does not solve the main performance issue. A city-buffer
byte comparison covers that buffer only: geometry traces showed other CPU mesh
arrays still copied each frame.161's exact-caller meshVBO policy is opt-in and
unvalidated; do not adopt it from this note. See the live diary for its outcome.

162's exact COLLADA caller promotion was actually activated but did not reduce
the dominant client-copy volume or clearly improve this view. It was disabled.
163's separate3MiB batch policy never reached its target allocation: the known
startup COLLADA-map crash recurred first, with the samePC/DFAR/node as144.
Its first performance/graphics test could not run. Do not attribute a startup
failure to a code path the log proves was never admitted.

When heap thread safety is uncertain, test the actual allocator in a disposable
arena before replacing the game's heap.166 exercised Sony mspace with4096serial
and16384concurrent allocations, four workers on three cores, bounded barriers,
range/overlap checks and payload canaries. Both passed. This does not prove
native object lifetime or rule out other allocator sequences.165's thread
creation failure was a test setup failure, not an allocator failure. Preserve
worker error codes, bound waits and retain the arena/context until process close
if workers fail to join or corruption makes normal destruction uncertain.
The tested source and integration notes are packaged in
[assets/mspace-selftest](../assets/mspace-selftest/README.md); the repository
knowledge updater mirrors their canonical C sources and verifies their hashes.

A repeated fault address can belong to a different allocation in a good boot.
167's watched address was covered by a44-byte GlitchAlloc block, not established
as a map node in that execution. REA showed GlitchAlloc calling malloc then
memset, so the imported malloc caller alone did not identify the object creator.
168 adds the verified outer allocator caller and real map header metadata,
only under an opt-in diagnostic marker. Keep original calls and distinguish
observed imported writes from unobserved inline/custom-allocator writes. A probe
can change timing/layout; successful instrumented boots do not certify a fix.

169 finally admitted the separate3MiB batch candidate and failed while copying
7536bytes fromNULL. Its core signature differed from the COLLADA-map fault.
The policy was rejected/disabled; the same binary with the marker removed
reached MainMenu and LEVEL2 again in170. Native GPU ownership does not guarantee
that callers retain CPU data access. Do not suppress theNULL copy or label a
successful buffer creation as a validated optimization.

172 observed FindMemBlockByAddr(pointer,nonzero_size) returning80020001 on this
device while GetMemBlockInfoByRange succeeded with readable permissions and an
extent covering the range. Use the range query for diagnostic span validation;
mapped memory still does not prove object lifetime or type.171's missing path
logs were an inactive guard, not validation of the traversal.172/173 exercised
the corrected mirror without seeing another bad node/worker find. This does not
rule out other mutations or timing hidden by instrumentation.

174 found questionable inherited semaphore timeout semantics, then observed
zero calls to those imports through boot/HUD and tested traversal. Do not patch
a static concern as the current bottleneck without runtime evidence. Newlib's
linked __malloc_lock/unlock called real LwMutex APIs; the private Sony mspace
test is different coverage from proving every newlib/game allocation sequence.

175/176 measured the original texture policy instead of guessing from XML names.
A generic log quota was exhausted by UI textures before the world loaded; use
separate bounded quotas for relevant families. Runtime LowMemoryPhone was0.
REA confirmed that _lod matches before building_, and CityAtlas is independent
of Map__. BuildingMipmapSkip1 returned2 for observed building textures; changing
only that property to0 in177 returned0 and visibly sharpened the near facade.
Same pose median FPS37.17 vs37.27(n10/n7) was within temporal variation. Memory
usage increased, so retain measured traversal and memory checks. Current-arena
free blocks do not represent the remaining reserved heap capacity.

Do not exclude real streaming from a traversal FPS measurement.177's capture-free
interval rendered388frames in19.633341s (~19.76FPS), with textures streaming.
Capture can affect the current AND adjacent watchdog windows before its stall
is published. A low max-frame counter alone does not certify capture exclusion.
The accepted facade setting did not eliminate delayed high-detail arrival.

Use actual virtual targets when inclusive actor timing hides the expensive part.
178 intercepted verified ARM MOV LR,PC / LDR PC call pairs, preserved this/delta
and the three-argument sync force, forwarded the live parent only to the wrapper,
and called each original exactly once. Measured components, sync and UpdateEnemy
totaled~0.56ms/update on both tested roof and street. That does not explain every
MainCharacterUpdate cost (2.76ms in the second street, ~11ms in another177 pose).
Keep context/pose differences explicit; symbol-nearest labels such as acosf+offset
or STL+offset are not target identities. ARM virtual calls may contain no BLX.
Diagnostic epochs and active flags prevent accepting stale ACKs or old reports.
179 extends this opt-in profile; its hardware outcome belongs in the live diary.
180 identified UpdateEffect as~1.7ms/update on one roof,~0.5ms in another street.
181 isolated its lighting-only world ray (~1.6ms). An exact-key100ms cache was
actually exercised and gave a small stationary gain in a repeat (~36.38->37.66),
with temporal drift and an intercepted-input event. It remains OFF; it adds a
bounded lighting delay when geometry changes and does not accelerate moving rays.
The first A/B was confounded by changed draw counts and an actors.gla texture
upload. Do not label an unidentified late actor upload as a late-building fix.

182's timed-condition counters stayed zero through boot/HUD/traversal. Static
REA found an engine-specific microsecond field, but that cannot explain an
unobserved runtime path. A generic small-nanosecond heuristic is ambiguous:
identify exact callers/opcodes and test second rollover, unnormalized fractions,
expired deadlines and64-bit arithmetic. Also verify CLOCK DOMAIN: TASM's imported
gettimeofday uses process time, whereas the SDK call in the pthread shim uses
wall time.184 corrects the experimental selector for known callers; it remains
OFF and was not hardware-exercised. Arithmetic unit tests are not runtime clock
validation. Unknown condattr clock behavior remains separate.

183 globals showed CameraFarDist600000 and CameraNearDist10000 feeding integer
far/mid planes; the actual near plane was float10.0. ELF OBJECT size4 does not
prove a C integer. The first diagnostic printed float bits as1092616192; fix the
interpretation, not the native value. REA SetCameraRenderPass confirmed split
passes with overlap. Runtime radius globals changed between menu and loaded world.
Keep configuration readers/consumers separate, and verify active values after
loading before tuning distances.184's far-plane reduction is still experimental;
see the current state at the beginning of reference15 for outcome and rollback.

184/186 distance candidates were rejected: far120000 had no convincing gain and
an uncertified horizon; mid20000 increased draws and reduced roof FPS to~35.
Equal input durations did not yield equal routes; do not call those precise A/Bs.

187 exposed hidden deployment differences: on-device override and cached flags
were good, built-in XML/fresh CMake defaults stale. Compare all parsed properties,
preserve override priority, configure fresh and compare generated flags. Boot
without override.187's61 properties and facade matched the accepted control.

188 moved verified city policy outside DEBUG. Enable promotion+dirtysync together
only after allocator hooks verify; disable markers win per process. Keep extra
mesh/batch/resource experiments separate. Markerless hardware passed, and189
ran the core with DEBUG OFF. Release checks exposed implicit diagnostic function
declarations and one debug-only DMA symbol; successful debug linkage misses these.

189 separates file harness from verbose logs. Publish check request AND result;
ACK is not success. City compare differed during streaming and equaled after
settling: concurrent CPU writes/dirtypages matter. Preserve both observations.
File diagnostics have measurable I/O cost; telemetry_pause still polls commands,
DEBUG OFF still retains error-level perf logs. Roof ABBA suggested cost with
temporal drift, not an exact traversal gain. Capture/check time is also DEVFILES.

On SCE_CTRL_INTERCEPTED stop dependent automation. Resuming with close_current:
false/force:true cleared it in186; frame/time/request proved SAME process, not a
fresh boot. Correct stick injection adds to the physical stick. List paths before
guessing reference filenames. Apply_patch delete+add of one path needs separate
operations; rejection occurred before mutation.

190 exercised the explicit CPU disable on the SAME binary: unavailable city
compare, preserved facade,~36.77roofFPS. Marker removed;191 restored GPU/active1.
Do not confuse a run/log number with a rebuilt binary.191 defaults verbose logs,
harness and native timing OFF in fresh CMake; explicit test cache enables harness.

191 makes quiet harness polling1Hz/state0.2Hz, plus menu changes/explicit ACKs.
Fast mode preserves4Hz/1Hz for input work. Same-binary ABBA measured~26.2→7.0ms/s
file diagnostics (~73%less) and36.6–36.9→38.1roofFPS. Traversals remained~21FPS:
this is a diagnostic overhead reduction, not the main streaming/performance fix.
Counters in a published state exclude the ongoing cycle; comparison/capture also
contribute to DEVFILES. A capture took~14s: exclude its adjacent windows too.

Expose live policy/pending flags separately from last-check page coverage and
check request/result.191's early check had10different/12marked/uncovered0;
post-travel200different/218marked/uncovered0, bothpending1. Settled repeats had0
differences/pending0/errors0. These distinguish known pending updates from an
unobserved persistent divergence, without relabeling differences as PASS.

Assert HUD and !(buttons&SCE_CTRL_INTERCEPTED) in automation BEFORE injection.
191 first sent a camera input despite the flag; it was received raw but ignored,
so that test was invalid. Finally release controls, resume the same app, obtain
fresh ACK/state, and retry only after interception clears. Valid repeat showed
dx6/dy0/down1 and release delta0/down0. Check the coordinate pipeline end to end:
Xperia halvesY, so the port's factor-2 compensates that before native touch gain.

SHF global-lock wall time is not automatically main-thread waiting:188 had
hundreds ofms total contention but main_wait only0–16ms in those windows. Read
time was several seconds. Do not implement descriptor-unlocked pread without
inflight leases/eviction protection or claim it solves a low main_wait path.
