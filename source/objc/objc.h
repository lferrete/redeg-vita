#ifndef REDEG_OBJC_H
#define REDEG_OBJC_H

#include <stdint.h>
#include <stddef.h>

typedef struct objc_class *Class;
typedef const char *SEL;
typedef void (*IMP)(void);
typedef struct objc_object { Class isa; } *id;

typedef struct objc_method {
    SEL name;
    const char *types;
    IMP imp;
} objc_method_t;

typedef struct objc_method_list {
    uint32_t entsizeAndFlags;
    uint32_t count;
    objc_method_t methods[1];
} objc_method_list_t;

typedef struct class_ro {
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;
    const uint8_t *ivarLayout;
    const char *name;
    const objc_method_list_t *baseMethods;
    const void *baseProtocols;
    const void *ivars;
    const uint8_t *weakIvarLayout;
    const void *baseProperties;
} class_ro_t;

typedef struct objc_class {
    Class isa;
    Class superclass;
    uint32_t cacheBuckets;
    uint32_t cacheMask;
    class_ro_t *ro;
} objc_class_t;

typedef struct {
    const char *sel;
    void *imp;
} objc_method_def_t;

/* ABI-compatible with the binary's objc_super (32-bit). */
struct objc_super {
    id receiver;
    Class cls;
};

void objc_runtime_init(void);
Class objc_create_root_class(const char *name, uint32_t instance_size,
                             const objc_method_def_t *methods, int n_methods,
                             const objc_method_def_t *cmethods, int n_cmethods);
Class objc_define_class(const char *name, Class super, uint32_t instance_size,
                        const objc_method_def_t *methods, int n_methods,
                        const objc_method_def_t *cmethods, int n_cmethods);
Class objc_get_class(const char *name);
void objc_register_binary_classes(uint32_t classlist, int count);
IMP objc_lookup_msg(id self, SEL sel);
IMP objc_lookup_super2(struct objc_super *super, SEL sel);
id objc_alloc_instance(Class cls);

/* Entry points expected by the binary (Mach-O names lose the leading '_'). */
id objc_msgSend(id self, SEL sel, ...);
id objc_msgSendSuper2(struct objc_super *super, SEL sel, ...);
void objc_msgSend_stret(void *stret, id self, SEL sel, ...);
void objc_msgSendSuper2_stret(void *stret, struct objc_super *super, SEL sel, ...);
void objc_setProperty(id self, SEL sel, long offset, id value, int atomic, int should_copy);

extern Class g_nsobject_class;
extern Class g_nsobject_meta;

/* Simple object helpers used by the fake classes. */
id objc_new(Class cls);
void objc_set_class_name(Class cls, const char *name);

#endif
