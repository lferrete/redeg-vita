#include "objc/objc_classes.h"
#include "macho/macho.h"
#include "utils/logger.h"

#include <vitaGL.h>
#include <zlib.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

typedef struct {
    const char *name;
    void *addr;
} sym_entry_t;

extern void NSLog(id fmt, ...);
extern void *kCFAllocatorDefault, *kCFRunLoopCommonModes, *kCFRunLoopDefaultMode;

/* forward decls from libc.c */
int redeg_stat(const char *path, void *out);
int redeg_statvfs(const char *path, void *out);
int64_t redeg_lseek(int fd, uint32_t off_lo, int32_t off_hi, int whence);
int redeg_truncate(const char *path, uint32_t len_lo, int32_t len_hi);
void *redeg_opendir(const char *path);
void *redeg_readdir(void *dir);
int redeg_closedir(void *dir);
int *redeg_error(void);
void redeg_bzero(void *p, size_t n);
int redeg_sigaction(int sig, const void *act, void *old);
void *redeg_dlopen(const char *path, int mode);
void *redeg_dlsym(void *h, const char *name);
int redeg_dlclose(void *h);
char *redeg_dlerror(void);
int redeg_select(int nfds, void *r, void *w, void *e, void *timeout);
int redeg_socket(int d, int t, int p);
int redeg_bind(int fd, const void *a, int l);
int redeg_connect(int fd, const void *a, int l);
int redeg_listen(int fd, int b);
int redeg_accept(int fd, void *a, void *l);
long redeg_send(int fd, const void *b, size_t l, int f);
long redeg_recv(int fd, void *b, size_t l, int f);
long redeg_sendto(int fd, const void *b, size_t l, int f, const void *a, int al);
long redeg_recvfrom(int fd, void *b, size_t l, int f, void *a, void *al);
int redeg_setsockopt(int fd, int l, int o, const void *v, int len);
int redeg_getsockname(int fd, void *a, void *l);
int redeg_getpeername(int fd, void *a, void *l);
int redeg_gethostname(char *n, size_t l);
int redeg_getdomainname(char *n, size_t l);
void *redeg_gethostbyname(const char *n);
int redeg_inet_aton(const char *cp, void *inp);
char *redeg_inet_ntoa(void *in);
int redeg_fcntl(int fd, int cmd, ...);
int redeg_chmod(const char *p, int m);
int redeg_backtrace(void **b, int s);
char **redeg_backtrace_symbols(void *const *b, int s);
int redeg_semaphore_create(unsigned task, int *sem, int policy, int value);
int redeg_semaphore_signal(int sem);
int redeg_semaphore_wait(int sem);
int redeg_keymgr_get(unsigned key, void **ptr);
int redeg_pthread_create(pthread_t *t, const pthread_attr_t *attr, void *(*fn)(void *), void *arg);
int redeg_pthread_mutex_lock(pthread_mutex_t *m);
int redeg_pthread_mutex_unlock(pthread_mutex_t *m);
int redeg_pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m);
int redeg_pthread_cond_signal(pthread_cond_t *c);
int redeg_printf(const char *fmt, ...);
void *redeg_malloc(size_t n);
size_t redeg_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
ssize_t redeg_read(int fd, void *buf, size_t count);
void *redeg_calloc(size_t n, size_t sz);
void *redeg_realloc(void *old, size_t n);
void redeg_exit(int code);
char *redeg_getenv(const char *name);
void *redeg_fopen(const char *path, const char *mode);
int redeg_open(const char *path, int flags, ...);
int redeg_keymgr_get2(unsigned key, void **ptr);
int redeg_keymgr_set(unsigned key, void *ptr);

extern uint32_t mach_task_self_;
extern uint8_t _objc_empty_cache[16];
extern uint8_t _objc_empty_vtable[16];

/* misc.c */
Class g_CLLocationManager, g_CLLocation;
Class g_MPMoviePlayerController;
extern void *kCLErrorDomain;
extern void *MPMoviePlayerContentPreloadDidFinishNotification;
extern void *MPMoviePlayerPlaybackDidFinishNotification;

/* cf.c */
void *CFStringCreateWithCString(void *a, const char *s, uint32_t e);
int CFStringGetCString(void *s, char *b, long n, uint32_t e);
int CFStringCompare(void *a, void *b, uint32_t o);
void *CFDataCreate(void *a, const uint8_t *b, long n);
const uint8_t *CFDataGetBytePtr(void *d);
long CFDataGetLength(void *d);
void *CFArrayGetValueAtIndex(void *a, long i);
void *CFDictionaryGetValue(void *d, const void *k);
int CFNumberGetValue(void *n, int t, void *o);
void CFRelease(void *o);
void *CFRunLoopGetCurrent(void);
int CFRunLoopRunInMode(void *m, uint32_t s_lo, uint32_t s_hi, int r);
void CFRunLoopStop(void *r);
void *CFURLCreateFromFileSystemRepresentation(void *a, const uint8_t *b, long n, int d);
void CGAffineTransformMakeTranslation(void);
void *CGColorSpaceCreateDeviceRGB(void);
void CGColorSpaceRelease(void *c);
void *CGBitmapContextCreate(void *d, unsigned long w, unsigned long h, unsigned long bpc,
                            unsigned long stride, void *cs, uint32_t info);
void *CGBitmapContextCreateImage(void *c);
void CGContextDrawImage(void *c, void *rect, void *img);
void CGContextRelease(void *c);
void *CGDataProviderCreateWithData(void *info, const void *d, unsigned long s, void *r);
void *CGDataProviderCreateWithURL(void *u);
void CGDataProviderRelease(void *p);
void *CGImageCreateWithJPEGDataProvider(void *p, const void *d, int i, int t);
unsigned long CGImageGetWidth(void *i);
unsigned long CGImageGetHeight(void *i);
void CGImageRelease(void *i);

/* audio.c */
int AudioFileClose(void *id);
int AudioFileOpenURL(void *url, signed char perm, uint32_t hint, void **out);
int AudioFileOpenWithCallbacks(void *c, void *r, void *g, void *s, void *w, uint32_t h, void **o);
int AudioFileGetProperty(void *id, uint32_t p, uint32_t *s, void *o);
int AudioFileGetPropertyInfo(void *id, uint32_t p, uint32_t *s, uint32_t *w);
int AudioFileReadPackets(void *id, unsigned char u, uint32_t *n, void *d, uint32_t np, void *o);
int AudioQueueNewOutput(const void *f, void *c, void *u, void *r, void *m, uint32_t fl, void **q);
int AudioQueueDispose(void *q, unsigned char i);
int AudioQueueAllocateBuffer(void *q, uint32_t s, void **b);
int AudioQueueFreeBuffer(void *q, void *b);
int AudioQueueEnqueueBuffer(void *q, void *b, uint32_t n, void *d);
int AudioQueueStart(void *q, const void *t);
int AudioQueueStop(void *q, unsigned char i);
int AudioQueuePause(void *q);
int AudioQueueFlush(void *q);
int AudioQueueReset(void *q);
int AudioQueueSetParameter(void *q, uint32_t id, float v);
int AudioQueueGetParameter(void *q, uint32_t id, float *v);
int AudioQueueGetProperty(void *q, uint32_t id, void *o, uint32_t *s);
int AudioQueueSetProperty(void *q, uint32_t id, const void *i, uint32_t s);
int AudioQueueAddPropertyListener(void *q, uint32_t id, void *p, void *u);
int AudioQueueGetCurrentTime(void *q, void *ts, void *t, void *tl);
void AudioServicesPlaySystemSound(uint32_t id);
int AudioSessionInitialize(void *r, void *m, void *i, void *u);
int AudioSessionSetActive(unsigned char a);
int AudioSessionSetProperty(uint32_t id, uint32_t s, const void *d);
int AudioSessionGetProperty(uint32_t id, uint32_t *s, void *d);
int AudioSessionAddPropertyListener(uint32_t id, void *p, void *u);

/* misc.c */
void *SCNetworkReachabilityCreateWithAddress(void *a, const void *b);
int SCNetworkReachabilityGetFlags(void *r, uint32_t *f);
void *CFNetServiceBrowserCreate(void *a, void *c, void *f);
void CFNetServiceBrowserInvalidate(void *b);
void CFNetServiceBrowserScheduleWithRunLoop(void *b, void *r, void *m);
int CFNetServiceBrowserSearchForServices(void *b, void *d, void *t, void *e);
int CFNetServiceBrowserStopSearch(void *b, void *e);
void CFNetServiceBrowserUnscheduleFromRunLoop(void *b, void *r, void *m);
void CFNetServiceCancel(void *s);
void *CFNetServiceCreate(void *a, void *d, void *t, void *n, int p);
void *CFNetServiceGetAddressing(void *s);
void *CFNetServiceGetDomain(void *s);
void *CFNetServiceGetName(void *s);
int CFNetServiceGetPortNumber(void *s);
void *CFNetServiceGetTXTData(void *s);
void *CFNetServiceGetTargetHost(void *s);
void *CFNetServiceGetType(void *s);
int CFNetServiceRegisterWithOptions(void *s, uint32_t o, void *e);
int CFNetServiceResolveWithTimeout(void *s, uint32_t t_lo, uint32_t t_hi, void *e);
void CFNetServiceScheduleWithRunLoop(void *s, void *r, void *m);
void CFNetServiceSetClient(void *s, void *c, void *u);
int CFNetServiceSetTXTData(void *s, void *d);
void CFNetServiceUnscheduleFromRunLoop(void *s, void *r, void *m);

/* libgcc */
extern void gcc_adddf3(void) __asm__("__adddf3");
extern void gcc_addsf3(void) __asm__("__addsf3");
extern void gcc_divdf3(void) __asm__("__divdf3");
extern void gcc_divdi3(void) __asm__("__divdi3");
extern void gcc_divsf3(void) __asm__("__divsf3");
extern void gcc_divsi3(void) __asm__("__divsi3");
extern void gcc_eqdf2(void) __asm__("__eqdf2");
extern void gcc_fixdfsi(void) __asm__("__fixdfsi");
extern void gcc_fixsfsi(void) __asm__("__fixsfsi");
extern void gcc_fixunsdfdi(void) __asm__("__fixunsdfdi");
extern void gcc_floatsidf(void) __asm__("__floatsidf");
extern void gcc_floatsisf(void) __asm__("__floatsisf");
extern void gcc_ltdf2(void) __asm__("__ltdf2");
extern void gcc_moddi3(void) __asm__("__moddi3");
extern void gcc_modsi3(void) __asm__("__modsi3");
extern void gcc_muldf3(void) __asm__("__muldf3");
extern void gcc_mulsf3(void) __asm__("__mulsf3");
extern void gcc_nedf2(void) __asm__("__nedf2");
extern void gcc_subdf3(void) __asm__("__subdf3");
extern void gcc_truncdfsf2(void) __asm__("__truncdfsf2");
extern void gcc_udivsi3(void) __asm__("__udivsi3");
extern void gcc_umodsi3(void) __asm__("__umodsi3");

/* ------------------------------------------------------------------ */
/* GL1 missing bits                                                   */
/* ------------------------------------------------------------------ */

static void redeg_glGetClipPlanef(GLenum plane, GLfloat *eq) { (void)plane; if (eq) { eq[0]=eq[1]=eq[2]=eq[3]=0; } }
static void redeg_glGetClipPlanex(GLenum plane, GLfixed *eq) { (void)plane; if (eq) { eq[0]=eq[1]=eq[2]=eq[3]=0; } }
static void redeg_glGetFixedv(GLenum pname, GLfixed *params) {
    if (!params) return;
    GLfloat f[16] = {0};
    glGetFloatv(pname, f);
    for (int i = 0; i < 16; i++) params[i] = (GLfixed)(f[i] * 65536.0f);
}
static void redeg_glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
    (void)light;
    if (!params) return;
    if (pname == GL_POSITION) { params[0]=0; params[1]=0; params[2]=1; params[3]=0; }
    else if (pname == GL_AMBIENT) { params[0]=0; params[1]=0; params[2]=0; params[3]=1; }
    else if (pname == GL_DIFFUSE) { params[0]=1; params[1]=1; params[2]=1; params[3]=1; }
    else if (pname == GL_SPECULAR) { params[0]=1; params[1]=1; params[2]=1; params[3]=1; }
    else { params[0]=params[1]=params[2]=params[3]=0; }
}
static void redeg_glGetLightxv(GLenum l, GLenum p, GLfixed *o) {
    GLfloat f[4]; redeg_glGetLightfv(l, p, f);
    for (int i=0;i<4;i++) o[i]=(GLfixed)(f[i]*65536.0f);
}
static void redeg_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) {
    (void)face;
    if (!params) return;
    if (pname == GL_AMBIENT) { params[0]=0.2f; params[1]=0.2f; params[2]=0.2f; params[3]=1; }
    else if (pname == GL_DIFFUSE) { params[0]=0.8f; params[1]=0.8f; params[2]=0.8f; params[3]=1; }
    else if (pname == GL_SPECULAR) { params[0]=0; params[1]=0; params[2]=0; params[3]=1; }
    else if (pname == GL_EMISSION) { params[0]=0; params[1]=0; params[2]=0; params[3]=1; }
    else if (pname == GL_SHININESS) { params[0]=0; }
    else { params[0]=params[1]=params[2]=params[3]=0; }
}
static void redeg_glGetMaterialxv(GLenum f, GLenum p, GLfixed *o) {
    GLfloat v[4]; redeg_glGetMaterialfv(f, p, v);
    for (int i=0;i<4;i++) o[i]=(GLfixed)(v[i]*65536.0f);
}
static void redeg_glGetTexEnvfv(GLenum a, GLenum b, GLfloat *c) { (void)a; (void)b; if (c) c[0]=c[1]=c[2]=c[3]=0; }
static void redeg_glGetTexParameterfv(GLenum t, GLenum p, GLfloat *o) { (void)t; if (!o) return; if (p==GL_TEXTURE_MIN_FILTER) o[0]=GL_NEAREST_MIPMAP_LINEAR; else if (p==GL_TEXTURE_MAG_FILTER) o[0]=GL_LINEAR; else o[0]=GL_REPEAT; }
static void redeg_glGetTexParameteriv(GLenum t, GLenum p, GLint *o) { GLfloat f=0; redeg_glGetTexParameterfv(t,p,&f); if (o) o[0]=(GLint)f; }
static void redeg_glGetTexParameterxv(GLenum t, GLenum p, GLfixed *o) { GLfloat f=0; redeg_glGetTexParameterfv(t,p,&f); if (o) o[0]=(GLfixed)(f*65536.0f); }
static void redeg_glTexParameterfv(GLenum t, GLenum p, const GLfloat *v) { if (v) glTexParameterf(t, p, v[0]); }
static void redeg_glTexParameterxv(GLenum t, GLenum p, const GLfixed *v) { if (v) glTexParameterf(t, p, (GLfloat)v[0]/65536.0f); }
static void redeg_glLightf(GLenum l, GLenum p, GLfloat v) { GLfloat a[4]={v,0,0,0}; glLightfv(l,p,a); }
static void redeg_glLightModelx(GLenum p, GLfixed v) { GLfloat a[4]={(GLfloat)v/65536.0f,0,0,0}; glLightModelfv(p,a); }
static void redeg_glLightx(GLenum l, GLenum p, GLfixed v) { redeg_glLightf(l,p,(GLfloat)v/65536.0f); }
static void redeg_glLightModelf(GLenum p, GLfloat v) { GLfloat a[4]={v,0,0,0}; glLightModelfv(p,a); }
static void redeg_glPointParameterf(GLenum p, GLfloat v) { (void)p; (void)v; }
static void redeg_glPointParameterfv(GLenum p, const GLfloat *v) { (void)p; (void)v; }
static void redeg_glPointParameterx(GLenum p, GLfixed v) { (void)p; (void)v; }
static void redeg_glPointParameterxv(GLenum p, const GLfixed *v) { (void)p; (void)v; }
static void redeg_glMultiTexCoord4f(GLenum t, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { (void)t; (void)x; (void)y; (void)z; (void)w; }
static void redeg_glMultiTexCoord4x(GLenum t, GLfixed x, GLfixed y, GLfixed z, GLfixed w) { (void)t; (void)x; (void)y; (void)z; (void)w; }
static void redeg_glLogicOp(GLenum op) { (void)op; }
static void redeg_glSampleCoveragex(GLclampx v, GLboolean invert) { (void)v; (void)invert; }
static void redeg_glPointSizePointerOES(GLenum type, GLsizei stride, const void *ptr) { (void)type; (void)stride; (void)ptr; }
static GLboolean redeg_glIsBuffer(GLuint b) { (void)b; return GL_FALSE; }
static void redeg_glGetRenderbufferParameteriv(GLenum t, GLenum p, GLint *v) {
    (void)t;
    if (!v) return;
    if (p == 0x8D42) *v = 480;      /* GL_RENDERBUFFER_WIDTH_OES */
    else if (p == 0x8D43) *v = 320; /* GL_RENDERBUFFER_HEIGHT_OES */
    else *v = 0;
}
static void redeg_glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    if (w <= 480 && h <= 320) {
        x = (x * 960) / 480;
        y = (y * 544) / 320;
        w = (w * 960) / 480;
        h = (h * 544) / 320;
    }
    glViewport(x, y, w, h);
}
static void redeg_glScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    if (w <= 480 && h <= 320) {
        x = (x * 960) / 480;
        y = (y * 544) / 320;
        w = (w * 960) / 480;
        h = (h * 544) / 320;
    }
    glScissor(x, y, w, h);
}
static void redeg_glShadeModel(GLenum mode) {
    (void)mode;
    glShadeModel(GL_SMOOTH);
}
static void redeg_glPixelStorei(GLenum pname, GLint param) {
    if (pname == 0x0CF5) return; /* GL_PACK_ALIGNMENT not supported in vitaGL */
    glPixelStorei(pname, param);
}
static void redeg_glTexParameteri(GLenum target, GLenum pname, GLint param) {
    if (pname == 0x8191) return; /* GL_GENERATE_MIPMAP not supported as TexParameter in vitaGL */
    glTexParameteri(target, pname, param);
}
static void redeg_glTexParameterx(GLenum target, GLenum pname, GLfixed param) {
    if (pname == 0x8191) return; /* GL_GENERATE_MIPMAP not supported as TexParameter in vitaGL */
    glTexParameterx(target, pname, param);
}
static void redeg_glBindFramebufferOES(GLenum target, GLuint fb) { (void)fb; glBindFramebuffer(target, 0); }
static void redeg_glFramebufferRenderbufferOES(GLenum target, GLenum attachment, GLenum rbtarget, GLuint rb) {
    (void)target; (void)attachment; (void)rbtarget; (void)rb; /* render to the display directly */
}
static GLenum redeg_glCheckFramebufferStatusOES(GLenum target) { (void)target; return GL_FRAMEBUFFER_COMPLETE; }
static void redeg_glCompressedTexSubImage2D(GLenum t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLsizei s, const void *d) { (void)t;(void)l;(void)x;(void)y;(void)w;(void)h;(void)f;(void)s;(void)d; }
static const GLubyte *redeg_glGetString(GLenum name) {
    const GLubyte *res = (name == 0x1F01)
        ? (const GLubyte *)"PowerVR SGX 543MP4+"
        : glGetString(name);
    l_info("[GL] glGetString(0x%04x) -> '%s'", (unsigned)name, res ? (const char *)res : "(null)");
    return res;
}

static int redeg_egl_true(void) { return 1; }

/* new/delete */
static void *redeg_operator_new(size_t n) { return malloc(n ? n : 1); }
static void redeg_operator_delete(void *p) { free(p); }

/* ------------------------------------------------------------------ */

#define S(n, f) { n, (void *)(f) }

static const sym_entry_t s_syms[] = {
    /* libc */
    S("memcpy", memcpy), S("memmove", memmove), S("memset", memset),
    S("malloc", redeg_malloc), S("free", free), S("calloc", redeg_calloc), S("realloc", redeg_realloc),
    S("printf", redeg_printf), S("puts", puts), S("sprintf", sprintf),
    S("snprintf", snprintf), S("vsnprintf", vsnprintf), S("sscanf", sscanf),
    S("atoi", atoi), S("strlen", strlen), S("strcmp", strcmp), S("strcpy", strcpy),
    S("strcat", strcat), S("strchr", strchr), S("strncpy", strncpy),
    S("strncat", strncat), S("strcasecmp", strcasecmp), S("strncasecmp", strncasecmp),
    S("strerror", strerror),
    S("fopen", redeg_fopen), S("fread", redeg_fread), S("fclose", fclose),
    S("open", redeg_open), S("read", redeg_read), S("write", write), S("close", close),
    S("mkdir", mkdir), S("rmdir", rmdir), S("unlink", unlink),
    S("rename", rename), S("remove", remove), S("chdir", chdir),
    S("opendir", redeg_opendir), S("readdir", redeg_readdir), S("closedir", redeg_closedir),
    S("stat", redeg_stat), S("statvfs", redeg_statvfs),
    S("lseek", redeg_lseek), S("truncate", redeg_truncate),
    S("time", time), S("localtime", localtime), S("tzset", tzset),
    S("gettimeofday", gettimeofday), S("usleep", usleep),
    S("setjmp", setjmp), S("longjmp", longjmp), S("exit", redeg_exit),
    S("__error", redeg_error), S("bzero", redeg_bzero),
    S("sigaction", redeg_sigaction),
    S("dlopen", redeg_dlopen), S("dlsym", redeg_dlsym),
    S("dlclose", redeg_dlclose), S("dlerror", redeg_dlerror),
    S("select", redeg_select), S("fcntl", redeg_fcntl), S("chmod", redeg_chmod),
    S("socket", redeg_socket), S("bind", redeg_bind), S("connect", redeg_connect),
    S("listen", redeg_listen), S("accept", redeg_accept), S("send", redeg_send),
    S("recv", redeg_recv), S("sendto", redeg_sendto), S("recvfrom", redeg_recvfrom),
    S("setsockopt", redeg_setsockopt), S("getsockname", redeg_getsockname),
    S("getpeername", redeg_getpeername), S("gethostname", redeg_gethostname),
    S("getdomainname", redeg_getdomainname), S("gethostbyname", redeg_gethostbyname),
    S("inet_aton", redeg_inet_aton), S("inet_ntoa", redeg_inet_ntoa),
    S("backtrace", redeg_backtrace), S("backtrace_symbols", redeg_backtrace_symbols),
    S("semaphore_create", redeg_semaphore_create),
    S("semaphore_signal", redeg_semaphore_signal),
    S("semaphore_wait", redeg_semaphore_wait),
    S("_keymgr_get_and_lock_processwide_ptr", redeg_keymgr_get),
    S("_keymgr_get_and_lock_processwide_ptr_2", redeg_keymgr_get2),
    S("_keymgr_set_and_unlock_processwide_ptr", redeg_keymgr_set),
    S("mach_task_self_", &mach_task_self_),

    /* pthread */
    S("pthread_attr_destroy", pthread_attr_destroy),
    S("pthread_attr_getdetachstate", pthread_attr_getdetachstate),
    S("pthread_attr_init", pthread_attr_init),
    S("pthread_attr_setdetachstate", pthread_attr_setdetachstate),
    S("pthread_attr_setscope", pthread_attr_setscope),
    S("pthread_attr_setstacksize", pthread_attr_setstacksize),
    S("pthread_cond_broadcast", pthread_cond_broadcast),
    S("pthread_cond_destroy", pthread_cond_destroy),
    S("pthread_cond_init", pthread_cond_init),
    S("pthread_cond_signal", redeg_pthread_cond_signal),
    S("pthread_cond_timedwait", pthread_cond_timedwait),
    S("pthread_cond_wait", redeg_pthread_cond_wait),
    S("pthread_create", redeg_pthread_create),
    S("pthread_detach", pthread_detach),
    S("pthread_equal", pthread_equal),
    S("pthread_exit", pthread_exit),
    S("pthread_getschedparam", pthread_getschedparam),
    S("pthread_getspecific", pthread_getspecific),
    S("pthread_join", pthread_join),
    S("pthread_key_create", pthread_key_create),
    S("pthread_key_delete", pthread_key_delete),
    S("pthread_kill", pthread_kill),
    S("pthread_mutex_destroy", pthread_mutex_destroy),
    S("pthread_mutex_init", pthread_mutex_init),
    S("pthread_mutex_lock", redeg_pthread_mutex_lock),
    S("pthread_mutex_trylock", pthread_mutex_trylock),
    S("pthread_mutex_unlock", redeg_pthread_mutex_unlock),
    S("pthread_mutexattr_destroy", pthread_mutexattr_destroy),
    S("pthread_mutexattr_gettype", pthread_mutexattr_gettype),
    S("pthread_mutexattr_init", pthread_mutexattr_init),
    S("pthread_mutexattr_settype", pthread_mutexattr_settype),
    S("pthread_once", pthread_once),
    S("pthread_self", pthread_self),
    S("pthread_setspecific", pthread_setspecific),
    S("pthread_sigmask", pthread_sigmask),

    /* libgcc */
    S("__adddf3", gcc_adddf3), S("__addsf3", gcc_addsf3), S("__divdf3", gcc_divdf3),
    S("__divdi3", gcc_divdi3), S("__divsf3", gcc_divsf3), S("__divsi3", gcc_divsi3),
    S("__eqdf2", gcc_eqdf2), S("__fixdfsi", gcc_fixdfsi), S("__fixsfsi", gcc_fixsfsi),
    S("__fixunsdfdi", gcc_fixunsdfdi), S("__floatsidf", gcc_floatsidf),
    S("__floatsisf", gcc_floatsisf), S("__ltdf2", gcc_ltdf2), S("__moddi3", gcc_moddi3),
    S("__modsi3", gcc_modsi3), S("__muldf3", gcc_muldf3), S("__mulsf3", gcc_mulsf3),
    S("__nedf2", gcc_nedf2), S("__subdf3", gcc_subdf3), S("__truncdfsf2", gcc_truncdfsf2),
    S("__udivsi3", gcc_udivsi3), S("__umodsi3", gcc_umodsi3),

    /* C++ */
    S("_Znwm", redeg_operator_new), S("_Znam", redeg_operator_new),
    S("_ZdlPv", redeg_operator_delete), S("_ZdaPv", redeg_operator_delete),

    /* zlib */
    S("inflate", inflate), S("inflateEnd", inflateEnd),
    S("inflateInit2_", inflateInit2_), S("inflateInit_", inflateInit_),

    /* libSystem getenv + ObjC runtime entry points */
    S("getenv", redeg_getenv),
    S("objc_msgSend", objc_msgSend),
    S("objc_msgSendSuper2", objc_msgSendSuper2),
    S("objc_msgSend_stret", objc_msgSend_stret),
    S("objc_msgSendSuper2_stret", objc_msgSendSuper2_stret),
    S("objc_setProperty", objc_setProperty),

    /* Foundation / UIKit entry */
    S("NSLog", NSLog), S("UIApplicationMain", redeg_UIApplicationMain),

    /* CoreFoundation */
    S("CFArrayGetValueAtIndex", CFArrayGetValueAtIndex),
    S("CFDataCreate", CFDataCreate), S("CFDataGetBytePtr", CFDataGetBytePtr),
    S("CFDataGetLength", CFDataGetLength), S("CFDictionaryGetValue", CFDictionaryGetValue),
    S("CFNumberGetValue", CFNumberGetValue), S("CFRelease", CFRelease),
    S("CFRunLoopGetCurrent", CFRunLoopGetCurrent), S("CFRunLoopRunInMode", CFRunLoopRunInMode),
    S("CFRunLoopStop", CFRunLoopStop), S("CFStringCompare", CFStringCompare),
    S("CFStringCreateWithCString", CFStringCreateWithCString),
    S("CFStringGetCString", CFStringGetCString),
    S("CFURLCreateFromFileSystemRepresentation", CFURLCreateFromFileSystemRepresentation),
    S("kCFAllocatorDefault", &kCFAllocatorDefault),
    S("kCFRunLoopCommonModes", &kCFRunLoopCommonModes),
    S("kCFRunLoopDefaultMode", &kCFRunLoopDefaultMode),

    /* CoreGraphics */
    S("CGAffineTransformMakeTranslation", CGAffineTransformMakeTranslation),
    S("CGBitmapContextCreate", CGBitmapContextCreate),
    S("CGBitmapContextCreateImage", CGBitmapContextCreateImage),
    S("CGColorSpaceCreateDeviceRGB", CGColorSpaceCreateDeviceRGB),
    S("CGColorSpaceRelease", CGColorSpaceRelease),
    S("CGContextDrawImage", CGContextDrawImage), S("CGContextRelease", CGContextRelease),
    S("CGDataProviderCreateWithData", CGDataProviderCreateWithData),
    S("CGDataProviderCreateWithURL", CGDataProviderCreateWithURL),
    S("CGDataProviderRelease", CGDataProviderRelease),
    S("CGImageCreateWithJPEGDataProvider", CGImageCreateWithJPEGDataProvider),
    S("CGImageGetHeight", CGImageGetHeight), S("CGImageGetWidth", CGImageGetWidth),
    S("CGImageRelease", CGImageRelease),

    /* AudioToolbox */
    S("AudioFileClose", AudioFileClose), S("AudioFileOpenURL", AudioFileOpenURL),
    S("AudioFileOpenWithCallbacks", AudioFileOpenWithCallbacks),
    S("AudioFileGetProperty", AudioFileGetProperty),
    S("AudioFileGetPropertyInfo", AudioFileGetPropertyInfo),
    S("AudioFileReadPackets", AudioFileReadPackets),
    S("AudioQueueNewOutput", AudioQueueNewOutput), S("AudioQueueDispose", AudioQueueDispose),
    S("AudioQueueAllocateBuffer", AudioQueueAllocateBuffer),
    S("AudioQueueFreeBuffer", AudioQueueFreeBuffer),
    S("AudioQueueEnqueueBuffer", AudioQueueEnqueueBuffer),
    S("AudioQueueStart", AudioQueueStart), S("AudioQueueStop", AudioQueueStop),
    S("AudioQueuePause", AudioQueuePause), S("AudioQueueFlush", AudioQueueFlush),
    S("AudioQueueReset", AudioQueueReset), S("AudioQueueSetParameter", AudioQueueSetParameter),
    S("AudioQueueGetParameter", AudioQueueGetParameter), S("AudioQueueGetProperty", AudioQueueGetProperty),
    S("AudioQueueSetProperty", AudioQueueSetProperty),
    S("AudioQueueAddPropertyListener", AudioQueueAddPropertyListener),
    S("AudioQueueGetCurrentTime", AudioQueueGetCurrentTime),
    S("AudioServicesPlaySystemSound", AudioServicesPlaySystemSound),
    S("AudioSessionInitialize", AudioSessionInitialize),
    S("AudioSessionSetActive", AudioSessionSetActive),
    S("AudioSessionSetProperty", AudioSessionSetProperty),
    S("AudioSessionGetProperty", AudioSessionGetProperty),
    S("AudioSessionAddPropertyListener", AudioSessionAddPropertyListener),

    /* framework data */
    S("kEAGLDrawablePropertyColorFormat", &kEAGLDrawablePropertyColorFormat),
    S("kEAGLDrawablePropertyRetainedBacking", &kEAGLDrawablePropertyRetainedBacking),
    S("kEAGLColorFormatRGBA8", &kEAGLColorFormatRGBA8),
    S("kCLLocationAccuracyBest", &kCLLocationAccuracyBest),
    S("kCLDistanceFilterNone", &kCLDistanceFilterNone),
    S("kCLErrorDomain", &kCLErrorDomain),
    S("MPMoviePlayerContentPreloadDidFinishNotification", &MPMoviePlayerContentPreloadDidFinishNotification),
    S("MPMoviePlayerPlaybackDidFinishNotification", &MPMoviePlayerPlaybackDidFinishNotification),

    /* SystemConfiguration / CFNetwork */
    S("SCNetworkReachabilityCreateWithAddress", SCNetworkReachabilityCreateWithAddress),
    S("SCNetworkReachabilityGetFlags", SCNetworkReachabilityGetFlags),
    S("CFNetServiceBrowserCreate", CFNetServiceBrowserCreate),
    S("CFNetServiceBrowserInvalidate", CFNetServiceBrowserInvalidate),
    S("CFNetServiceBrowserScheduleWithRunLoop", CFNetServiceBrowserScheduleWithRunLoop),
    S("CFNetServiceBrowserSearchForServices", CFNetServiceBrowserSearchForServices),
    S("CFNetServiceBrowserStopSearch", CFNetServiceBrowserStopSearch),
    S("CFNetServiceBrowserUnscheduleFromRunLoop", CFNetServiceBrowserUnscheduleFromRunLoop),
    S("CFNetServiceCancel", CFNetServiceCancel), S("CFNetServiceCreate", CFNetServiceCreate),
    S("CFNetServiceGetAddressing", CFNetServiceGetAddressing),
    S("CFNetServiceGetDomain", CFNetServiceGetDomain),
    S("CFNetServiceGetName", CFNetServiceGetName),
    S("CFNetServiceGetPortNumber", CFNetServiceGetPortNumber),
    S("CFNetServiceGetTXTData", CFNetServiceGetTXTData),
    S("CFNetServiceGetTargetHost", CFNetServiceGetTargetHost),
    S("CFNetServiceGetType", CFNetServiceGetType),
    S("CFNetServiceRegisterWithOptions", CFNetServiceRegisterWithOptions),
    S("CFNetServiceResolveWithTimeout", CFNetServiceResolveWithTimeout),
    S("CFNetServiceScheduleWithRunLoop", CFNetServiceScheduleWithRunLoop),
    S("CFNetServiceSetClient", CFNetServiceSetClient),
    S("CFNetServiceSetTXTData", CFNetServiceSetTXTData),
    S("CFNetServiceUnscheduleFromRunLoop", CFNetServiceUnscheduleFromRunLoop),

    /* EGL extras missing from vitaGL */
    S("eglBindTexImage", redeg_egl_true), S("eglReleaseTexImage", redeg_egl_true),
    S("eglCopyBuffers", redeg_egl_true), S("eglCreatePbufferSurface", redeg_egl_true),
    S("eglCreatePixmapSurface", redeg_egl_true), S("eglGetCurrentDisplay", redeg_egl_true),
    S("eglSurfaceAttrib", redeg_egl_true), S("eglWaitGL", redeg_egl_true),
    S("eglWaitNative", redeg_egl_true),

    /* GLES1 gaps */
    S("glBindFramebufferOES", redeg_glBindFramebufferOES), S("glBindRenderbufferOES", glBindRenderbuffer),
    S("glCheckFramebufferStatusOES", redeg_glCheckFramebufferStatusOES),
    S("glDeleteFramebuffersOES", glDeleteFramebuffers), S("glDeleteRenderbuffersOES", glDeleteRenderbuffers),
    S("glFramebufferRenderbufferOES", redeg_glFramebufferRenderbufferOES),
    S("glFramebufferTexture2DOES", glFramebufferTexture2D), S("glGenerateMipmapOES", glGenerateMipmap),
    S("glGenFramebuffersOES", glGenFramebuffers), S("glGenRenderbuffersOES", glGenRenderbuffers),
    S("glGetFramebufferAttachmentParameterivOES", glGetFramebufferAttachmentParameteriv),
    S("glGetRenderbufferParameterivOES", redeg_glGetRenderbufferParameteriv),
    S("glIsFramebufferOES", glIsFramebuffer), S("glIsRenderbufferOES", glIsRenderbuffer),
    S("glRenderbufferStorageOES", glRenderbufferStorage),
    S("glGetClipPlanef", redeg_glGetClipPlanef), S("glGetClipPlanex", redeg_glGetClipPlanex),
    S("glGetFixedv", redeg_glGetFixedv),
    S("glGetLightfv", redeg_glGetLightfv), S("glGetLightxv", redeg_glGetLightxv),
    S("glGetMaterialfv", redeg_glGetMaterialfv), S("glGetMaterialxv", redeg_glGetMaterialxv),
    S("glGetTexEnvfv", redeg_glGetTexEnvfv),
    S("glGetTexParameterfv", redeg_glGetTexParameterfv),
    S("glGetTexParameteriv", redeg_glGetTexParameteriv),
    S("glGetTexParameterxv", redeg_glGetTexParameterxv),
    S("glTexParameterfv", redeg_glTexParameterfv), S("glTexParameterxv", redeg_glTexParameterxv),
    S("glLightf", redeg_glLightf), S("glLightx", redeg_glLightx),
    S("glLightModelx", redeg_glLightModelx),
    S("glLightModelf", redeg_glLightModelf),
    S("glPointParameterf", redeg_glPointParameterf),
    S("glPointParameterfv", redeg_glPointParameterfv),
    S("glPointParameterx", redeg_glPointParameterx),
    S("glPointParameterxv", redeg_glPointParameterxv),
    S("glMultiTexCoord4f", redeg_glMultiTexCoord4f), S("glMultiTexCoord4x", redeg_glMultiTexCoord4x),
    S("glLogicOp", redeg_glLogicOp), S("glSampleCoveragex", redeg_glSampleCoveragex),
    S("glPointSizePointerOES", redeg_glPointSizePointerOES),
    S("glIsBuffer", redeg_glIsBuffer), S("glCompressedTexSubImage2D", redeg_glCompressedTexSubImage2D),
    S("glGetString", redeg_glGetString),
    S("glViewport", redeg_glViewport), S("glScissor", redeg_glScissor),
    S("glShadeModel", redeg_glShadeModel), S("glPixelStorei", redeg_glPixelStorei),
};

/* Every GL/EGL symbol vitaGL exports is resolved from this table by name. */
void *redeg_lookup_gl(const char *name);

void redeg_missing_import(void) {
    static int n = 0;
    if (n < 32) {
        l_warn("[import] call to unimplemented stub from 0x%08x", (uint32_t)(uintptr_t)__builtin_return_address(0));
        n++;
    }
}

uint32_t redeg_resolve_symbol(const char *name) {
    if (!name) return 0;

    if (strncmp(name, "_OBJC_CLASS_$_", 14) == 0) {
        Class c = objc_get_class(name + 14);
        if (!c) l_warn("[objc] unknown class %s", name + 14);
        return (uint32_t)(uintptr_t)c;
    }
    if (strncmp(name, "_OBJC_METACLASS_$_", 18) == 0) {
        Class c = objc_get_class(name + 18);
        if (!c) l_warn("[objc] unknown metaclass %s", name + 18);
        return (uint32_t)(uintptr_t)(c ? c->isa : NULL);
    }
    if (strcmp(name, "___CFConstantStringClassReference") == 0)
        return (uint32_t)(uintptr_t)g_NSCFConstantString;
    if (strcmp(name, "__objc_empty_cache") == 0)
        return (uint32_t)(uintptr_t)&_objc_empty_cache[0];
    if (strcmp(name, "__objc_empty_vtable") == 0)
        return (uint32_t)(uintptr_t)&_objc_empty_vtable[0];

    const char *base = (name[0] == '_') ? name + 1 : name;
    for (size_t i = 0; i < sizeof(s_syms) / sizeof(s_syms[0]); i++)
        if (strcmp(s_syms[i].name, base) == 0)
            return (uint32_t)(uintptr_t)s_syms[i].addr;

    void *gl = redeg_lookup_gl(base);
    if (gl) return (uint32_t)(uintptr_t)gl;
    return 0;
}
