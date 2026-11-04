#include "objc/objc_classes.h"
#include "objc/cgttypes.h"
#include "utils/logger.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CoreFoundation objects are laid out like the binary's __cfstring so that
 * they also work as NSString (isa = __NSCFConstantString). */
typedef struct {
    uint32_t isa;
    uint32_t flags;
    char *cstr;
    uint32_t length;
} cfstring_t;

typedef struct {
    uint32_t isa;
    uint8_t *data;
    uint32_t length;
} cfdata_t;

typedef struct {
    uint32_t isa;
    int64_t value;
} cfnumber_t;

typedef struct {
    uint32_t isa;
    char *path;
} cfurl_t;

typedef struct {
    uint32_t isa;
    uint8_t *data;
    uint32_t width, height, bpp, stride;
    int owns;
} cgimage_t;

typedef struct {
    uint32_t isa;
    uint8_t *data;
    uint32_t width, height, bpc, stride, info;
    int owns;
} cgcontext_t;

typedef struct {
    uint32_t isa;
    cgcontext_t *ctx;
} cgcolorspace_t;

typedef struct {
    uint32_t isa;
    uint8_t *data;
    uint32_t size;
} cgprovider_t;

typedef struct { float a, b, c, d, tx, ty; } cgaffine_t;
typedef struct { float x, y; } cgpoint_t;
typedef struct { float w, h; } cgsize_t;
typedef struct { cgpoint_t origin; cgsize_t size; } cgrect_t;

void *kCFAllocatorDefault = NULL;
void *kCFRunLoopCommonModes = NULL;
void *kCFRunLoopDefaultMode = NULL;

static uint32_t s_cf_isa = 0;

static void cf_init_isa(void) {
    if (!s_cf_isa) s_cf_isa = (uint32_t)(uintptr_t)g_NSCFConstantString;
}

/* ---------------- CFString ---------------- */

void *CFStringCreateWithCString(void *alloc, const char *cstr, uint32_t encoding) {
    (void)alloc; (void)encoding;
    cf_init_isa();
    cfstring_t *s = (cfstring_t *)calloc(1, sizeof(cfstring_t));
    s->isa = s_cf_isa;
    s->flags = 0x7c8;
    s->cstr = strdup(cstr ? cstr : "");
    s->length = (uint32_t)strlen(s->cstr);
    return s;
}

int CFStringGetCString(void *str, char *buf, long size, uint32_t encoding) {
    (void)encoding;
    if (!str || !buf || size <= 0) return 0;
    const char *s = ((cfstring_t *)str)->cstr;
    long n = (long)strlen(s);
    if (n >= size) n = size - 1;
    memcpy(buf, s, n);
    buf[n] = 0;
    return 1;
}

int CFStringCompare(void *a, void *b, uint32_t opts) {
    (void)opts;
    const char *sa = a ? ((cfstring_t *)a)->cstr : "";
    const char *sb = b ? ((cfstring_t *)b)->cstr : "";
    return strcmp(sa, sb);
}

/* ---------------- CFData ---------------- */

void *CFDataCreate(void *alloc, const uint8_t *bytes, long length) {
    (void)alloc;
    cfdata_t *d = (cfdata_t *)calloc(1, sizeof(cfdata_t));
    d->data = (uint8_t *)malloc((size_t)length + 1);
    if (bytes && length > 0) memcpy(d->data, bytes, (size_t)length);
    d->data[length] = 0;
    d->length = (uint32_t)length;
    return d;
}

const uint8_t *CFDataGetBytePtr(void *data) { return data ? ((cfdata_t *)data)->data : NULL; }
long CFDataGetLength(void *data) { return data ? (long)((cfdata_t *)data)->length : 0; }

/* ---------------- CFArray / CFDictionary / CFNumber ---------------- */

void *CFArrayGetValueAtIndex(void *arr, long idx) {
    if (!arr) return NULL;
    /* our NSArray layout: isa, items[8], count */
    typedef struct { uint32_t isa; void *items[8]; int count; } arr_t;
    arr_t *a = (arr_t *)arr;
    if (idx < 0 || idx >= a->count) return NULL;
    return a->items[idx];
}

void *CFDictionaryGetValue(void *dict, const void *key) { (void)dict; (void)key; return NULL; }

int CFNumberGetValue(void *num, int type, void *out) {
    (void)type;
    if (!num || !out) return 0;
    *(int64_t *)out = ((cfnumber_t *)num)->value;
    return 1;
}

void CFRelease(void *obj) { (void)obj; }

/* ---------------- CFRunLoop ---------------- */

static uint32_t s_runloop;

void *CFRunLoopGetCurrent(void) { return &s_runloop; }
int CFRunLoopRunInMode(void *mode, uint32_t seconds_lo, uint32_t seconds_hi, int returnAfterSourceHandled) {
    (void)mode; (void)seconds_lo; (void)seconds_hi; (void)returnAfterSourceHandled;
    return 0;
}
void CFRunLoopStop(void *rl) { (void)rl; }

/* ---------------- CFURL ---------------- */

void *CFURLCreateFromFileSystemRepresentation(void *alloc, const uint8_t *buf, long len, int isdir) {
    (void)alloc; (void)isdir;
    cfurl_t *u = (cfurl_t *)calloc(1, sizeof(cfurl_t));
    u->path = (char *)malloc((size_t)len + 1);
    memcpy(u->path, buf, (size_t)len);
    u->path[len] = 0;
    return u;
}

const char *cfurl_path(void *url) { return url ? ((cfurl_t *)url)->path : ""; }

/* ---------------- CoreGraphics ---------------- */

redeg_cgaffine_t CGAffineTransformMakeTranslation(float tx, float ty) {
    redeg_cgaffine_t r = {1, 0, 0, 1, tx, ty};
    return r;
}

void *CGColorSpaceCreateDeviceRGB(void) {
    cgcolorspace_t *c = (cgcolorspace_t *)calloc(1, sizeof(cgcolorspace_t));
    return c;
}
void CGColorSpaceRelease(void *cs) { free(cs); }

void *CGBitmapContextCreate(void *data, unsigned long width, unsigned long height,
                            unsigned long bpc, unsigned long stride, void *cs, uint32_t info) {
    (void)cs;
    cgcontext_t *c = (cgcontext_t *)calloc(1, sizeof(cgcontext_t));
    c->width = (uint32_t)width;
    c->height = (uint32_t)height;
    c->bpc = (uint32_t)bpc;
    c->stride = (uint32_t)stride;
    c->info = info;
    if (data) {
        c->data = (uint8_t *)data;
        c->owns = 0;
    } else {
        c->data = (uint8_t *)calloc(1, (size_t)height * stride);
        c->owns = 1;
    }
    return c;
}

void *CGBitmapContextCreateImage(void *ctx) {
    if (!ctx) return NULL;
    cgcontext_t *c = (cgcontext_t *)ctx;
    cgimage_t *img = (cgimage_t *)calloc(1, sizeof(cgimage_t));
    img->data = c->data;
    img->width = c->width;
    img->height = c->height;
    img->bpp = c->bpc;
    img->stride = c->stride;
    img->owns = 0;
    return img;
}

void CGContextDrawImage(void *ctx, cgrect_t rect, void *image) {
    (void)ctx; (void)rect; (void)image;
}

void CGContextRelease(void *ctx) {
    if (!ctx) return;
    cgcontext_t *c = (cgcontext_t *)ctx;
    if (c->owns) free(c->data);
    free(c);
}

void *CGDataProviderCreateWithData(void *info, const void *data, unsigned long size, void *release) {
    (void)info; (void)release;
    cgprovider_t *p = (cgprovider_t *)calloc(1, sizeof(cgprovider_t));
    p->data = (uint8_t *)data;
    p->size = (uint32_t)size;
    return p;
}

void *CGDataProviderCreateWithURL(void *url) {
    const char *path = cfurl_path(url);
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
    fclose(f);
    cgprovider_t *p = (cgprovider_t *)calloc(1, sizeof(cgprovider_t));
    p->data = buf;
    p->size = (uint32_t)sz;
    return p;
}

void CGDataProviderRelease(void *provider) {
    if (!provider) return;
    cgprovider_t *p = (cgprovider_t *)provider;
    free(p->data);
    free(p);
}

void *CGImageCreateWithJPEGDataProvider(void *provider, const void *decode, int interpolate, int intent) {
    (void)decode; (void)interpolate; (void)intent;
    cgimage_t *img = (cgimage_t *)calloc(1, sizeof(cgimage_t));
    if (provider) {
        cgprovider_t *p = (cgprovider_t *)provider;
        img->data = p->data;
        img->width = 0;  /* unknown until a JPEG decoder is wired */
        img->height = 0;
    }
    return img;
}

unsigned long CGImageGetWidth(void *image) { return image ? ((cgimage_t *)image)->width : 0; }
unsigned long CGImageGetHeight(void *image) { return image ? ((cgimage_t *)image)->height : 0; }
void CGImageRelease(void *image) { free(image); }
