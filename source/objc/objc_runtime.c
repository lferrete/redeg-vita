#include "objc/objc.h"
#include "utils/logger.h"

#include <stdlib.h>
#include <string.h>

Class g_nsobject_class = NULL;
Class g_nsobject_meta = NULL;

typedef struct {
    Class cls;
    char name[96];
} class_entry_t;

static class_entry_t s_classes[256];
static int s_nclasses = 0;

#define CACHE_SIZE 4096
typedef struct {
    Class cls;
    SEL sel;
    IMP imp;
} cache_entry_t;
static cache_entry_t s_cache[CACHE_SIZE];

extern void redeg_process_touches(id self, SEL sel);

static IMP lookup_in_list(const objc_method_list_t *ml, SEL sel) {
    if (!ml) return NULL;
    if (sel && strcmp(sel, "processTouches") == 0)
        return (IMP)redeg_process_touches;
    uint32_t entsize = ml->entsizeAndFlags & ~3u;
    if (entsize < 12) entsize = 12;
    uint32_t count = ml->count;
    if (count > 4096) return NULL;
    const uint8_t *base = (const uint8_t *)ml + 8;
    for (uint32_t i = 0; i < count; i++) {
        const objc_method_t *m = (const objc_method_t *)(base + i * entsize);
        if (!m->name || !m->imp) continue;
        if (m->name == sel || strcmp(m->name, sel) == 0)
            return m->imp;
    }
    return NULL;
}

static IMP lookup_class(Class cls, SEL sel) {
    for (Class c = cls; c; c = c->superclass) {
        if (!c->ro) continue;
        IMP imp = lookup_in_list(c->ro->baseMethods, sel);
        if (imp) return imp;
    }
    return NULL;
}

static id objc_return_nil(void) { return NULL; }

IMP objc_lookup_msg(id self, SEL sel) {
    if (!self) return (IMP)objc_return_nil;
    Class cls = self->isa;
    if (!cls || !sel) return (IMP)objc_return_nil;

    unsigned idx = (((uintptr_t)cls >> 2) ^ ((uintptr_t)sel >> 2)) & (CACHE_SIZE - 1);
    cache_entry_t *ce = &s_cache[idx];
    if (ce->cls == cls && ce->sel == sel)
        return ce->imp;

    IMP imp = lookup_class(cls, sel);
    if (!imp) {
        static int logged = 0;
        if (logged < 96) {
            l_warn("[objc] no method '%s' on class %s (obj=%p)", sel,
                   (cls->ro && cls->ro->name) ? cls->ro->name : "?", (void *)self);
            logged++;
        }
        imp = (IMP)objc_return_nil;
    }
    ce->cls = cls;
    ce->sel = sel;
    ce->imp = imp;
    return imp;
}

IMP objc_lookup_super2(struct objc_super *super, SEL sel) {
    if (!super || !super->receiver) return (IMP)objc_return_nil;
    Class start = super->cls ? super->cls->superclass : super->receiver->isa->superclass;
    IMP imp = lookup_class(start, sel);
    if (!imp) {
        l_warn("[objc] no super method '%s' (cls=%s)", sel,
               (super->cls && super->cls->ro && super->cls->ro->name) ? super->cls->ro->name : "?");
        imp = (IMP)objc_return_nil;
    }
    return imp;
}

__attribute__((naked)) id objc_msgSend(id self, SEL sel, ...) {
    __asm__ volatile(
        "push {r0-r3, r9-r11, lr}\n"
        "bl   objc_lookup_msg\n"
        "mov  r12, r0\n"
        "pop  {r0-r3, r9-r11, lr}\n"
        "bx   r12\n"
    );
}

__attribute__((naked)) id objc_msgSendSuper2(struct objc_super *super, SEL sel, ...) {
    __asm__ volatile(
        "push {r0-r3, r9-r11, lr}\n"
        "bl   objc_lookup_super2\n"
        "mov  r12, r0\n"
        "pop  {r0-r3, r9-r11, lr}\n"
        "ldr  r0, [r0]\n"
        "bx   r12\n"
    );
}

__attribute__((naked)) void objc_msgSend_stret(void *stret, id self, SEL sel, ...) {
    __asm__ volatile(
        "push {r0-r3, r9-r11, lr}\n"
        "mov  r0, r1\n"
        "mov  r1, r2\n"
        "bl   objc_lookup_msg\n"
        "mov  r12, r0\n"
        "pop  {r0-r3, r9-r11, lr}\n"
        "bx   r12\n"
    );
}

__attribute__((naked)) void objc_msgSendSuper2_stret(void *stret, struct objc_super *super, SEL sel, ...) {
    __asm__ volatile(
        "push {r0-r3, r9-r11, lr}\n"
        "mov  r0, r1\n"
        "mov  r1, r2\n"
        "bl   objc_lookup_super2\n"
        "mov  r12, r0\n"
        "pop  {r0-r3, r9-r11, lr}\n"
        "ldr  r1, [r1]\n"
        "bx   r12\n"
    );
}

void objc_setProperty(id self, SEL sel, long offset, id value, int atomic, int should_copy) {
    (void)sel; (void)atomic; (void)should_copy;
    if (!self) return;
    *(id *)((char *)self + offset) = value;
}

static objc_method_list_t *build_method_list(const objc_method_def_t *defs, int n) {
    if (!defs || n <= 0) return NULL;
    objc_method_list_t *ml = (objc_method_list_t *)calloc(1,
        sizeof(objc_method_list_t) + (size_t)(n - 1) * sizeof(objc_method_t));
    if (!ml) return NULL;
    ml->entsizeAndFlags = sizeof(objc_method_t);
    ml->count = (uint32_t)n;
    for (int i = 0; i < n; i++) {
        ml->methods[i].name = defs[i].sel;
        ml->methods[i].types = "";
        ml->methods[i].imp = (IMP)defs[i].imp;
    }
    return ml;
}

static Class alloc_class_struct(const char *name, Class super, uint32_t inst_size,
                                const objc_method_def_t *methods, int n,
                                const objc_method_def_t *cmethods, int cn,
                                int is_root) {
    objc_class_t *cls = (objc_class_t *)calloc(1, sizeof(objc_class_t));
    objc_class_t *meta = (objc_class_t *)calloc(1, sizeof(objc_class_t));
    class_ro_t *ro = (class_ro_t *)calloc(1, sizeof(class_ro_t));
    class_ro_t *mro = (class_ro_t *)calloc(1, sizeof(class_ro_t));
    if (!cls || !meta || !ro || !mro) return NULL;

    cls->isa = meta;
    cls->superclass = super;
    cls->ro = ro;
    ro->name = name;
    ro->instanceSize = inst_size;
    ro->instanceStart = super ? super->ro->instanceSize : 0;
    ro->baseMethods = build_method_list(methods, n);

    meta->isa = is_root ? meta : g_nsobject_meta;
    meta->superclass = is_root ? cls : (super ? super->isa : NULL);
    meta->ro = mro;
    mro->name = name;
    mro->instanceSize = inst_size;
    mro->instanceStart = ro->instanceStart;
    mro->baseMethods = build_method_list(cmethods, cn);

    return (Class)cls;
}

void objc_runtime_init(void) {
    memset(s_cache, 0, sizeof(s_cache));
    memset(s_classes, 0, sizeof(s_classes));
    s_nclasses = 0;
}

Class objc_create_root_class(const char *name, uint32_t instance_size,
                             const objc_method_def_t *methods, int n_methods,
                             const objc_method_def_t *cmethods, int n_cmethods) {
    Class c = alloc_class_struct(name, NULL, instance_size, methods, n_methods,
                                 cmethods, n_cmethods, 1);
    g_nsobject_class = c;
    g_nsobject_meta = c ? c->isa : NULL;
    if (c && s_nclasses < (int)(sizeof(s_classes) / sizeof(s_classes[0]))) {
        s_classes[s_nclasses].cls = c;
        strncpy(s_classes[s_nclasses].name, name, sizeof(s_classes[s_nclasses].name) - 1);
        s_nclasses++;
    }
    return c;
}

Class objc_define_class(const char *name, Class super, uint32_t instance_size,
                        const objc_method_def_t *methods, int n_methods,
                        const objc_method_def_t *cmethods, int n_cmethods) {
    if (!g_nsobject_class) return NULL;
    if (!super) super = g_nsobject_class;
    Class c = alloc_class_struct(name, super, instance_size, methods, n_methods,
                                 cmethods, n_cmethods, 0);
    if (!c) return NULL;
    if (s_nclasses < (int)(sizeof(s_classes) / sizeof(s_classes[0]))) {
        s_classes[s_nclasses].cls = c;
        strncpy(s_classes[s_nclasses].name, name, sizeof(s_classes[s_nclasses].name) - 1);
        s_nclasses++;
    }
    return c;
}

Class objc_get_class(const char *name) {
    for (int i = 0; i < s_nclasses; i++)
        if (strcmp(s_classes[i].name, name) == 0)
            return s_classes[i].cls;
    return NULL;
}

void objc_register_binary_classes(uint32_t classlist, int count) {
    const uint32_t *list = (const uint32_t *)classlist;
    for (int i = 0; i < count; i++) {
        Class c = (Class)(uintptr_t)list[i];
        if (!c) continue;
        if (c->isa) c->isa->isa = g_nsobject_meta;
        const char *name = (c->ro && c->ro->name) ? c->ro->name : "?";
        if (s_nclasses < (int)(sizeof(s_classes) / sizeof(s_classes[0]))) {
            s_classes[s_nclasses].cls = c;
            strncpy(s_classes[s_nclasses].name, name, sizeof(s_classes[s_nclasses].name) - 1);
            s_nclasses++;
        }
        l_info("[objc] binary class %s @%p super=%p meta=%p", name, (void *)c,
               (void *)c->superclass, (void *)c->isa);
    }
}

id objc_alloc_instance(Class cls) {
    if (!cls || !cls->ro) return NULL;
    uint32_t sz = cls->ro->instanceSize;
    if (sz < 4) sz = 4;
    id obj = (id)calloc(1, sz);
    static int n = 0;
    if (n++ < 30) l_info("[alloc] %s size=%u -> %p", cls->ro->name ? cls->ro->name : "?", sz, (void *)obj);
    if (obj) obj->isa = cls;
    return obj;
}

id objc_new(Class cls) { return objc_alloc_instance(cls); }

void objc_set_class_name(Class cls, const char *name) {
    if (cls && cls->ro) cls->ro->name = name;
}
