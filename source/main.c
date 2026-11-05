#include "macho/macho.h"
#include "objc/objc_classes.h"
#include "port_author.h"
#include "utils/glutil.h"
#include "utils/logger.h"

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/sysmem.h>
#include <kubridge.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/power.h>
#include <psp2/touch.h>
#include <psp2/ctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int _newlib_heap_size_user = 112 * 1024 * 1024;
uint32_t g_got_base = 0;

__attribute__((naked)) int redeg_call_game_main(uint32_t fn, int argc, char **argv, uint32_t got) {
    __asm__ volatile(
        "mov r12, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "mov r9, r3\n"
        "bx r12\n");
}

#include <pthread.h>

static void *heartbeat_thread(void *p) {
    (void)p;
    for (;;) {
        sceKernelDelayThread(2000 * 1000);
        l_info("[heartbeat] alive");
    }
    return NULL;
}

static void *selftest_thread(void *p) {
    (void)p;
    SceUID fd = sceIoOpen(DATA_PATH "selftest_thread.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, "THREAD-OK\n", 10);
        sceIoSyncByFd(fd, 0);
        sceIoClose(fd);
    }
    l_info("[selftest] thread running");
    return NULL;
}

static void probe_low_memory(void) {
    uint32_t addrs[] = {0x1000, 0x10000, 0x100000, 0x40000000};
    for (unsigned i = 0; i < sizeof(addrs)/sizeof(addrs[0]); i++) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = 0x1;
        opt.field_C = addrs[i];
        char name[32];
        sceClibSnprintf(name, sizeof(name), "lowprobe_%u", i);
        SceUID id = kuKernelAllocMemBlock(name, 0x0C20D060, 0x1000, &opt);
        void *base = NULL;
        if (id >= 0) sceKernelGetMemBlockBase(id, &base);
        l_info("[probe] map 0x%08x -> id=0x%x base=%p", addrs[i], id, base);
        if (id >= 0) sceKernelFreeMemBlock(id);
    }
}

int redeg_stat(const char *path, void *out);
int redeg_stat(const char *path, void *out);
int redeg_stat(const char *path, void *out);
int redeg_stat(const char *path, void *out);
int redeg_stat(const char *path, void *out);
void redeg_dump_hooks(void);
void redeg_s3e_probe(void) {
    uint32_t delta = g_macho.delta;
    uint32_t *g = (uint32_t *)(uintptr_t)(0x6fb608 + delta);
    l_info("[s3e] prefix0=0x%08x prefix1=0x%08x", g[0], g[1]);
    {
        unsigned char *inuse = (unsigned char *)(uintptr_t)(0x6fb608 + 0x4c4 + delta);
        unsigned next = *(unsigned *)(uintptr_t)(0x6fb608 + 0x8e4 + delta);
        char bits[40];
        int used = 0;
        for (int i = 0; i < 32; i++) {
            bits[i] = inuse[i] ? '1' : '0';
            used += inuse[i] ? 1 : 0;
        }
        bits[32] = 0;
        l_info("[s3e] slots used=%d next=%u [%s]", used, next, bits);
    redeg_dump_hooks();
    {
        const volatile unsigned char *mp = (const volatile unsigned char *)(uintptr_t)0x9822db84u;
        l_info("[hook] mode@9822db84: %02x %02x %02x %02x '%c%c%c%c'",
               mp[0], mp[1], mp[2], mp[3], mp[0], mp[1], mp[2], mp[3]);
        const volatile unsigned char *m2 = (const volatile unsigned char *)(uintptr_t)0x98022b84u;
        l_info("[hook] mode@98022b84: %02x %02x %02x %02x", m2[0], m2[1], m2[2], m2[3]);
    }
    }

    static unsigned char st[128];
    for (int i = 0; i < (int)sizeof(st); i++) st[i] = 0;
    int r = redeg_stat("ux0:data/redeg/game/s3e.icf", st);
    unsigned mode = *(unsigned short *)(st + 4);
    long long size = 0;
    memcpy(&size, st + 60, sizeof(size));
    l_info("[s3e] stat(s3e.icf) r=%d mode=0x%04x (reg=%d dir=%d) size=%lld",
           r, mode, (mode & 0xF000) == 0x8000, (mode & 0xF000) == 0x4000, size);
    for (int i = 0; i < (int)sizeof(st); i++) st[i] = 0;
    r = redeg_stat("ux0:data/redeg/game/Documents", st);
    mode = *(unsigned short *)(st + 4);
    l_info("[s3e] stat(Documents) r=%d mode=0x%04x (reg=%d dir=%d)", r, mode,
           (mode & 0xF000) == 0x8000, (mode & 0xF000) == 0x4000);
}

void redeg_s3e_open_test(void) {
    uint32_t delta = g_macho.delta;
    uint32_t *g = (uint32_t *)(uintptr_t)(0x6fb608 + delta);
    l_info("[s3e] idle: prefix0=0x%08x prefix1=0x%08x", g[0], g[1]);

    static char gp0[256];
    for (int i = 0; i < (int)sizeof(gp0); i++) gp0[i] = 0;
    int (*getpath)(char *, const char *, int, int, int) =
        (int (*)(char *, const char *, int, int, int))(uintptr_t)(0x1a7d4 + delta);
    int gr = getpath(gp0, "s3e.icf", 9, 256, 1);
    l_info("[s3e] idle: GetSystemPath -> %d '%s'", gr, gp0);

    void *(*openf)(const char *, const char *) =
        (void *(*)(const char *, const char *))(uintptr_t)(0x1b7c0 + delta);
    l_info("[s3e] idle: calling s3eFileOpen");
    void *fh = openf("s3e.icf", "rb");
    l_info("[s3e] idle: s3eFileOpen -> %p", fh);
    int (*geterror)(void) = (int (*)(void))(uintptr_t)(0x153b8 + delta);
    l_info("[s3e] idle: error=%d", geterror());
    if (fh) {
        void (*closef)(void *) = (void (*)(void *))(uintptr_t)(0x199b4 + delta);
        closef(fh);
        l_info("[s3e] idle: closed");
    }
}


/* ------------------------------------------------------------------ *
 * Guest instrumentation: patch call sites with a thunk that calls the
 * original function and records its arguments and return value.
 * ------------------------------------------------------------------ */
static uint32_t s_hook_code = 0;
static uint32_t s_hook_data = 0;
static int s_hook_ready = 0;

typedef struct {
    uint32_t site;       /* address of the BL instruction to patch */
    uint32_t target;     /* original call target */
    uint32_t slot;       /* thunk offset in the code page */
    uint32_t scratch;    /* scratch offset in the data page */
    const char *name;
} hook_def_t;

static const hook_def_t s_hook_defs[] = {
    { 0x1b4d4, 0x180b4, 0x000, 0x000, "InterpretMode" },
    { 0x1b6f8, 0x18444, 0x080, 0x040, "UserFS" },
    { 0x1b668, 0x1a7d4, 0x100, 0x080, "GetSystemPath" },
    { 0x1cd28, 0x22b7c8, 0x180, 0x0C0, "open" },
    { 0x220558, 0x1b7c0, 0x200, 0x100, "s3eFileOpen" },
};

typedef struct {
    uint32_t vmaddr;
    uint32_t orig;
    uint32_t slot;
    uint32_t scratch;
    const char *name;
} entry_def_t;

static const entry_def_t s_entry_defs[] = {
    { 0x2bad0,  0xE92D40B0, 0x280, 0x140, "SurfaceShow" },
    { 0x2da64,  0xE92D4090, 0x300, 0x160, "GL_start" },
    { 0x21de78, 0xE92D40F0, 0x380, 0x180, "SurfaceShow_platform" },
    { 0x2bb44,  0xE92D40B0, 0x400, 0x1a0, "SurfaceInit" },
};

static void build_entry_thunk(uint32_t thunk, uint32_t orig, uint32_t scratch, uint32_t ret) {
    volatile uint32_t *w = (volatile uint32_t *)(uintptr_t)thunk;
    w[0] = orig;
    w[1] = 0xE59FC014;   /* ldr r12, [pc, #20] -> w[8] */
    w[2] = 0xE58C0000;
    w[3] = 0xE58C1004;
    w[4] = 0xE58C2008;
    w[5] = 0xE58C300C;
    w[6] = 0xE59FC004;   /* ldr r12, [pc, #4]  -> w[9] */
    w[7] = 0xE12FFF1C;
    w[8] = scratch;
    w[9] = ret;
}

static void register_entry_branches(void) {
    for (unsigned i = 0; i < sizeof(s_entry_defs) / sizeof(s_entry_defs[0]); i++)
        macho_add_branch(s_entry_defs[i].vmaddr, s_hook_code + s_entry_defs[i].slot);
}

static void finish_entry_hooks(uint32_t delta) {
    if (!s_hook_code) return;
    for (unsigned i = 0; i < sizeof(s_entry_defs) / sizeof(s_entry_defs[0]); i++) {
        build_entry_thunk(s_hook_code + s_entry_defs[i].slot, s_entry_defs[i].orig,
                          s_hook_data + s_entry_defs[i].scratch,
                          s_entry_defs[i].vmaddr + 4 + delta);
    }
}

static void dump_entry_hooks(void) {
    for (unsigned i = 0; i < sizeof(s_entry_defs) / sizeof(s_entry_defs[0]); i++) {
        const volatile uint32_t *s =
            (const volatile uint32_t *)(uintptr_t)(s_hook_data + s_entry_defs[i].scratch);
        l_info("[entry] %s: r0=0x%08x r1=0x%08x r2=0x%08x r3=0x%08x",
               s_entry_defs[i].name, s[0], s[1], s[2], s[3]);
    }
}

static void build_call_thunk(uint32_t thunk, uint32_t target, uint32_t scratch, uint32_t ret) {
    volatile uint32_t *w = (volatile uint32_t *)(uintptr_t)thunk;
    int32_t off = (int32_t)(target - (thunk + 8 + 8));
    w[0] = 0xE92D400F;   /* push {r0-r3, lr} */
    w[1] = 0xE24DD004;   /* sub sp, sp, #4  (8-byte align) */
    w[2] = 0xEB000000u | (((uint32_t)(off >> 2)) & 0x00FFFFFFu); /* bl target */
    w[3] = 0xE59FC030;   /* ldr r12, [pc, #0x30] -> w[17] */
    w[4] = 0xE58C0000;   /* str r0, [r12]      (return value) */
    w[5] = 0xE59D0004;   /* ldr r0, [sp, #4]   (arg0) */
    w[6] = 0xE58C0004;   /* str r0, [r12, #4] */
    w[7] = 0xE59D0008;
    w[8] = 0xE58C0008;
    w[9] = 0xE59D000C;
    w[10] = 0xE58C000C;
    w[11] = 0xE59D0010;
    w[12] = 0xE58C0010;
    w[13] = 0xE59C0000;  /* ldr r0, [r12]      (restore return value) */
    w[14] = 0xE59FE008;  /* ldr lr, [pc, #8]   -> w[18] */
    w[15] = 0xE28DD018;  /* add sp, sp, #24 */
    w[16] = 0xE12FFF1E;  /* bx lr */
    w[17] = scratch;
    w[18] = ret;
}

static int install_hook_page(void) {
    uint32_t bases[] = { 0x98740000u, 0x98760000u, 0x98800000u, 0x98600000u };
    for (unsigned i = 0; i < sizeof(bases) / sizeof(bases[0]); i++) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = 0x1;
        opt.field_C = bases[i];
        SceUID id = kuKernelAllocMemBlock("redeg_hooks", 0x0C20D060, 0x2000, &opt);
        void *base = NULL;
        if (id >= 0) sceKernelGetMemBlockBase(id, &base);
        if (id >= 0 && base) {
            s_hook_code = (uint32_t)(uintptr_t)base;
            s_hook_data = s_hook_code + 0x1000;
            l_info("hook page at 0x%08x (data 0x%08x)", s_hook_code, s_hook_data);
            return 0;
        }
    }
    l_warn("hook page allocation failed");
    return -1;
}

static void register_hook_branches(void) {
    for (unsigned i = 0; i < sizeof(s_hook_defs) / sizeof(s_hook_defs[0]); i++)
        macho_add_branch(s_hook_defs[i].site, s_hook_code + s_hook_defs[i].slot);
}

static void finish_hooks(uint32_t delta) {
    if (!s_hook_code) return;
    for (unsigned i = 0; i < sizeof(s_hook_defs) / sizeof(s_hook_defs[0]); i++) {
        build_call_thunk(s_hook_code + s_hook_defs[i].slot,
                         s_hook_defs[i].target + delta,
                         s_hook_data + s_hook_defs[i].scratch,
                         s_hook_defs[i].site + 4 + delta);
    }
    l_info("guest call thunks built");
}

static void finalize_hooks(void) {
    if (!s_hook_code) return;
    kuKernelMemProtect((void *)(uintptr_t)s_hook_code, 0x1000,
                       KU_KERNEL_PROT_READ | KU_KERNEL_PROT_EXEC);
    kuKernelFlushCaches((const void *)(uintptr_t)s_hook_code, 0x2000);
    s_hook_ready = 1;
    l_info("guest hooks installed");
}

void redeg_dump_hooks(void) {
    if (!s_hook_ready) return;
    dump_entry_hooks();
    for (unsigned i = 0; i < sizeof(s_hook_defs) / sizeof(s_hook_defs[0]); i++) {
        const volatile uint32_t *s =
            (const volatile uint32_t *)(uintptr_t)(s_hook_data + s_hook_defs[i].scratch);
        l_info("[hook] %s: ret=0x%08x a0=0x%08x a1=0x%08x a2=0x%08x a3=0x%08x",
               s_hook_defs[i].name, s[0], s[1], s[2], s[3], s[4]);
        if ((i == 0 || i == 4) && s[1] >= 0x98000000u && s[1] < 0x99000000u)
            l_info("[hook]   str='%s'", (const char *)(uintptr_t)s[1]);
        if (i == 2 && s[2] >= 0x98000000u && s[2] < 0x99000000u)
            l_info("[hook]   path='%s'", (const char *)(uintptr_t)s[2]);
        if (i == 3 && s[1] >= 0x98000000u && s[1] < 0x99000000u)
            l_info("[hook]   path='%s'", (const char *)(uintptr_t)s[1]);
    }
}

static void init_platform(void) {
    sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_OLED_OFF);
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);
    sceKernelChangeThreadPriority(0, 127);
    sceKernelChangeThreadCpuAffinityMask(0, SCE_KERNEL_CPU_MASK_USER_2);
    sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    SceUID stream_mod = sceKernelLoadStartModule("ur0:tai/vita_stream.suprx", 0, NULL, 0, NULL, NULL);
    l_info("[stream] load ur0:tai/vita_stream.suprx -> 0x%08x", stream_mod);
}

void redeg_try_stack_guard(void) {
    static int tried = 0;
    if (tried) return;
    tried = 1;
    /* The engine's line reader uses the guest thread's stack top as an
     * unbounded destination buffer. Map a fixed 16 KB guard page directly
     * above it (guest stacks are consistently at 0x91b01000..0x91f01000). */
    uint32_t bases[] = { 0x91f01000u, 0x91f00000u, 0x91f02000u };
    for (unsigned i = 0; i < sizeof(bases) / sizeof(bases[0]); i++) {
        SceKernelAllocMemBlockKernelOpt opt;
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = 0x1;
        opt.field_C = bases[i];
        SceUID id = kuKernelAllocMemBlock("redeg_stack_guard", 0x0C20D060, 0x4000, &opt);
        void *base = NULL;
        if (id >= 0) sceKernelGetMemBlockBase(id, &base);
        l_info("[stack] guard try 0x%08x -> id=0x%x base=%p", bases[i], id, base);
        if (id >= 0 && base) return;
    }
    l_warn("[stack] no guard page could be mapped");
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    extern SceUID redeg_log_tid;
    redeg_log_tid = sceKernelGetThreadId();

    init_platform();
    probe_low_memory();

    l_info("==============================================");
    l_info(" Resident Evil: Degeneration - PS Vita loader");
    l_info(" Port by " PORT_AUTHOR " (v" PORT_VERSION ")");
    l_info(" " PORT_COPYRIGHT);
    l_info(" Native iOS (ARMv6) Mach-O + Marmalade/S3E");
    l_info(" Build: " __DATE__ " " __TIME__);
    l_info("==============================================");

    {
        pthread_t th;
        int tr = pthread_create(&th, NULL, selftest_thread, NULL);
        l_info("[selftest] pthread_create(NULL attr) -> %d", tr);
        pthread_t hb;
        l_info("[selftest] heartbeat -> %d", pthread_create(&hb, NULL, heartbeat_thread, NULL));
    }

    /* Self-test of the I/O path used by the logger. */
    {
        SceUID lf = sceIoOpen(DATA_PATH "logs/probe.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (lf >= 0) {
            sceIoWrite(lf, "PROBE", 5);
            sceIoSyncByFd(lf, 0);
            sceIoClose(lf);
        }
        SceUID t = sceIoOpen(DATA_PATH "selftest.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (t >= 0) {
            char msg[128];
            int len = sceClibSnprintf(msg, sizeof(msg), "HELLO-WORLD logfd=%d\n", lf);
            sceIoWrite(t, msg, len);
            sceIoSyncByFd(t, 0);
            sceIoClose(t);
        }
    }

    sceIoMkdir(DATA_PATH "logs", 0777);
    sceIoMkdir(DATA_PATH "shader_cache", 0777);
    sceIoMkdir(DATA_PATH "game", 0777);
    sceIoMkdir(DATA_PATH "Documents", 0777);

    setenv("HOME", DATA_PATH "game", 1);
    setenv("TMPDIR", DATA_PATH "tmp", 1);
    sceIoMkdir(DATA_PATH "tmp", 0777);
    sceIoMkdir(DATA_PATH "game/Documents", 0777);

    if (chdir(DATA_PATH "game") != 0)
        l_warn("chdir(%sgame) failed", DATA_PATH);
    else
        l_info("cwd: %sgame", DATA_PATH);

    objc_runtime_init();
    objc_foundation_init();
    objc_uikit_init();
    objc_egl_init();
    objc_misc_classes_init();
    l_success("ObjC runtime + framework classes ready");

    gl_preload();
    gl_init();

    if (0 && install_hook_page() == 0) { register_hook_branches(); register_entry_branches(); }

    /* Diagnostic: make s3eFileSetRoot a no-op so the engine cannot
     * prepend a bogus root to the drive prefixes. */
    macho_add_patch(0x19580, 0xE12FFF1E); /* bx lr */
    /* s3eStackSwitchReadConfig resets the loader stack to 0; keep our
     * pre-installed fibre stack instead. */
    macho_add_patch(0x29188, 0xE1A00000); /* nop: str r3,[r2] */
    /* Keep _s3eStackSwitchCallS3EFuncR12 on the current thread stack,
     * aligned down to 8 bytes (bic sp, r4, #7) so that:
     *   1) 5+ argument functions (glTexImage2D, glOrthox, glFrustumx, etc.)
     *      still have [r4+0x28..0x44] copied to [sp+0..0x1c], and
     *   2) sp is 8-byte aligned on entry to vitaGL (required by NEON vld1.64 [sp:64]). */
    macho_add_patch(0x291c4, 0xE1A00000); /* nop: bl _s3eStackSwitchIsSwapDisabled */
    macho_add_patch(0x291c8, 0xE1A00000); /* nop: cmp r0, #0 */
    macho_add_patch(0x291cc, 0xE1A00000); /* nop: bne noStackSwitch */
    macho_add_patch(0x291dc, 0xE3C4D007); /* bic sp, r4, #7 (instead of ldr sp, [r0]) */
    macho_add_patch(0x291e4, 0xE1A00000); /* nop: bl _s3eStackSwitchEnterLoaderStack */
    macho_add_patch(0x291e8, 0xE1A00000); /* nop: bl _s3eStackSwitchGetGameStack */
    macho_add_patch(0x291f4, 0xE1A00000); /* nop: bl _s3eStackSwitchSetGameStack */
    macho_add_patch(0x2925c, 0xE1A00000); /* nop: bl _s3eStackSwitchSetGameStack */
    macho_add_patch(0x29260, 0xE1A00000); /* nop: bl _s3eStackSwitchExitLoaderStack */
    /* Also keep _s3eStackSwitchCallS3EFuncR12_4Args_Fast_{Void,Int32Ret,Int64Ret}
     * on the current thread's kernel stack (8-byte aligned) instead of switching SP
     * to [0x6fed8c]. PS Vita's kernel validates that SP lies within the current
     * SceKernelThreadInfo stack bounds during syscalls/interrupts and aborts with
     * stopReason 0x00010006 (stack overflow) if SP points to a separate memblock. */
    macho_add_patch(0x292d8, 0xE92D0200); /* Fast_Void:     stmdb sp!, {sb} (at [r4, #-4]) */
    macho_add_patch(0x292dc, 0xE3CDD007); /* Fast_Void:     bic sp, sp, #7 */
    macho_add_patch(0x292ec, 0xE5149004); /* Fast_Void:     ldr sb, [r4, #-4] */
    macho_add_patch(0x29324, 0xE92D0200); /* Fast_Int32Ret: stmdb sp!, {sb} (at [r4, #-4]) */
    macho_add_patch(0x29328, 0xE3CDD007); /* Fast_Int32Ret: bic sp, sp, #7 */
    macho_add_patch(0x29338, 0xE5149004); /* Fast_Int32Ret: ldr sb, [r4, #-4] */
    macho_add_patch(0x29374, 0xE92D0200); /* Fast_Int64Ret: stmdb sp!, {sb} (at [r4, #-4]) */
    macho_add_patch(0x29378, 0xE3CDD007); /* Fast_Int64Ret: bic sp, sp, #7 */
    macho_add_patch(0x29388, 0xE5149004); /* Fast_Int64Ret: ldr sb, [r4, #-4] */
    /* IsEnabled() -> 0 and IsSwapDisabled() -> 2, so the CallCallback
     * entry point also stays on the current (pthread) stack. */
    macho_add_patch(0x2912c, 0xE3A00000); /* mov r0, #0 */
    macho_add_patch(0x29130, 0xE12FFF1E); /* bx lr */
    macho_add_patch(0x290ec, 0xE3A00002); /* mov r0, #2 */
    macho_add_patch(0x290f0, 0xE12FFF1E); /* bx lr */
    /* Force native landscape (480x320 logical -> 960x544 physical) with 0 deg rotation (BLIT_DIR_NORMAL):
     * - _s3eSurfaceSetRotation (0x0002af08): no-op (bx lr) so m_DeviceBlitDirection stays 0
     * - _s3eSurfaceReadConfig_platform (0x0021e108): pass 0 instead of 1 to _s3eSurfaceSetRotation
     * - _s3eSurfaceCheckRotation (0x0021dd6c): no-op (bx lr) so UIDevice orientation never rotates surface
     * - _s3eDeviceGetUserOrientation (0x0001387c): return 0 (mov r0, #0; bx lr)
     * - IwGxSetScreenOrient internal helper (0x0005e1f4): force orient = 0 (mov r4, #0) so IwGx
     *   keeps m_ScreenOrient = 0, m_ScreenWidth = 480, m_ScreenHeight = 320, and does not apply
     *   a -90 deg glRotatex on GL_PROJECTION. */
    macho_add_patch(0x02af08, 0xE12FFF1E); /* _s3eSurfaceSetRotation: bx lr */
    macho_add_patch(0x21e108, 0xE3A00000); /* _s3eSurfaceReadConfig_platform: mov r0, #0 */
    macho_add_patch(0x21dd6c, 0xE12FFF1E); /* _s3eSurfaceCheckRotation: bx lr */
    macho_add_patch(0x01387c, 0xE3A00000); /* _s3eDeviceGetUserOrientation: mov r0, #0 */
    macho_add_patch(0x013880, 0xE12FFF1E); /* _s3eDeviceGetUserOrientation: bx lr */
    macho_add_patch(0x05e1f4, 0xE3A04000); /* IwGx orient setter: mov r4, #0 */
    /* Complete _s3eVideoPlay (0x0002d02c) immediately via its built-in stop notification
     * path (0x0002d180: mov r4, #0; _s3eCallbacksNotify(8, 2, ...); clear g_s3eVideoGlobals[0])
     * instead of launching iOS MPMoviePlayerController on intro.mp4 and waiting forever on _g_Active. */
    macho_add_patch(0x02d108, 0xEA00001C); /* _s3eVideoPlay @ 0x0002d108: b 0x0002d180 */
    /* Keep both Touchscreen UI/HUD buttons (Action 0 = 0xc9, Action 1 = 0xc8) AND physical
     * controls (Actions 2..12) active simultaneously in CKeyBinder::initDefaultLayout (0x0008e668):
     * - @ 0x0008e6d4: after storing 0xc9 (+0x00) and 0xc8 (+0x04), branch to 0x0008e6f2 (b 0x8e6f2; nop)
     *   instead of zeroing Actions 2..11 (+0x08..+0x2c).
     * - @ 0x0008e72c: nop out beq 0x8e772 so Action 12 (+0x30) is always set to 0x4f (79). */
    macho_add_patch(0x08e6d4, 0x46C0E00D); /* CKeyBinder::initDefaultLayout: b 0x8e6f2; nop */
    macho_add_patch(0x08e72c, 0x204F46C0); /* CKeyBinder::initDefaultLayout: nop; movs r0, 0x4f */
    /* Bypass IwGx pointer-only equality cache on gl*Pointer wrappers (0x000ba1e4..0x000ba26e):
     * IwGx caches only the pointer address in _g_IwGxState without checking type/size/stride,
     * skipping glVertexPointer/glNormalPointer/glColorPointer/glTexCoordPointer when a scratch
     * buffer address is reused with a different type (e.g. GL_SHORT vs GL_FIXED). */
    macho_add_patch(0x0ba1f0, 0x462346C0); /* IwGx glColorPointer wrapper: nop pointer-equality skip */
    macho_add_patch(0x0ba220, 0x462246C0); /* IwGx glNormalPointer wrapper: nop pointer-equality skip */
    macho_add_patch(0x0ba23c, 0x462346C0); /* IwGx glVertexPointer wrapper: nop pointer-equality skip */
    macho_add_patch(0x0ba25c, 0x46C042A3); /* IwGx glTexCoordPointer wrapper: nop pointer-equality skip */
    /* Preserve native matrix updates and 16-bit IwAnim skinning. The
     * mesh explosion came from rebasing the numeric sine table. */

    macho_image_t img;
    if (macho_load(DATA_PATH "game/residentevil", &img) < 0) {
        l_fatal("Mach-O load failed");
        sceKernelExitProcess(0);
    }

    finish_hooks(img.delta);
    finish_entry_hooks(img.delta);
    finalize_hooks();

    /* Opt-in loader evidence, captured before the guest mutates its image. */
    FILE *graphics_probe = fopen(DATA_PATH "graphics_probe", "rb");
    if (graphics_probe) {
        fclose(graphics_probe);
        FILE *dump = fopen(DATA_PATH "logs/loaded-image.bin", "wb");
        if (dump) {
            size_t n = fwrite((const void *)(uintptr_t)img.load_base, 1, img.image_size, dump);
            fclose(dump);
            l_info("[graphics-probe] loaded image base=%08x delta=%08x bytes=%u/%u",
                   img.load_base, img.delta, (unsigned)n, img.image_size);
        }
    }

    /* Diagnostic: keep the whole image RWX. Some Marmalade runtime paths
     * write temporary data into __TEXT-adjacent pages; RX there showed up
     * as illegal-address aborts inside pure register loops. */
    {
        int r = kuKernelMemProtect((void *)(uintptr_t)img.load_base, img.image_size,
                                   KU_KERNEL_PROT_READ | KU_KERNEL_PROT_WRITE | KU_KERNEL_PROT_EXEC);
        l_info("[mm] image RWX -> %d", r);
    }

    redeg_try_stack_guard();

    /* Give the s3e stack switcher a real loader stack. The engine expects
     * s3eStackSwitchReadConfig() to leave this at the loader's SP, but in
     * this build it resets it to 0; the first s3eFileRead then switches to
     * SP=0 and takes an illegal-address abort. */
    {
        SceUID sid = sceKernelAllocMemBlock("redeg_fibre",
                                            SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
                                            0x100000, NULL);
        void *sb = NULL;
        if (sid >= 0 && sceKernelGetMemBlockBase(sid, &sb) >= 0 && sb) {
            /* _s3eStackSwitchCallS3EFuncR12_4Args_Fast_* loads sp = [0x6fed8c]
             * and then executes `stmdb sp!, {sb}` (4 bytes) before `bx ip`.
             * Set top to (8-byte aligned - 4) so that SP is 8-byte aligned
             * upon entering the target function (required by NEON vld1.64 [sp:64]). */
            uint32_t top = (((uint32_t)(uintptr_t)sb + 0x100000 - 16) & ~7u) - 4u;
            *(uint32_t *)(uintptr_t)(0x6fed8c + img.delta) = top;
            *(uint32_t *)(uintptr_t)(0x6fed90 + img.delta) = img.load_base + 0x232000;
            l_info("[fibre] loader stack 0x%08x..0x%08x -> [0x%08x]",
                   (uint32_t)(uintptr_t)sb, top, 0x6fed8c + img.delta);
        } else {
            l_warn("[fibre] stack alloc failed (0x%x)", sid);
        }
    }

    {
        uint32_t *q = (uint32_t *)(uintptr_t)(0x460f0 + img.delta);
        l_info("code@460f0: %08x %08x %08x %08x [%08x %08x %08x %08x]",
               q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7]);
    }

    /* scan for bad ptr: find where 0x9891711a / 0x91811a live in the image */
    {
        uint32_t *base = (uint32_t *)(uintptr_t)img.load_base;
        uint32_t n = img.image_size / 4;
        int found = 0;
        for (uint32_t i = 0; i < n; i++) {
            if (base[i] == 0x9891711Au || base[i] == 0x91811Au || base[i] == 0x0091811Au) {
                l_warn("scan for bad ptr: vm 0x%08x = 0x%08x",
                       0x1000 + i * 4, base[i]);
                found++;
            }
        }
        l_info("scan for bad ptr: %d hit(s)", found);
    }

    objc_register_binary_classes(img.classlist, img.nclasses);

    /* The engine's first config read runs before it installs the drive
     * prefixes (its init calls s3eFileReadConfig with arg 0, skipping the
     * prefix setup), so relative files like s3e.icf resolve to nothing.
     * Initialize s3eFile ourselves first: s3eFileInit is a one-shot memset
     * guarded by a bitmask, so the engine's later call becomes a no-op and
     * our prefixes survive. Do NOT call s3eFileSetRoot: its root is appended
     * after the drive prefix and would corrupt absolute Vita paths. */
    {
        void (*s3e_file_init)(void) =
            (void (*)(void))(uintptr_t)(0x17f10 + img.delta);
        void (*s3e_file_set_prefix)(const char *, const char *) =
            (void (*)(const char *, const char *))(uintptr_t)(0x182a4 + img.delta);
        s3e_file_init();
        s3e_file_set_prefix("rom://", DATA_PATH "game/");
        s3e_file_set_prefix("ram://", DATA_PATH "game/Documents/");
        s3e_file_set_prefix("rst://", DATA_PATH "game/");
        s3e_file_set_prefix("raw://", DATA_PATH "game/");
        l_info("s3eFileInit + drive prefixes -> %sgame/", DATA_PATH);
        uint32_t *gg = (uint32_t *)(uintptr_t)(0x6fb608 + img.delta);
        l_info("prefix after set: 0x%08x 0x%08x 0x%08x 0x%08x", gg[0], gg[1], gg[2], gg[3]);
    }

    uint32_t entry = img.entry + img.delta;
    g_got_base = img.load_base + 0x232000;   /* vmaddr 0x233000 (GOT/nl_symbol_ptr) */
    l_info("GOT base (r9) = 0x%08x", g_got_base);
    l_success("calling main @ 0x%08x (orig 0x%08x)", entry, img.entry);

    static char *game_argv[] = { "residentevil", NULL };
    int ret = redeg_call_game_main(entry, 1, game_argv, g_got_base);

    l_error("main returned %d", ret);
    sceKernelDelayThread(2 * 1000 * 1000);
    sceKernelExitProcess(0);
    return 0;
}
