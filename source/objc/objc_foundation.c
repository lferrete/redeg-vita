#include "objc/objc_classes.h"
#include "utils/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Class g_NSString, g_NSNumber, g_NSURL, g_NSDictionary, g_NSArray, g_NSEnumerator;
Class g_NSNotificationCenter, g_NSAutoreleasePool, g_NSLocale, g_NSCFConstantString;

typedef struct {
    Class isa;
    char *str;
} nsstring_t;

typedef struct {
    Class isa;
    long value;
} nsnumber_t;

typedef struct {
    Class isa;
    char *path;
} nsurl_t;

typedef struct {
    Class isa;
    id items[8];
    int count;
} nsarray_t;

typedef struct {
    Class isa;
    nsarray_t *array;
    int idx;
} nsenum_t;

/* ---------------- NSObject ---------------- */

static id nso_alloc(id self, SEL sel) {
    (void)sel;
    return objc_alloc_instance((Class)self);
}
static id nso_init(id self, SEL sel) { (void)sel; return self; }
static id nso_retain(id self, SEL sel) { (void)sel; return self; }
static void nso_release(id self, SEL sel) { (void)self; (void)sel; }
static id nso_autorelease(id self, SEL sel) { (void)sel; return self; }
static Class nso_class(id self, SEL sel) { (void)sel; return self->isa; }
static Class nso_class_cls(id self, SEL sel) { (void)sel; return (Class)self; }
static void nso_dealloc(id self, SEL sel) { (void)self; (void)sel; }
void redeg_perform_on_main(id target, SEL sel, id arg, int wait);

static id nso_perform_main(id self, SEL sel, SEL aSelector, id arg, int wait) {
    (void)sel;
    static int logged = 0;
    if (logged++ < 16) l_info("[main] perform %s wait=%d", aSelector, wait);
    redeg_perform_on_main(self, aSelector, arg, wait);
    return NULL;
}
static int nso_is_equal(id self, SEL sel, id other) { (void)sel; return self == other; }

static const objc_method_def_t nso_methods[] = {
    {"init", nso_init}, {"dealloc", nso_dealloc}, {"retain", nso_retain},
    {"release", nso_release}, {"autorelease", nso_autorelease},
    {"class", nso_class}, {"isEqual:", nso_is_equal},
    {"performSelectorOnMainThread:withObject:waitUntilDone:", nso_perform_main},
};

static const objc_method_def_t nso_cmethods[] = {
    {"alloc", nso_alloc}, {"class", nso_class_cls},
};

/* ---------------- NSAutoreleasePool ---------------- */

static const objc_method_def_t pool_methods[] = {
    {"init", nso_init},
};

/* ---------------- NSString ---------------- */

static id nsstr_string_with_utf8(id self, SEL sel, const char *s) {
    (void)self; (void)sel;
    nsstring_t *o = (nsstring_t *)calloc(1, sizeof(nsstring_t));
    o->isa = g_NSString;
    o->str = strdup(s ? s : "");
    return (id)o;
}
static id redeg_nsstring_new_impl(const char *s) {
    return nsstr_string_with_utf8(NULL, NULL, s);
}
id redeg_nsstring_new(const char *s) {
    extern id redeg_nsstring_new_impl(const char *s);
    return redeg_nsstring_new_impl(s);
}
static id nsstr_init_with_utf8(id self, SEL sel, const char *s) {
    (void)sel;
    ((nsstring_t *)self)->str = strdup(s ? s : "");
    return self;
}
static id nsstr_init_with_string(id self, SEL sel, id other) {
    (void)sel;
    ((nsstring_t *)self)->str = strdup(other ? ((nsstring_t *)other)->str : "");
    return self;
}
static const char *nsstr_utf8(id self, SEL sel) {
    (void)sel;
    return ((nsstring_t *)self)->str ? ((nsstring_t *)self)->str : "";
}
static int nsstr_is_equal(id self, SEL sel, id other) {
    (void)sel;
    if (!other) return 0;
    const char *a = nsstr_utf8(self, NULL);
    const char *b = ((nsstring_t *)other)->str;
    return b && strcmp(a, b) == 0;
}
static int nsstr_get_cstring(id self, SEL sel, char *buf, int maxlen, unsigned enc) {
    (void)sel; (void)enc;
    const char *s = nsstr_utf8(self, NULL);
    if (!buf || maxlen <= 0) return 0;
    int n = (int)strlen(s);
    if (n >= maxlen) n = maxlen - 1;
    memcpy(buf, s, n);
    buf[n] = 0;
    return 1;
}
static long nsstr_length(id self, SEL sel) {
    (void)sel;
    return (long)strlen(nsstr_utf8(self, NULL));
}

static const objc_method_def_t nsstr_methods[] = {
    {"initWithUTF8String:", nsstr_init_with_utf8},
    {"initWithString:", nsstr_init_with_string},
    {"UTF8String", nsstr_utf8},
    {"isEqualToString:", nsstr_is_equal},
    {"getCString:maxLength:encoding:", nsstr_get_cstring},
    {"length", nsstr_length},
};
static const objc_method_def_t nsstr_cmethods[] = {
    {"stringWithUTF8String:", nsstr_string_with_utf8},
};

/* ---------------- __NSCFConstantString (from the binary's __cfstring) ---------------- */

static const char *cfstr_utf8(id self, SEL sel) {
    (void)sel;
    const char *p = *(const char **)((const char *)self + 8);
    return p ? p : "";
}
static long cfstr_length(id self, SEL sel) {
    (void)sel;
    return (long)*(const uint32_t *)((const char *)self + 12);
}
static int cfstr_is_equal(id self, SEL sel, id other) {
    (void)sel;
    const char *a = cfstr_utf8(self, NULL);
    const char *b = nsstr_utf8(other, NULL);
    return b && strcmp(a, b) == 0;
}
static int cfstr_get_cstring(id self, SEL sel, char *buf, int maxlen, unsigned enc) {
    (void)sel; (void)enc;
    const char *s = cfstr_utf8(self, NULL);
    if (!buf || maxlen <= 0) return 0;
    int n = (int)strlen(s);
    if (n >= maxlen) n = maxlen - 1;
    memcpy(buf, s, n);
    buf[n] = 0;
    return 1;
}
static const objc_method_def_t cfstr_methods[] = {
    {"UTF8String", cfstr_utf8},
    {"length", cfstr_length},
    {"isEqualToString:", cfstr_is_equal},
    {"getCString:maxLength:encoding:", cfstr_get_cstring},
};

/* ---------------- NSNumber ---------------- */

static id nsnum_number_with_bool(id self, SEL sel, int v) {
    (void)self; (void)sel;
    nsnumber_t *o = (nsnumber_t *)calloc(1, sizeof(nsnumber_t));
    o->isa = g_NSNumber;
    o->value = v ? 1 : 0;
    return (id)o;
}
static int nsnum_bool_value(id self, SEL sel) {
    (void)sel;
    return (int)((nsnumber_t *)self)->value;
}
static const objc_method_def_t nsnum_methods[] = {
    {"boolValue", nsnum_bool_value}, {"intValue", nsnum_bool_value},
};
static const objc_method_def_t nsnum_cmethods[] = {
    {"numberWithBool:", nsnum_number_with_bool},
};

/* ---------------- NSURL ---------------- */

static id nsurl_url_with_string(id self, SEL sel, id str) {
    (void)self; (void)sel;
    nsurl_t *o = (nsurl_t *)calloc(1, sizeof(nsurl_t));
    o->isa = g_NSURL;
    o->path = strdup(str ? nsstr_utf8(str, NULL) : "");
    return (id)o;
}
static id nsurl_init_with_utf8(id self, SEL sel, const char *s) {
    (void)sel;
    ((nsurl_t *)self)->path = strdup(s ? s : "");
    return self;
}
static id nsurl_init_file_path(id self, SEL sel, id s) {
    (void)sel;
    ((nsurl_t *)self)->path = strdup(s ? nsstr_utf8(s, NULL) : "");
    return self;
}
static const char *nsurl_path(id self, SEL sel) {
    (void)sel;
    return ((nsurl_t *)self)->path ? ((nsurl_t *)self)->path : "";
}
static const objc_method_def_t nsurl_methods[] = {
    {"initWithUTF8String:", nsurl_init_with_utf8},
    {"initFileURLWithPath:", nsurl_init_file_path},
    {"path", nsurl_path},
};
static const objc_method_def_t nsurl_cmethods[] = {
    {"URLWithString:", nsurl_url_with_string},
    {"fileURLWithPath:", nsurl_url_with_string},
};

/* ---------------- NSDictionary ---------------- */

static id nsdict_dict_with_objects(id self, SEL sel, ...) {
    (void)self; (void)sel;
    return objc_alloc_instance(g_NSDictionary);
}
static const objc_method_def_t nsdict_cmethods[] = {
    {"dictionaryWithObjectsAndKeys:", nsdict_dict_with_objects},
};

/* ---------------- NSArray / NSEnumerator ---------------- */

id redeg_make_nsarray1(id item) {
    nsarray_t *a = (nsarray_t *)calloc(1, sizeof(nsarray_t));
    a->isa = g_NSArray;
    a->items[0] = item;
    a->count = 1;
    return (id)a;
}

static id nsarray_object_at(id self, SEL sel, unsigned idx) {
    (void)sel;
    nsarray_t *a = (nsarray_t *)self;
    return idx < (unsigned)a->count ? a->items[idx] : NULL;
}
static long nsarray_count(id self, SEL sel) {
    (void)sel;
    return ((nsarray_t *)self)->count;
}
static id nsarray_object_enumerator(id self, SEL sel) {
    (void)sel;
    nsenum_t *e = (nsenum_t *)calloc(1, sizeof(nsenum_t));
    e->isa = g_NSEnumerator;
    e->array = (nsarray_t *)self;
    e->idx = 0;
    return (id)e;
}
static const objc_method_def_t nsarray_methods[] = {
    {"objectAtIndex:", nsarray_object_at},
    {"count", nsarray_count},
    {"objectEnumerator", nsarray_object_enumerator},
};

static id nsenum_next_object(id self, SEL sel) {
    (void)sel;
    nsenum_t *e = (nsenum_t *)self;
    if (!e->array || e->idx >= e->array->count) return NULL;
    return e->array->items[e->idx++];
}
static const objc_method_def_t nsenum_methods[] = {
    {"nextObject", nsenum_next_object},
};

/* ---------------- NSNotificationCenter ---------------- */

static id g_notification_center = NULL;
static id nsnc_default(id self, SEL sel) {
    (void)self; (void)sel;
    if (!g_notification_center) g_notification_center = objc_alloc_instance(g_NSNotificationCenter);
    return g_notification_center;
}
static void nsnc_add(id self, SEL sel, id observer, SEL aSelector, id name, id object) {
    (void)self; (void)sel; (void)observer; (void)aSelector; (void)name; (void)object;
}
static void nsnc_remove(id self, SEL sel, id observer, id name, id object) {
    (void)self; (void)sel; (void)observer; (void)name; (void)object;
}
static const objc_method_def_t nsnc_methods[] = {
    {"addObserver:selector:name:object:", nsnc_add},
    {"removeObserver:name:object:", nsnc_remove},
};
static const objc_method_def_t nsnc_cmethods[] = {
    {"defaultCenter", nsnc_default},
};

const char *redeg_string_cstr(id s) {
    if (!s) return "(null)";
    if (s->isa == g_NSCFConstantString) return *(const char **)((const char *)s + 8);
    if (s->isa == g_NSString) return ((nsstring_t *)s)->str;
    return "(not-a-string)";
}

/* ---------------- NSLog ---------------- */

void NSLog(id fmt, ...) {
    char out[1024];
    const char *s = "";
    if (fmt) {
        if (fmt->isa == g_NSCFConstantString) {
            s = cfstr_utf8(fmt, NULL);
        } else if (fmt->isa == g_NSString) {
            s = nsstr_utf8(fmt, NULL);
        } else if (fmt->isa && fmt->isa->ro && fmt->isa->ro->name &&
                   strcmp(fmt->isa->ro->name, "NSString") == 0) {
            s = ((nsstring_t *)fmt)->str;
        } else {
            s = (const char *)fmt;
        }
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, sizeof(out), s ? s : "", ap);
    va_end(ap);
    l_info("[NSLog] %s", out);
}

static id nslocale_preferred_languages(id self, SEL sel) {
    (void)self; (void)sel;
    return redeg_make_nsarray1(redeg_nsstring_new("en"));
}
static const objc_method_def_t nslocale_cmethods[] = {
    {"preferredLanguages", nslocale_preferred_languages},
};

/* ---------------- init ---------------- */

void objc_foundation_init(void) {
    objc_create_root_class("NSObject", 4,
        nso_methods, sizeof(nso_methods)/sizeof(nso_methods[0]),
        nso_cmethods, sizeof(nso_cmethods)/sizeof(nso_cmethods[0]));

    g_NSString = objc_define_class("NSString", NULL, (uint32_t)sizeof(nsstring_t),
        nsstr_methods, sizeof(nsstr_methods)/sizeof(nsstr_methods[0]),
        nsstr_cmethods, sizeof(nsstr_cmethods)/sizeof(nsstr_cmethods[0]));
    g_NSCFConstantString = objc_define_class("__NSCFConstantString", g_NSString, 16,
        cfstr_methods, sizeof(cfstr_methods)/sizeof(cfstr_methods[0]), NULL, 0);
    g_NSNumber = objc_define_class("NSNumber", NULL, (uint32_t)sizeof(nsnumber_t),
        nsnum_methods, sizeof(nsnum_methods)/sizeof(nsnum_methods[0]),
        nsnum_cmethods, sizeof(nsnum_cmethods)/sizeof(nsnum_cmethods[0]));
    g_NSURL = objc_define_class("NSURL", NULL, (uint32_t)sizeof(nsurl_t),
        nsurl_methods, sizeof(nsurl_methods)/sizeof(nsurl_methods[0]),
        nsurl_cmethods, sizeof(nsurl_cmethods)/sizeof(nsurl_cmethods[0]));
    g_NSDictionary = objc_define_class("NSDictionary", NULL, 8,
        NULL, 0, nsdict_cmethods, sizeof(nsdict_cmethods)/sizeof(nsdict_cmethods[0]));
    g_NSArray = objc_define_class("NSArray", NULL, (uint32_t)sizeof(nsarray_t),
        nsarray_methods, sizeof(nsarray_methods)/sizeof(nsarray_methods[0]), NULL, 0);
    g_NSEnumerator = objc_define_class("NSEnumerator", NULL, (uint32_t)sizeof(nsenum_t),
        nsenum_methods, sizeof(nsenum_methods)/sizeof(nsenum_methods[0]), NULL, 0);
    g_NSNotificationCenter = objc_define_class("NSNotificationCenter", NULL, 8,
        nsnc_methods, sizeof(nsnc_methods)/sizeof(nsnc_methods[0]),
        nsnc_cmethods, sizeof(nsnc_cmethods)/sizeof(nsnc_cmethods[0]));
    g_NSAutoreleasePool = objc_define_class("NSAutoreleasePool", NULL, 8,
        pool_methods, sizeof(pool_methods)/sizeof(pool_methods[0]), NULL, 0);
    g_NSLocale = objc_define_class("NSLocale", NULL, 8,
        NULL, 0, nslocale_cmethods, sizeof(nslocale_cmethods)/sizeof(nslocale_cmethods[0]));
}
