#include "objc/objc_classes.h"
#include "utils/logger.h"
#include "utils/glutil.h"

#include <string.h>

Class g_EAGLContext, g_CAEAGLLayer;

static id g_current_ctx = NULL;

static id eagl_alloc(id self, SEL sel) {
    (void)sel;
    return objc_alloc_instance((Class)self);
}
static id eagl_init_api(id self, SEL sel, int api) {
    (void)sel;
    l_info("[EAGL] initWithAPI %d -> %p", api, (void *)self);
    return self;
}
static int eagl_set_current(id self, SEL sel, id ctx) {
    (void)self; (void)sel;
    static int n = 0;
    if (n++ < 8) l_info("[EAGL] setCurrentContext:%p", (void *)ctx);
    g_current_ctx = ctx;
    return 1;
}
static id eagl_get_current(id self, SEL sel) {
    (void)self; (void)sel;
    return g_current_ctx;
}
static int eagl_renderbuffer_storage(id self, SEL sel, unsigned target, id drawable) {
    (void)self; (void)sel;
    static int n = 0;
    if (n++ < 8) l_info("[EAGL] renderbufferStorage:0x%x fromDrawable:%p", target, (void *)drawable);
    return 1;
}
static int eagl_present(id self, SEL sel, unsigned target) {
    (void)self; (void)sel; (void)target;
    gl_swap();
    return 1;
}

static const objc_method_def_t eagl_methods[] = {
    {"initWithAPI:", eagl_init_api},
    {"renderbufferStorage:fromDrawable:", eagl_renderbuffer_storage},
    {"presentRenderbuffer:", eagl_present},
};
static const objc_method_def_t eagl_cmethods[] = {
    {"alloc", eagl_alloc},
    {"setCurrentContext:", eagl_set_current},
    {"currentContext", eagl_get_current},
};

static void caeagl_set_props(id self, SEL sel, id props) { (void)self; (void)sel; (void)props; }
static void caeagl_set_opaque(id self, SEL sel, int opaque) { (void)self; (void)sel; (void)opaque; }

static const objc_method_def_t caeagl_methods[] = {
    {"setDrawableProperties:", caeagl_set_props},
    {"setOpaque:", caeagl_set_opaque},
};

void objc_egl_init(void) {
    g_EAGLContext = objc_define_class("EAGLContext", NULL, 8,
        eagl_methods, sizeof(eagl_methods)/sizeof(eagl_methods[0]),
        eagl_cmethods, sizeof(eagl_cmethods)/sizeof(eagl_cmethods[0]));
    g_CAEAGLLayer = objc_define_class("CAEAGLLayer", NULL, 8,
        caeagl_methods, sizeof(caeagl_methods)/sizeof(caeagl_methods[0]), NULL, 0);

    kEAGLDrawablePropertyColorFormat = redeg_nsstring_new("kEAGLDrawablePropertyColorFormat");
    kEAGLDrawablePropertyRetainedBacking = redeg_nsstring_new("kEAGLDrawablePropertyRetainedBacking");
    kEAGLColorFormatRGBA8 = redeg_nsstring_new("kEAGLColorFormatRGBA8");
}
