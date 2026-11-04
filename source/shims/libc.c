#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/kernel/clib.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utils/logger.h"

/* ------------------------------------------------------------------ */
/* Darwin-compatible structs (iPhone OS 2.x, 32-bit)                   */
/* ------------------------------------------------------------------ */

typedef struct {
    int32_t tv_sec;
    int32_t tv_nsec;
} darwin_timespec_t;

typedef struct {
    int32_t st_dev;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint64_t st_ino;
    uint32_t st_uid;
    uint32_t st_gid;
    int32_t st_rdev;
    darwin_timespec_t st_atimespec;
    darwin_timespec_t st_mtimespec;
    darwin_timespec_t st_ctimespec;
    darwin_timespec_t st_birthtimespec;
    int64_t st_size;
    int64_t st_blocks;
    int32_t st_blksize;
    uint32_t st_flags;
    uint32_t st_gen;
    int32_t st_lspare;
    int64_t st_qspare[2];
} __attribute__((packed)) darwin_stat_t;

typedef struct {
    uint64_t f_bsize;
    uint64_t f_frsize;
    uint64_t f_blocks;
    uint64_t f_bfree;
    uint64_t f_bavail;
    uint64_t f_files;
    uint64_t f_ffree;
    uint64_t f_favail;
    uint64_t f_fsid;
    uint64_t f_flag;
    uint64_t f_namemax;
} darwin_statvfs64_t;

/* iPhone OS 2.x statvfs is the 32-bit variant: the game reads
 * [4]=f_frsize and [8]=f_blocks (64-bit). */
typedef struct {
    uint32_t f_bsize;
    uint32_t f_frsize;
    uint64_t f_blocks;
    uint64_t f_bfree;
    uint64_t f_bavail;
    uint64_t f_files;
    uint64_t f_ffree;
    uint64_t f_favail;
    uint64_t f_fsid;
    uint32_t f_flag;
    uint32_t f_namemax;
} darwin_statvfs_t;

typedef struct {
    uint64_t d_ino;
    uint64_t d_seekoff;
    uint16_t d_reclen;
    uint16_t d_namlen;
    uint8_t d_type;
    char d_name[1024];
} darwin_dirent_t;

/* ------------------------------------------------------------------ */
/* stat / statvfs / lseek / truncate                                  */
/* ------------------------------------------------------------------ */

int redeg_stat(const char *path, darwin_stat_t *out) {
    l_info("[fs] stat(%s)", path ? path : "(null)");
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    memset(out, 0, sizeof(*out));
    out->st_dev = (int32_t)st.st_dev;
    out->st_mode = (uint16_t)st.st_mode;
    out->st_nlink = (uint16_t)st.st_nlink;
    out->st_ino = (uint64_t)st.st_ino;
    out->st_uid = (uint32_t)st.st_uid;
    out->st_gid = (uint32_t)st.st_gid;
    out->st_rdev = (int32_t)st.st_rdev;
    out->st_atimespec.tv_sec = (int32_t)st.st_atime;
    out->st_mtimespec.tv_sec = (int32_t)st.st_mtime;
    out->st_ctimespec.tv_sec = (int32_t)st.st_ctime;
    out->st_birthtimespec = out->st_ctimespec;
    out->st_size = (int64_t)st.st_size;
    out->st_blocks = (int64_t)st.st_blocks;
    out->st_blksize = (int32_t)st.st_blksize;
    return 0;
}

int redeg_statvfs(const char *path, darwin_statvfs_t *out) {
    (void)path;
    memset(out, 0, sizeof(*out));
    SceIoStat st;
    if (sceIoGetstat("ux0:", &st) < 0) {
        out->f_bsize = 32768;
        out->f_frsize = 32768;
        out->f_blocks = 0;
        return 0;
    }
    uint64_t freeb = 0, total = 0;
    /* sceIoDevctl not used; report a plausible non-zero filesystem. */
    out->f_bsize = 32768;
    out->f_frsize = 32768;
    out->f_blocks = total / 32768;
    out->f_bfree = out->f_bavail = freeb / 32768;
    out->f_namemax = 255;
    return 0;
}

/* Darwin APCS-gnu passes 64-bit off_t/int64_t 4-byte aligned (so the 2nd arg
 * goes in r1:r2 and the 3rd arg in r3), whereas ARM EABI (AAPCS) 8-byte aligns
 * int64_t into an even-odd register pair (r2:r3) and pushes the 3rd arg to [sp].
 * Always split 64-bit parameters at odd word positions into (lo, hi) 32-bit words!
 * Also use newlib lseek() because redeg_open/redeg_read return/use newlib POSIX fds. */
int64_t redeg_lseek(int fd, uint32_t offset_lo, int32_t offset_hi, int whence) {
    int64_t offset = ((int64_t)offset_hi << 32) | (uint64_t)offset_lo;
    off_t r = lseek(fd, (off_t)offset, whence);
    if (r == (off_t)-1) return -1;
    return (int64_t)r;
}

int redeg_truncate(const char *path, uint32_t length_lo, int32_t length_hi) {
    int64_t length = ((int64_t)length_hi << 32) | (uint64_t)length_lo;
    return truncate(path, (off_t)length);
}

/* ------------------------------------------------------------------ */
/* dirent                                                             */
/* ------------------------------------------------------------------ */

static darwin_dirent_t s_dent[8];
static int s_dent_idx = 0;

void *redeg_opendir(const char *path) {
    l_info("[fs] opendir(%s)", path ? path : "(null)");
    return (void *)opendir(path);
}

darwin_dirent_t *redeg_readdir(void *dir) {
    struct dirent *e = readdir((DIR *)dir);
    static int dirlog = 0;
    if (dirlog++ < 40) l_info("[fs] readdir -> %s", e ? e->d_name : "(null)");
    if (!e) return NULL;
    darwin_dirent_t *d = &s_dent[s_dent_idx++ & 7];
    memset(d, 0, sizeof(*d));
    d->d_ino = 0;
    d->d_seekoff = 0;
    size_t n = strlen(e->d_name);
    if (n > 1023) n = 1023;
    d->d_namlen = (uint16_t)n;
    d->d_reclen = (uint16_t)(21 + n + 1);
    d->d_type = 8;
    memcpy(d->d_name, e->d_name, n + 1);
    return d;
}

int redeg_closedir(void *dir) {
    return closedir((DIR *)dir);
}

/* ------------------------------------------------------------------ */
/* misc libc                                                          */
/* ------------------------------------------------------------------ */

int *redeg_error(void) { return &errno; }

void redeg_bzero(void *p, size_t n) { memset(p, 0, n); }

int redeg_sigaction(int sig, const void *act, void *old) {
    (void)sig; (void)act; (void)old;
    return 0;
}

void *redeg_dlopen(const char *path, int mode) { (void)path; (void)mode; return NULL; }
void *redeg_dlsym(void *h, const char *name) { (void)h; (void)name; return NULL; }
int redeg_dlclose(void *h) { (void)h; return 0; }
char *redeg_dlerror(void) { return "dlopen not supported"; }

int redeg_select(int nfds, void *r, void *w, void *e, void *timeout) {
    (void)nfds; (void)r; (void)w; (void)e; (void)timeout;
    return 0;
}

int redeg_socket(int domain, int type, int protocol) { (void)domain; (void)type; (void)protocol; return -1; }
int redeg_bind(int fd, const void *addr, int len) { (void)fd; (void)addr; (void)len; return -1; }
int redeg_connect(int fd, const void *addr, int len) { (void)fd; (void)addr; (void)len; return -1; }
int redeg_listen(int fd, int backlog) { (void)fd; (void)backlog; return -1; }
int redeg_accept(int fd, void *addr, void *len) { (void)fd; (void)addr; (void)len; return -1; }
long redeg_send(int fd, const void *buf, size_t len, int flags) { (void)fd; (void)buf; (void)len; (void)flags; return -1; }
long redeg_recv(int fd, void *buf, size_t len, int flags) { (void)fd; (void)buf; (void)len; (void)flags; return -1; }
long redeg_sendto(int fd, const void *buf, size_t len, int flags, const void *addr, int alen) { (void)fd; (void)buf; (void)len; (void)flags; (void)addr; (void)alen; return -1; }
long redeg_recvfrom(int fd, void *buf, size_t len, int flags, void *addr, void *alen) { (void)fd; (void)buf; (void)len; (void)flags; (void)addr; (void)alen; return -1; }
int redeg_setsockopt(int fd, int level, int opt, const void *val, int len) { (void)fd; (void)level; (void)opt; (void)val; (void)len; return -1; }
int redeg_getsockname(int fd, void *addr, void *len) { (void)fd; (void)addr; (void)len; return -1; }
int redeg_getpeername(int fd, void *addr, void *len) { (void)fd; (void)addr; (void)len; return -1; }
int redeg_gethostname(char *name, size_t len) { if (name && len) { strncpy(name, "vita", len); } return 0; }
int redeg_getdomainname(char *name, size_t len) { if (name && len) name[0] = 0; return 0; }
void *redeg_gethostbyname(const char *name) { (void)name; return NULL; }
int redeg_inet_aton(const char *cp, void *inp) { (void)cp; (void)inp; return 0; }
char *redeg_inet_ntoa(void *in) { (void)in; return "0.0.0.0"; }
int redeg_fcntl(int fd, int cmd, ...) { (void)fd; (void)cmd; return 0; }
int redeg_chmod(const char *path, int mode) { (void)path; (void)mode; return 0; }

int redeg_backtrace(void **buffer, int size) { (void)buffer; (void)size; return 0; }
char **redeg_backtrace_symbols(void *const *buffer, int size) { (void)buffer; (void)size; return NULL; }

/* Mach semaphores used by the Marmalade threading layer. */
int redeg_semaphore_create(unsigned int task, int *sem, int policy, int value) {
    (void)task; (void)policy;
    char name[32];
    static int n = 0;
    sceClibSnprintf(name, sizeof(name), "redeg_sem_%d", n++);
    int id = sceKernelCreateSema(name, 0, value, 0x7fffffff, NULL);
    if (id < 0) return -1;
    *sem = id;
    return 0;
}
int redeg_semaphore_signal(int sem) {
    return sceKernelSignalSema(sem, 1) < 0 ? -1 : 0;
}
int redeg_semaphore_wait(int sem) {
    return sceKernelWaitSema(sem, 1, NULL) < 0 ? -1 : 0;
}

/* keymgr */
int redeg_keymgr_get(unsigned int key, void **ptr) { (void)key; *ptr = NULL; return 0; }
int redeg_keymgr_get2(unsigned int key, void **ptr) { (void)key; *ptr = NULL; return 0; }
int redeg_keymgr_set(unsigned int key, void *ptr) { (void)key; (void)ptr; return 0; }

static void thread_marker(const char *tag) {
    SceUID fd = sceIoOpen("ux0:data/redeg/thread.log", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0) { sceIoWrite(fd, tag, (unsigned)strlen(tag)); sceIoWrite(fd, "\n", 1); sceIoSyncByFd(fd, 0); sceIoClose(fd); }
}

extern uint32_t g_got_base;
typedef struct { void *(*fn)(void *); void *arg; uint32_t got; } redeg_thread_start_t;

__attribute__((naked)) static void *redeg_thread_entry(void *p) {
    __asm__ volatile(
        "ldr r12, [r0]\n"
        "ldr r1, [r0, #4]\n"
        "ldr r9, [r0, #8]\n"
        "mov r0, r1\n"
        /* Leave a guard page above the guest's initial SP: some engine
         * routines put a local buffer at sp+0x240 and write one byte past
         * the stack top, which otherwise aborts with DFAR == stack_top. */
        "sub sp, sp, #4096\n"
        "bx r12\n");
}
static void *redeg_thread_trampoline(void *p) {
    redeg_thread_start_t s = *(redeg_thread_start_t *)p;
    free(p);
    thread_marker("thread entry");
    l_info("[pthread] thread entry fn=%p arg=%p", (void *)s.fn, s.arg);
    void *r = s.fn(s.arg);
    thread_marker("thread exit");
    l_info("[pthread] thread exit %p", r);
    return r;
}
int redeg_pthread_mutex_lock(pthread_mutex_t *m) {
    static int n = 0;
    if (n++ < 60) l_info("[sync] lock %p tid=%d", (void *)m, sceKernelGetThreadId());
    return pthread_mutex_lock(m);
}
int redeg_pthread_mutex_unlock(pthread_mutex_t *m) {
    static int n = 0;
    if (n++ < 60) l_info("[sync] unlock %p tid=%d", (void *)m, sceKernelGetThreadId());
    return pthread_mutex_unlock(m);
}
int redeg_pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m) {
    static int n = 0;
    if (n++ < 20) l_info("[sync] cond_wait %p %p", (void *)c, (void *)m);
    return pthread_cond_wait(c, m);
}
int redeg_pthread_cond_signal(pthread_cond_t *c) {
    static int n = 0;
    if (n++ < 20) l_info("[sync] cond_signal %p", (void *)c);
    return pthread_cond_signal(c);
}

int redeg_pthread_create(pthread_t *t, const pthread_attr_t *attr, void *(*fn)(void *), void *arg) {
    l_info("[pthread] create fn=%p arg=%p", (void *)fn, arg);
    redeg_thread_start_t *s = (redeg_thread_start_t *)malloc(sizeof(*s));
    s->fn = fn; s->arg = arg; s->got = g_got_base;
    (void)attr;
    /* The guest's Darwin attr is not layout-compatible with newlib's, and
     * the engine's threads are deep (C++ initializers + Marmalade). Give
     * every guest thread a generous stack instead of newlib's default.
     * Allocate one extra page above the reported stack top: a few guest
     * routines place a full-size local buffer ending exactly at SP and
     * write a terminator one byte past it; the guard page then faults
     * (DFAR == stack_top). The extra mapped page absorbs that byte. */
    pthread_attr_t local;
    pthread_attr_init(&local);
    pthread_attr_setstacksize(&local, 4 * 1024 * 1024);
    int r = pthread_create(t, &local, redeg_thread_entry, s);
    pthread_attr_destroy(&local);
    l_info("[pthread] create -> %d (4MB stack)", r);
    return r;
}

void *redeg_malloc(size_t n) {
    if (n >= 1024 * 1024) l_info("[malloc] request %u (0x%x)", (unsigned)n, (unsigned)n);
    void *p = malloc(n);
    if (n >= 1024 * 1024) l_info("[malloc] -> %p", p);
    return p;
}

void *redeg_calloc(size_t n, size_t sz) {
    size_t total = n * sz;
    if (total >= 1024 * 1024) l_info("[calloc] request %u (0x%x)", (unsigned)total, (unsigned)total);
    void *p = calloc(n, sz);
    if (total >= 1024 * 1024) l_info("[calloc] -> %p", p);
    return p;
}

void *redeg_realloc(void *old, size_t n) {
    if (n >= 1024 * 1024) l_info("[realloc] request %u (0x%x) old=%p", (unsigned)n, (unsigned)n, old);
    void *p = realloc(old, n);
    if (n >= 1024 * 1024) l_info("[realloc] -> %p", p);
    return p;
}

char *redeg_getenv(const char *name) {
    char *v = getenv(name);
    l_info("[env] %s = %s", name ? name : "(null)", v ? v : "(null)");
    return v;
}

volatile uint32_t g_io_read_bytes = 0;
volatile uint32_t g_io_read_calls = 0;
volatile uint32_t g_io_open_calls = 0;

size_t redeg_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t n = fread(ptr, size, nmemb, stream);
    g_io_read_bytes += (uint32_t)(n * size);
    g_io_read_calls++;
    return n;
}

ssize_t redeg_read(int fd, void *buf, size_t count) {
    ssize_t n = read(fd, buf, count);
    if (n > 0) g_io_read_bytes += (uint32_t)n;
    g_io_read_calls++;
    return n;
}

FILE *redeg_fopen(const char *path, const char *mode) {
    l_info("[fs] fopen(%s, %s)", path ? path : "(null)", mode ? mode : "(null)");
    return fopen(path, mode);
}

int redeg_open(const char *path, int flags, ...) {
    int mode = 0;
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    l_info("[fs] open(%s, 0x%x, 0%o)", path ? path : "(null)", flags, mode);
    return open(path, flags, mode);
}

void redeg_exit(int code) {
    l_info("[exit] code=%d from 0x%08x", code, (uint32_t)(uintptr_t)__builtin_return_address(0));
    for (;;) sceKernelDelayThread(1000 * 1000);
}

int redeg_printf(const char *fmt, ...) {
    static int count = 0;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (count++ < 200) {
        size_t n = strlen(buf);
        while (n && (buf[n-1] == 10 || buf[n-1] == 13)) buf[--n] = 0;
        l_info("[printf] %s", buf);
    }
    return 0;
}

uint32_t mach_task_self_ = 0;
uint8_t _objc_empty_cache[16];
uint8_t _objc_empty_vtable[16];
