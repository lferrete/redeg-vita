#include "utils/logger.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LOG_DIR DATA_PATH "logs"
#define LOG_MAX 999

static bool s_tried = false;
static char s_path[128];
static char s_line[1600];
/* Guest-created pthreads call our shims and thus this logger. SceLibKernel
 * I/O from those threads has been aborting at random points, so only the
 * main/UI thread writes log lines. */
SceUID redeg_log_tid = -1;
/* Keep the big output buffer out of the caller's stack: the engine calls
 * our shims from Marmalade fibre stacks that are only a few KB deep, and
 * a 1.7 KB local plus snprintf's own frame overflowed them. */
static char s_out[1700];

/* Guest-thread log ring. Formatting is safe on the guest thread; actual
 * file I/O (sceIoOpen) is not (the eboot data segment is slid +0x40000 at
 * runtime and r9 is the game GOT). The main thread drains this ring. */
static char s_guest_line[1600];
static char s_gring[64][200];
static volatile uint32_t s_gwrite = 0, s_gread = 0;

void redeg_guest_log_drain(void) {
    while (s_gread != s_gwrite) {
        uint32_t i = s_gread % 64;
        l_info("[guest] %s", s_gring[i]);
        s_gread++;
    }
}


static int log_next_index(void) {
    char p[64];
    snprintf(p, sizeof(p), LOG_DIR "/next.idx");
    SceUID fd = sceIoOpen(p, SCE_O_RDONLY, 0777);
    if (fd < 0) return 1;
    char b[16] = {0};
    int n = sceIoRead(fd, b, sizeof(b) - 1);
    sceIoClose(fd);
    if (n <= 0) return 1;
    int v = atoi(b);
    if (v < 1 || v > LOG_MAX) v = 1;
    return v;
}

static void log_pick_path(void) {
    if (s_tried) return;
    s_tried = true;
    sceIoMkdir(LOG_DIR, 0777);
    int start = log_next_index();
    int idx = start;
    for (int n = 0; n < LOG_MAX; ++n) {
        idx = start + n;
        if (idx > LOG_MAX) idx -= LOG_MAX;
        snprintf(s_path, sizeof(s_path), LOG_DIR "/redeg_%03d.log", idx);
        SceIoStat st;
        if (sceIoGetstat(s_path, &st) < 0) break;
    }
    char p[64];
    snprintf(p, sizeof(p), LOG_DIR "/next.idx");
    SceUID nf = sceIoOpen(p, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (nf >= 0) {
        char b[16];
        int l = snprintf(b, sizeof(b), "%d\n", idx >= LOG_MAX ? 1 : idx + 1);
        sceIoWrite(nf, b, (unsigned)l);
        sceIoClose(nf);
    }
}

const char *log_get_path(void) {
    return s_path;
}

void _log_print(int level, const char *fmt, ...) {
    /* SceLibKernel I/O from guest-created pthreads aborts (the eboot's data
     * segment is slid by 0x40000 at runtime and the guest r9 is the game's
     * GOT). Guest lines are formatted into a memory ring and drained to the
     * file by the main/UI thread. */
    if (redeg_log_tid != -1 && sceKernelGetThreadId() != redeg_log_tid) {
        va_list gap;
        va_start(gap, fmt);
        vsnprintf(s_guest_line, sizeof(s_guest_line), fmt, gap);
        va_end(gap);
        uint32_t i = s_gwrite % 64;
        strncpy(s_gring[i], s_guest_line, sizeof(s_gring[0]) - 1);
        s_gring[i][sizeof(s_gring[0]) - 1] = 0;
        __sync_synchronize();
        s_gwrite++;
        return;
    }
    log_pick_path();

    const char *tag = "INFO";
    switch (level) {
        case LT_DEBUG: tag = "DEBUG"; break;
        case LT_WARN: tag = "WARN"; break;
        case LT_ERROR: tag = "ERROR"; break;
        case LT_FATAL: tag = "FATAL"; break;
        case LT_SUCCESS: tag = "OK"; break;
        case LT_WAIT: tag = "WAIT"; break;
        default: break;
    }

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(s_line, sizeof(s_line), fmt, ap);
    va_end(ap);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof(s_line)) n = (int)sizeof(s_line) - 1;
    while (n > 0 && (s_line[n - 1] == '\n' || s_line[n - 1] == '\r')) s_line[--n] = 0;

    int l = snprintf(s_out, sizeof(s_out), "[%s] %s\n", tag, s_line);
    if (l > 0) {
        SceUID fd = sceIoOpen(s_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
        if (fd >= 0) {
            sceIoWrite(fd, s_out, (unsigned)l);
            sceIoSyncByFd(fd, 0);
            sceIoClose(fd);
        }
    }
}
