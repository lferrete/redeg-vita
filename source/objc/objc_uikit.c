#include "objc/objc_classes.h"
#include "objc/cgttypes.h"
#include "macho/macho.h"
#include "utils/logger.h"

#include <psp2/kernel/threadmgr.h>
#include <psp2/touch.h>
#include <psp2/ctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static SceUID s_ui_thread = -1;

Class g_UIApplication, g_UIWindow, g_UIView, g_UIViewController;
Class g_UIScreen, g_UIDevice, g_UIColor, g_UIImage, g_UIImageView;
Class g_UIAccelerometer, g_UIAlertView, g_UITextField;

id redeg_nsstring_new(const char *s);
id redeg_make_nsarray1(id item);

#define MAX_VIEWS 96

typedef struct {
    id obj;
    float x, y, w, h;
    int hidden;
    int opaque;
    int clears;
    int multitouch;
    int needs_display;
    float transform[6];
    id superview;
    id subviews[24];
    int nsub;
    id layer;
} view_state_t;

static view_state_t s_views[MAX_VIEWS];
static int s_nviews = 0;

static view_state_t *view_get(id obj, int create) {
    for (int i = 0; i < s_nviews; i++)
        if (s_views[i].obj == obj) return &s_views[i];
    if (!create || s_nviews >= MAX_VIEWS) return NULL;
    view_state_t *v = &s_views[s_nviews++];
    memset(v, 0, sizeof(*v));
    v->obj = obj;
    v->w = 480; v->h = 320;
    v->transform[0] = 1; v->transform[4] = 1;
    return v;
}

/* ---------------- UIView ---------------- */

static id uiv_init_with_frame(id self, SEL sel, float x, float y, float w, float h) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) { v->x = x; v->y = y; v->w = w; v->h = h; }
    register uint32_t fp asm("r11");
    l_info("[UIView] initWithFrame self=%p frame=%d,%d %dx%d caller=0x%08x fp=0x%08x",
           (void *)self, (int)x, (int)y, (int)w, (int)h,
           (uint32_t)(uintptr_t)__builtin_return_address(0), fp);
    return self;
}
static redeg_cgrect_t uiv_bounds(id self, SEL sel) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    redeg_cgrect_t r = {{0, 0}, {v ? v->w : 480, v ? v->h : 320}};
    return r;
}
static void uiv_add_subview(id self, SEL sel, id child) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v && child) {
        if (v->nsub < (int)(sizeof(v->subviews)/sizeof(v->subviews[0])))
            v->subviews[v->nsub++] = child;
        view_state_t *c = view_get(child, 1);
        if (c) c->superview = self;
    }
    l_info("[UIView] addSubview %p -> %p", (void *)child, (void *)self);
}
static void uiv_remove_from_super(id self, SEL sel) {
    (void)sel;
    view_state_t *c = view_get(self, 0);
    if (c && c->superview) {
        view_state_t *p = view_get(c->superview, 0);
        if (p) {
            for (int i = 0; i < p->nsub; i++) {
                if (p->subviews[i] == self) {
                    for (int j = i; j < p->nsub - 1; j++) p->subviews[j] = p->subviews[j + 1];
                    p->nsub--;
                    break;
                }
            }
        }
        c->superview = NULL;
    }
}
static void uiv_set_hidden(id self, SEL sel, int hidden) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) v->hidden = hidden;
}
static int uiv_is_hidden(id self, SEL sel) {
    (void)sel;
    view_state_t *v = view_get(self, 0);
    return v ? v->hidden : 0;
}
static void uiv_set_opaque(id self, SEL sel, int opaque) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) v->opaque = opaque;
}
static void uiv_set_bg(id self, SEL sel, id color) { (void)self; (void)sel; (void)color; }
static void uiv_set_clears(id self, SEL sel, int val) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) v->clears = val;
}
static void uiv_set_multitouch(id self, SEL sel, int val) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) v->multitouch = val;
}
static void uiv_set_transform(id self, SEL sel, float a, float b, float c, float d, float tx, float ty) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) {
        v->transform[0] = a; v->transform[1] = b; v->transform[2] = c;
        v->transform[3] = d; v->transform[4] = tx; v->transform[5] = ty;
    }
    l_info("[UIView] setTransform %p = [%.3f %.3f %.3f %.3f %.1f %.1f]",
           (void *)self, a, b, c, d, tx, ty);
}
static void uiv_set_needs_display(id self, SEL sel) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v) v->needs_display = 1;
}
static id uiv_layer(id self, SEL sel) {
    (void)sel;
    view_state_t *v = view_get(self, 1);
    if (v && !v->layer)
        v->layer = objc_alloc_instance(g_CAEAGLLayer);
    return v ? v->layer : NULL;
}

static const objc_method_def_t uiv_methods[] = {
    {"initWithFrame:", uiv_init_with_frame},
    {"bounds", uiv_bounds},
    {"addSubview:", uiv_add_subview},
    {"removeFromSuperview", uiv_remove_from_super},
    {"setHidden:", uiv_set_hidden},
    {"isHidden", uiv_is_hidden},
    {"setOpaque:", uiv_set_opaque},
    {"setBackgroundColor:", uiv_set_bg},
    {"setClearsContextBeforeDrawing:", uiv_set_clears},
    {"setMultipleTouchEnabled:", uiv_set_multitouch},
    {"setTransform:", uiv_set_transform},
    {"setNeedsDisplay", uiv_set_needs_display},
    {"layer", uiv_layer},
};

/* ---------------- UIWindow ---------------- */

static id g_key_window = NULL;
static void uiw_make_key(id self, SEL sel) {
    (void)sel;
    g_key_window = self;
    l_info("[UIWindow] makeKeyAndVisible %p", (void *)self);
}
static const objc_method_def_t uiw_methods[] = {
    {"makeKeyAndVisible", uiw_make_key},
};

/* ---------------- UIScreen ---------------- */

static id g_screen = NULL;
static id uiscreen_main(id self, SEL sel) {
    (void)self; (void)sel;
    if (!g_screen) g_screen = objc_alloc_instance(g_UIScreen);
    return g_screen;
}
static redeg_cgrect_t uiscreen_bounds(id self, SEL sel) {
    (void)self; (void)sel;
    redeg_cgrect_t r = {{0, 0}, {480, 320}};
    l_info("[UIScreen] bounds -> %d x %d", (int)r.size.w, (int)r.size.h);
    return r;
}
static redeg_cgrect_t uiscreen_appframe(id self, SEL sel) {
    (void)self; (void)sel;
    redeg_cgrect_t r = {{0, 0}, {480, 320}};
    l_info("[UIScreen] applicationFrame -> %d x %d", (int)r.size.w, (int)r.size.h);
    return r;
}
static const objc_method_def_t uiscreen_methods[] = {
    {"bounds", uiscreen_bounds}, {"applicationFrame", uiscreen_appframe},
};
static const objc_method_def_t uiscreen_cmethods[] = {
    {"mainScreen", uiscreen_main},
};

/* ---------------- UIColor ---------------- */

static id uicolor_black(id self, SEL sel) {
    (void)self; (void)sel;
    static id o = NULL;
    if (!o) o = objc_alloc_instance(g_UIColor);
    return o;
}
static const objc_method_def_t uicolor_cmethods[] = {
    {"blackColor", uicolor_black}, {"whiteColor", uicolor_black},
};

/* ---------------- UIImage / UIImageView ---------------- */

static id uiimage_named(id self, SEL sel, id name) {
    (void)self; (void)sel; (void)name;
    return objc_alloc_instance(g_UIImage);
}
static id uiimage_init_cgimage(id self, SEL sel, void *cgimage) {
    (void)sel; (void)cgimage;
    return self;
}
static void uiimage_draw_in_rect(id self, SEL sel, redeg_cgrect_t r) {
    (void)self; (void)sel; (void)r;
}
static void uiimage_draw_at_point(id self, SEL sel, redeg_cgpoint_t p) {
    (void)self; (void)sel; (void)p;
}
static void uiimage_draw_in_rect_blend(id self, SEL sel, redeg_cgrect_t r, int mode, float alpha) {
    (void)self; (void)sel; (void)r; (void)mode; (void)alpha;
}
static const objc_method_def_t uiimage_methods[] = {
    {"initWithCGImage:", uiimage_init_cgimage},
    {"drawInRect:", uiimage_draw_in_rect},
    {"drawAtPoint:", uiimage_draw_at_point},
    {"drawInRect:blendMode:alpha:", uiimage_draw_in_rect_blend},
};
static const objc_method_def_t uiimage_cmethods[] = {
    {"imageNamed:", uiimage_named},
    {"imageWithCGImage:", uiimage_named},
};

static id uiiv_init_with_image(id self, SEL sel, id image) {
    (void)sel; (void)image;
    return self;
}
static void uiiv_set_image(id self, SEL sel, id image) { (void)self; (void)sel; (void)image; }
static const objc_method_def_t uiiv_methods[] = {
    {"initWithImage:", uiiv_init_with_image},
    {"setImage:", uiiv_set_image},
};

/* ---------------- UIDevice ---------------- */

static id g_device = NULL;
static id uidev_current(id self, SEL sel) {
    (void)self; (void)sel;
    if (!g_device) g_device = objc_alloc_instance(g_UIDevice);
    return g_device;
}
static id uidev_system_name(id self, SEL sel) { (void)self; (void)sel; return redeg_nsstring_new("iPhone OS"); }
static id uidev_system_version(id self, SEL sel) { (void)self; (void)sel; return redeg_nsstring_new("2.2.1"); }
static id uidev_model(id self, SEL sel) { (void)self; (void)sel; return redeg_nsstring_new("iPhone"); }
static id uidev_name(id self, SEL sel) { (void)self; (void)sel; return redeg_nsstring_new("iPhone"); }
static id uidev_languages(id self, SEL sel) {
    (void)self; (void)sel;
    return redeg_make_nsarray1(redeg_nsstring_new("en"));
}
static int uidev_orientation(id self, SEL sel) { (void)self; (void)sel; return 3; }
static void uidev_begin_notify(id self, SEL sel) { (void)self; (void)sel; }
static void uidev_end_notify(id self, SEL sel) { (void)self; (void)sel; }
static int uidev_is_notifying(id self, SEL sel) { (void)self; (void)sel; return 1; }
static const objc_method_def_t uidev_methods[] = {
    {"systemName", uidev_system_name},
    {"systemVersion", uidev_system_version},
    {"model", uidev_model},
    {"name", uidev_name},
    {"preferredLanguages", uidev_languages},
    {"orientation", uidev_orientation},
    {"beginGeneratingDeviceOrientationNotifications", uidev_begin_notify},
    {"endGeneratingDeviceOrientationNotifications", uidev_end_notify},
    {"isGeneratingDeviceOrientationNotifications", uidev_is_notifying},
};
static const objc_method_def_t uidev_cmethods[] = {
    {"currentDevice", uidev_current},
};

/* ---------------- UIAccelerometer ---------------- */

static id g_accel = NULL;
static id accel_shared(id self, SEL sel) {
    (void)self; (void)sel;
    if (!g_accel) g_accel = objc_alloc_instance(g_UIAccelerometer);
    return g_accel;
}
static void accel_set_interval(id self, SEL sel, double interval) { (void)self; (void)sel; (void)interval; }
static void accel_set_delegate(id self, SEL sel, id d) { (void)self; (void)sel; (void)d; }
static const objc_method_def_t accel_methods[] = {
    {"setUpdateInterval:", accel_set_interval},
    {"setDelegate:", accel_set_delegate},
};
static const objc_method_def_t accel_cmethods[] = {
    {"sharedAccelerometer", accel_shared},
};

/* ---------------- UIAlertView ---------------- */

static void alert_set_title(id self, SEL sel, id s) { (void)self; (void)sel; l_warn("[UIAlertView] title=%s", redeg_string_cstr(s)); }
static void alert_set_message(id self, SEL sel, id s) { (void)self; (void)sel; l_warn("[UIAlertView] message=%s", redeg_string_cstr(s)); }
static int alert_add_button(id self, SEL sel, id s) { (void)self; (void)sel; (void)s; return 0; }
static id s_alert_delegate = NULL;
extern void redeg_s3e_probe(void);
int redeg_alert_shown = 0;
/* The engine shows an alert and *then* waits on _g_DialogCond. Calling the
 * delegate synchronously here runs on the same loader thread and loses the
 * wakeup (signal before wait). Defer the dismiss to the main/UI tick. */
static volatile int s_alert_pending = 0;
static id s_alert_pending_view = NULL;
static void alert_show(id self, SEL sel) {
    (void)sel;
    redeg_alert_shown = 1;
    l_warn("[s3e] at-alert probe");
    redeg_s3e_probe();
    l_warn("[UIAlertView] show %p (deferred main-thread dismiss)", (void *)self);
    s_alert_pending_view = self;
    s_alert_pending = 1;
}
static void alert_set_delegate(id self, SEL sel, id d) { (void)self; (void)sel; s_alert_delegate = d; }
static const objc_method_def_t alert_methods[] = {
    {"setTitle:", alert_set_title}, {"setMessage:", alert_set_message},
    {"addButtonWithTitle:", alert_add_button}, {"show", alert_show},
    {"setDelegate:", alert_set_delegate},
};

/* ---------------- UITextField ---------------- */

static int tf_become_first(id self, SEL sel) { (void)self; (void)sel; return 1; }
static int tf_resign_first(id self, SEL sel) { (void)self; (void)sel; return 1; }
static id tf_text(id self, SEL sel) { (void)self; (void)sel; return redeg_nsstring_new(""); }
static const objc_method_def_t tf_methods[] = {
    {"becomeFirstResponder", tf_become_first},
    {"resignFirstResponder", tf_resign_first},
    {"text", tf_text},
};

/* ---------------- UIApplication ---------------- */

static id g_app = NULL;
static id g_delegate = NULL;

static id app_shared(id self, SEL sel) {
    (void)self; (void)sel;
    if (!g_app) g_app = objc_alloc_instance(g_UIApplication);
    return g_app;
}
static void app_set_delegate(id self, SEL sel, id d) { (void)self; (void)sel; g_delegate = d; }
static void app_status_hidden(id self, SEL sel, int hidden, int animated) {
    (void)self; (void)sel; (void)hidden; (void)animated;
}
static void app_status_orient(id self, SEL sel, int orient, int animated) {
    (void)self; (void)sel; (void)orient; (void)animated;
}
static int app_open_url(id self, SEL sel, id url) {
    (void)self; (void)sel; (void)url; return 1;
}
static void app_begin_orientation(id self, SEL sel) { (void)self; (void)sel; }
static void app_idle_disabled(id self, SEL sel, int disabled) { (void)self; (void)sel; (void)disabled; }
static const objc_method_def_t app_methods[] = {
    {"setDelegate:", app_set_delegate},
    {"setStatusBarHidden:animated:", app_status_hidden},
    {"setStatusBarOrientation:animated:", app_status_orient},
    {"openURL:", app_open_url},
    {"beginGeneratingDeviceOrientationNotifications", app_begin_orientation},
    {"setIdleTimerDisabled:", app_idle_disabled},
};
static const objc_method_def_t app_cmethods[] = {
    {"sharedApplication", app_shared},
};

/* ---------------- UIViewController ---------------- */

static id uivc_init(id self, SEL sel) { (void)sel; return self; }
static const objc_method_def_t uivc_methods[] = {
    {"init", uivc_init},
};

/* ---------------- UIApplicationMain ---------------- */

int redeg_UIApplicationMain(int argc, char **argv, const char *principal, const char *delegate) {
    (void)argc; (void)argv; (void)principal;
    l_info("[UIKit] UIApplicationMain('%s', '%s')", principal ? principal : "", delegate ? delegate : "");

    id app = app_shared(NULL, NULL);

    Class delcls = delegate ? objc_get_class(delegate) : NULL;
    if (!delcls) delcls = objc_get_class("s3eAppDelegate");
    if (!delcls) {
        l_fatal("[UIKit] delegate class not found");
        return 1;
    }
    id del = objc_alloc_instance(delcls);
    g_delegate = del;
    app_set_delegate(app, NULL, del);

    l_info("[UIKit] delegate %p, calling applicationDidFinishLaunching:", (void *)del);
    SEL s = "applicationDidFinishLaunching:";
    IMP imp = objc_lookup_msg(del, s);
    if (imp) {
        ((void (*)(id, SEL, id))imp)(del, s, app);
        l_info("[UIKit] applicationDidFinishLaunching returned");
    } else {
        l_fatal("[UIKit] applicationDidFinishLaunching: not found");
    }

    SEL active = "applicationDidBecomeActive:";
    IMP imp2 = objc_lookup_msg(del, active);
    if (imp2) {
        l_info("[UIKit] calling applicationDidBecomeActive:");
        ((void (*)(id, SEL, id))imp2)(del, active, app);
        l_info("[UIKit] applicationDidBecomeActive returned");
    }

    /* Do NOT call NewGLSubView or [GLView show] here!
     * On the guest thread, _s3eGLInit allocates g_s3eGLGlobals and then
     * _s3eEglCreateContext -> _s3eGL_start -> _RunOnMainThread(_s3eGL_start_real)
     * dispatches [s3eAppDelegate NewGLSubView] once g_s3eGLGlobals is valid.
     * Calling NewGLSubView prematurely before _s3eGLInit causes
     * GLView::createContext to run while g_s3eGLGlobals is NULL, and later
     * GLView::createFramebuffer skips createContext because self->m_Context
     * is already non-NULL, leaving g_s3eGLGlobals->m_Context == NULL. */

    {
        extern void redeg_dump_hooks(void);
        static int dump_ticks = 0;
        static int dumps = 0;
        static unsigned loop_count = 0;
        for (;;) {
            if ((++loop_count % 5000) == 0) l_info("[loop] iter %u", loop_count);
            if ((loop_count % 5000) == 0) {
                uint32_t d = g_macho.delta;
                uint32_t glue0 = *(volatile uint32_t *)(uintptr_t)(0x6fed8c + d);
                uint32_t glue1 = *(volatile uint32_t *)(uintptr_t)(0x6fed90 + d);
                uint32_t fib8  = *(volatile uint32_t *)(uintptr_t)(0x6fb5a8 + 8 + d);
                uint32_t fib14 = *(volatile uint32_t *)(uintptr_t)(0x6fb5a8 + 0x14 + d);
                uint32_t lit37028 = *(volatile uint32_t *)(uintptr_t)(0x37028 + d);
                extern volatile uint32_t g_io_read_bytes, g_io_read_calls;
                l_info("[fibre] loaderStack=0x%08x r9=0x%08x gameStack=0x%08x limit=0x%08x io=%u/%u lit=0x%08x",
                       glue0, glue1, fib8, fib14, g_io_read_bytes, g_io_read_calls, lit37028);
            }
            if (dumps < 6 && ++dump_ticks >= 2500) {
                dump_ticks = 0;
                dumps++;
                l_info("[hook] ---- sample %d ----", dumps);
                redeg_dump_hooks();
            }
            redeg_uikit_tick();
            extern void redeg_guest_log_drain(void);
            redeg_guest_log_drain();
            sceKernelDelayThread(2000);
        }
    }
    return 0;
}

typedef struct { id target; SEL sel; id arg; volatile int done; int heap; } pending_call_t;
static pending_call_t *s_pending[16];
static volatile int s_pending_count = 0;
static volatile int s_pending_lock = 0;

static int pending_lock(void) {
    for (int i = 0; i < 100000; i++) {
        if (__sync_lock_test_and_set(&s_pending_lock, 1) == 0) return 1;
    }
    return 0;
}
static void pending_unlock(void) { __sync_lock_release(&s_pending_lock); }

void redeg_perform_on_main(id target, SEL sel, id arg, int wait) {
    static int logged = 0;
    if (!target || !sel) return;

    /* Bring-up: execute every selector inline, even from guest pthreads.
     * The cross-thread queue + stack_call.done handshake was aborting with
     * an imprecise illegal-address error on the s3e thread, and the engine
     * only needs the callback to run before it continues. */
    {
        IMP imp = objc_lookup_msg(target, sel);
        if (logged++ < 8) l_info("[main] inline %s on %p", sel, (void *)target);
        if (imp) ((void (*)(id, SEL, id))imp)(target, sel, arg);
        return;
    }

    pending_call_t stack_call;
    pending_call_t *call;
    if (wait) {
        stack_call.target = target; stack_call.sel = sel; stack_call.arg = arg; stack_call.done = 0; stack_call.heap = 0;
        call = &stack_call;
    } else {
        call = (pending_call_t *)calloc(1, sizeof(*call));
        if (!call) return;
        call->target = target; call->sel = sel; call->arg = arg; call->heap = 1;
    }

    if (!pending_lock()) {
        l_warn("[main] perform queue busy");
        if (!wait) free(call);
        return;
    }
    int queued = 0;
    if (s_pending_count < (int)(sizeof(s_pending) / sizeof(s_pending[0]))) {
        s_pending[s_pending_count++] = call;
        queued = 1;
        if (logged++ < 8) l_info("[main] queued %s on %p wait=%d", sel, (void *)target, wait);
    } else {
        l_warn("[main] perform queue full, dropping %s", sel);
    }
    pending_unlock();

    if (!queued) {
        if (!wait) free(call);
        return;
    }
    if (wait) {
        while (!call->done) sceKernelDelayThread(1000);
    }
}

extern uintptr_t g_got_base;

__attribute__((naked)) static void call_s3e_set_touch(void *fn, void *touchObj, int pressed, int x, int y, int fromOS) {
    __asm__ volatile(
        "push {r4, r5, r9, lr}\n"
        "sub sp, sp, #8\n"
        "mov r4, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "mov r2, r3\n"
        "ldr r3, [sp, #24]\n"
        "ldr r5, [sp, #28]\n"
        "str r5, [sp, #0]\n"
        "ldr r9, =g_got_base\n"
        "ldr r9, [r9]\n"
        "blx r4\n"
        "add sp, sp, #8\n"
        "pop {r4, r5, r9, pc}\n"
    );
}

__attribute__((naked)) static void call_s3e_set_pos(void *fn, int x, int y, void *touchObj, int fromOS) {
    __asm__ volatile(
        "push {r4, r9, lr}\n"
        "sub sp, sp, #4\n"
        "mov r4, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "mov r2, r3\n"
        "ldr r3, [sp, #16]\n"
        "ldr r9, =g_got_base\n"
        "ldr r9, [r9]\n"
        "blx r4\n"
        "add sp, sp, #4\n"
        "pop {r4, r9, pc}\n"
    );
}

__attribute__((naked)) static void call_s3e_set_key(void *fn, int key, int pressed) {
    __asm__ volatile(
        "push {r4, r9, lr}\n"
        "sub sp, sp, #4\n"
        "mov r4, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "ldr r9, =g_got_base\n"
        "ldr r9, [r9]\n"
        "blx r4\n"
        "add sp, sp, #4\n"
        "pop {r4, r9, pc}\n"
        ".ltorg\n"
    );
}

void redeg_process_touches(id self, SEL sel) {
    (void)self; (void)sel;
    uint32_t d = g_macho.delta;
    if (!d) return;

    void *fn_set_touch = (void *)(uintptr_t)(0x000218e4u + d);
    void *fn_set_pos   = (void *)(uintptr_t)(0x00022038u + d);
    void *fn_set_key   = (void *)(uintptr_t)(0x0001f47cu + d);
    void *fn_dispatch_cb = (void *)(uintptr_t)(0x000c3d3cu + d + 1u); /* Thumb: CTouchButtonCallbackList::Dispatch */

    /* Ensure the active CKeyBinder table at 0x00235db0 and CSettings layouts
     * always have both Touch UI keys (Action 0 = 0xc9, Action 1 = 0xc8) and
     * physical action bindings (Actions 2..12) populated, even if a save file
     * or static init loaded zeroed touch-only bindings. */
    static const uint32_t s_default_keys[13] = {
        0xc9, /* 201: Action 0 (Confirm / Fire / Interact) */
        0xc8, /* 200: Action 1 (Cancel / Back / Run) */
        0x4d, /* 77:  Action 2 (LSK / Inventory / Menu) */
        0x4c, /* 76:  Action 3 (RSK / Pause / Skip) */
        0x0f, /* 15:  Action 4 (Down / Backward) */
        0x15, /* 21:  Action 5 (Key 3 / Reload / Sub-action) */
        0x11, /* 17:  Action 6 (Right / Turn Right) */
        0x13, /* 19:  Action 7 (Key 1 / Quick Turn / Sub-action) */
        0x0e, /* 14:  Action 8 (Up / Forward) */
        0x10, /* 16:  Action 9 (Left / Turn Left) */
        0x14, /* 20:  Action 10 (Key 2 / Aim / Ready Weapon) */
        0x16, /* 22:  Action 11 (Key 4 / Switch Target / Laser) */
        0x4f, /* 79:  Action 12 (Clear / Pause) */
    };
    uint32_t *kb = (uint32_t *)(uintptr_t)(0x00235db0u + d);
    if (kb[0] != 0xc9 || kb[1] != 0xc8 || kb[2] != 0x4d || kb[8] != 0x0e || kb[10] != 0x14) {
        for (int i = 0; i < 13; i++) kb[i] = s_default_keys[i];
        l_info("[input] populated hybrid Touch + Gamepad CKeyBinder table at %p", (void *)kb);
    }
    uintptr_t settings = *(uintptr_t *)(uintptr_t)(0x00238d18u + d);
    if (settings) {
        uint32_t *lk0 = (uint32_t *)(settings + 0x218);
        if (lk0[0] != 0xc9 || lk0[1] != 0xc8 || lk0[8] != 0x0e) {
            for (int layout = 0; layout < 4; layout++) {
                uint32_t *lk = (uint32_t *)(settings + 0x218 + layout * 0x38);
                for (int i = 0; i < 13; i++) lk[i] = s_default_keys[i];
            }
            l_info("[input] populated CSettings key layouts at 0x%08x", (unsigned)settings);
        }
    }

    /* Inspect BOTH touch managers used by GetInputKeyState (0x00104cb0):
     *   1) mgr1 = *0x00238d44 (CTouchScreenManager, 0x000a54f8): btns at +0x04, nbtns at +0x08
     *   2) mgr2 = *0x00235908 (CMenuTouchManager,   0x00084710): btns at +0x00, nbtns at +0x04
     * Verified CTouchControl / CTouchButton layout via Rizin:
     *   +0x00: vtable (0x0012448c + d = CTouchButton, 0x00126cc4 + d = CVirtualPad)
     *   +0x04: m_Flags (0 = CVirtualPad, bit 1 (2) = CTouchButton set at 0x0009dab8)
     *   +0x55: uint8_t m_bVisible (CTouchControl::IsVisible at 0x000f6ed2)
     *   +0x56: uint8_t m_bEnabled (initialized to 1 at 0x000b699a, checked at 0x000f6ef0)
     *   +0xd4: uint8_t CTouchButton::m_bDisabled (set at 0x0009da88, 1 = greyed out) */
    uintptr_t mgr1 = *(uintptr_t *)(uintptr_t)(0x00238d44u + d);
    uintptr_t mgr2 = *(uintptr_t *)(uintptr_t)(0x00235908u + d);
    uintptr_t vis_btns[32];
    int n_vis = 0;
    uintptr_t vpad_ptr = 0;
    int vpad_active = 0;

    struct {
        uintptr_t mgr;
        uintptr_t *btns;
        uint32_t nbtns;
    } sources[2] = {
        { mgr1, mgr1 ? *(uintptr_t **)(mgr1 + 0x04) : NULL, mgr1 ? *(uint32_t *)(mgr1 + 0x08) : 0 },
        { mgr2, mgr2 ? *(uintptr_t **)(mgr2 + 0x00) : NULL, mgr2 ? *(uint32_t *)(mgr2 + 0x04) : 0 },
    };

    uint32_t vt_btn  = 0x0012448cu + d;
    uint32_t vt_vpad = 0x00126cc4u + d;
    for (int s = 0; s < 2; s++) {
        uintptr_t *btns = sources[s].btns;
        uint32_t nbtns  = sources[s].nbtns;
        if (!btns || nbtns > 2048) continue;
        for (uint32_t i = 0; i < nbtns; i++) {
            uintptr_t b = btns[i];
            if (!b) continue;
            uint8_t vis = *(uint8_t *)(b + 0x55);
            if (!vis) continue;
            uint32_t vt    = *(uint32_t *)(b + 0x00);
            uint32_t flags = *(uint32_t *)(b + 0x04);
            if (vt == vt_vpad) {
                vpad_ptr = b;
                vpad_active = 1;
            } else if (vt == vt_btn || (flags & 2u)) {
                uint8_t btn_dis = *(uint8_t *)(b + 0xd4);
                if (btn_dis) continue;
                int16_t w = *(int16_t *)(b + 0x44);
                int16_t h = *(int16_t *)(b + 0x46);
                if (w > 0 && h > 0 && n_vis < 32) {
                    int dup = 0;
                    for (int k = 0; k < n_vis; k++) {
                        if (vis_btns[k] == b) { dup = 1; break; }
                    }
                    if (!dup) vis_btns[n_vis++] = b;
                }
            }
        }
    }

    /* In 3D Gameplay / Cutscene HUD (vpad_active == 1 or any 0x000bfe09 HUD button visible),
     * retain only 3D HUD buttons whose primary callback is SendPlayerActionEvent (0x000bfe09),
     * and clear stale vis=1 flags on any lingering CGUIMenuItem (0x000bfeb1) from an un-ticked
     * PDA Options tab close so they never reappear during reload/draw animations. */
    int has_player_act = vpad_active;
    for (int i = 0; i < n_vis && !has_player_act; i++) {
        uintptr_t b = vis_btns[i];
        uint32_t n_cb = *(uint32_t *)(b + 0x6c);
        uint32_t *fns = *(uint32_t **)(b + 0x68);
        uint32_t fn0  = (n_cb && fns) ? (fns[0] - d) : 0;
        if (fn0 == 0x000bfe09u) has_player_act = 1;
    }
    if (has_player_act) {
        int out_vis = 0;
        for (int i = 0; i < n_vis; i++) {
            uintptr_t b = vis_btns[i];
            uint32_t n_cb = *(uint32_t *)(b + 0x6c);
            uint32_t *fns = *(uint32_t **)(b + 0x68);
            uint32_t fn0  = (n_cb && fns) ? (fns[0] - d) : 0;
            if (fn0 == 0x000bfe09u) {
                vis_btns[out_vis++] = b;
            } else if (fn0 == 0x000bfeb1u) {
                *(uint8_t *)(b + 0x55) = 0;
            }
        }
        n_vis = out_vis;
    }

    /* Sort visible CTouchButtons top-to-bottom, left-to-right, placing CGUIMenuItems
     * (fn0 == 0x000bfeb1) before non-item chrome buttons (like PDA < / > / (X) tabs)
     * and placing bottom-left corner back/cancel icon (x < 70 && y > 240) at the end
     * so s_menu_idx=0 always starts on the first primary menu option. */
    for (int i = 0; i < n_vis - 1; i++) {
        for (int j = i + 1; j < n_vis; j++) {
            uint32_t n_cbi = *(uint32_t *)(vis_btns[i] + 0x6c);
            uint32_t *fnsi = *(uint32_t **)(vis_btns[i] + 0x68);
            uint32_t fni   = (n_cbi && fnsi) ? (fnsi[0] - d) : 0;
            uint32_t n_cbj = *(uint32_t *)(vis_btns[j] + 0x6c);
            uint32_t *fnsj = *(uint32_t **)(vis_btns[j] + 0x68);
            uint32_t fnj   = (n_cbj && fnsj) ? (fnsj[0] - d) : 0;
            int item_i = (fni == 0x000bfeb1u) ? 1 : 0;
            int item_j = (fnj == 0x000bfeb1u) ? 1 : 0;
            int16_t xi = *(int16_t *)(vis_btns[i] + 0x40);
            int16_t yi = *(int16_t *)(vis_btns[i] + 0x42);
            int16_t xj = *(int16_t *)(vis_btns[j] + 0x40);
            int16_t yj = *(int16_t *)(vis_btns[j] + 0x42);
            int back_i = (xi < 70 && yi > 240) ? 1 : 0;
            int back_j = (xj < 70 && yj > 240) ? 1 : 0;
            int swap = 0;
            if (item_i != item_j) {
                swap = item_i < item_j;
            } else if (back_i != back_j) {
                swap = back_i > back_j;
            } else if (yj < yi - 8 || (abs(yj - yi) <= 8 && xj < xi)) {
                swap = 1;
            }
            if (swap) {
                uintptr_t tmp = vis_btns[i];
                vis_btns[i] = vis_btns[j];
                vis_btns[j] = tmp;
            }
        }
    }

    static int s_menu_idx = 0;
    static int s_prev_n_vis = -1;
    static int s_prev_vpad = -1;
    static uintptr_t s_prev_first_btn = 0;
    static uintptr_t s_prev_mgr1 = 0xdeadbeef;
    static uintptr_t s_prev_mgr2 = 0xdeadbeef;
    static uint32_t s_prev_n1 = 0xdeadbeef;
    static uint32_t s_prev_n2 = 0xdeadbeef;
    uintptr_t first_btn = (n_vis > 0) ? vis_btns[0] : 0;
    if (n_vis != s_prev_n_vis || vpad_active != s_prev_vpad || first_btn != s_prev_first_btn ||
        mgr1 != s_prev_mgr1 || mgr2 != s_prev_mgr2 ||
        sources[0].nbtns != s_prev_n1 || sources[1].nbtns != s_prev_n2) {
        s_menu_idx = 0;
        /* On Main Menu (btn[0]=CONTINUE at (62,79), btn[1]=NEW GAME at (62,119)),
         * start cursor on NEW GAME (index 1) by default. */
        if (n_vis >= 5 && *(int16_t *)(vis_btns[0] + 0x40) == 62 && *(int16_t *)(vis_btns[0] + 0x42) == 79) {
            s_menu_idx = 1;
        }
        s_prev_n_vis = n_vis;
        s_prev_vpad = vpad_active;
        s_prev_first_btn = first_btn;
        s_prev_mgr1 = mgr1;
        s_prev_mgr2 = mgr2;
        s_prev_n1 = sources[0].nbtns;
        s_prev_n2 = sources[1].nbtns;
        if (!vpad_active && n_vis > 0 && s_menu_idx < n_vis) {
            uintptr_t b0 = vis_btns[s_menu_idx];
            uint32_t n_cb0 = *(uint32_t *)(b0 + 0x6c);
            uint32_t *fns0 = *(uint32_t **)(b0 + 0x68);
            uint32_t fn0 = (n_cb0 && fns0) ? (fns0[0] - d) : 0;
            if (fn0 == 0x000bfeb1u) {
                ((void (*)(void *, uint32_t))fn_dispatch_cb)((void *)(b0 + 0x60), 2u);
            }
        }
        l_info("[input] mgr1=%p(n=%u) mgr2=%p(n=%u) vis_btns=%d vpad=%d (vpad_ptr=%p)",
               (void *)mgr1, sources[0].nbtns, (void *)mgr2, sources[1].nbtns,
               n_vis, vpad_active, (void *)vpad_ptr);
        for (int s = 0; s < 2; s++) {
            uintptr_t *btns = sources[s].btns;
            uint32_t nbtns  = sources[s].nbtns;
            if (!btns || nbtns > 2048) continue;
            uint32_t logged = 0;
            for (uint32_t i = 0; i < nbtns && logged < 24; i++) {
                uintptr_t b = btns[i];
                if (!b) continue;
                uint8_t vis = *(uint8_t *)(b + 0x55);
                if (!vis && i >= 6) continue;
                logged++;
                l_info("[input]   raw[s%d:%u] %p vt=0x%x flg=0x%x vis=%u en=%u dis=%u op=%d/%d rect=(%d,%d %dx%d) key0=0x%x",
                       s + 1, i, (void *)b,
                       (unsigned)(*(uint32_t *)(b + 0x00) - d),
                       *(uint32_t *)(b + 0x04),
                       vis, *(uint8_t *)(b + 0x56), *(uint8_t *)(b + 0xd4),
                       *(int32_t *)(b + 0x0c), *(int32_t *)(b + 0x10),
                       *(int16_t *)(b + 0x40), *(int16_t *)(b + 0x42),
                       *(int16_t *)(b + 0x44), *(int16_t *)(b + 0x46),
                       *(uint32_t *)(b + 0x60));
            }
        }
        for (int i = 0; i < n_vis; i++) {
            uintptr_t b = vis_btns[i];
            uint32_t key0 = *(uint32_t *)(b + 0x60);
            uint32_t n_cb = *(uint32_t *)(b + 0x6c);
            uint32_t *fns = *(uint32_t **)(b + 0x68);
            uintptr_t *uds = *(uintptr_t **)(b + 0x78);
            uint32_t *ap  = *(uint32_t **)(b + 0x88); /* onPress   (ev & 2) */
            uint32_t *ah  = *(uint32_t **)(b + 0x98); /* onHold    (ev & 1) */
            uint32_t *ar  = *(uint32_t **)(b + 0xa8); /* onRelease (ev & 4) */
            uint32_t fn0  = (n_cb && fns) ? (fns[0] - d) : 0;
            uintptr_t ud0 = (n_cb && uds) ? uds[0] : 0;
            uint32_t act_p = (n_cb && ap) ? ap[0] : 0x11;
            uint32_t act_h = (n_cb && ah) ? ah[0] : 0x11;
            uint32_t act_r = (n_cb && ar) ? ar[0] : 0x11;
            l_info("[input]   btn[%d] %p rect=(%d,%d %dx%d) key0=0x%x cb=%u fn=0x%x ud0=%p P=%u H=%u R=%u",
                   i, (void *)b,
                   *(int16_t *)(b + 0x40), *(int16_t *)(b + 0x42),
                   *(int16_t *)(b + 0x44), *(int16_t *)(b + 0x46),
                   key0, n_cb, fn0, (void *)ud0, act_p, act_h, act_r);
        }
    }
    if (s_menu_idx >= n_vis) s_menu_idx = (n_vis > 0) ? (n_vis - 1) : 0;

    static int s_touch_down = 0;
    static int s_synth_touch_down = 0;
    static uint32_t s_synth_mask = 0;
    static int s_synth_x = 240, s_synth_y = 160;
    static int s_last_x = 240, s_last_y = 160;
    SceTouchData touch;
    memset(&touch, 0, sizeof(touch));
    if (sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1) >= 0 && touch.reportNum > 0) {
        int x = (touch.report[0].x * 480) / 1920;
        int y = (touch.report[0].y * 320) / 1088;
        if (x < 0) x = 0; else if (x > 479) x = 479;
        if (y < 0) y = 0; else if (y > 319) y = 319;
        if (!s_touch_down) {
            s_touch_down = 1;
            s_last_x = x;
            s_last_y = y;
            l_info("[input] touch down (%d, %d)", x, y);
            call_s3e_set_touch(fn_set_touch, (void *)1, 1, x, y, 1);
        } else if (x != s_last_x || y != s_last_y) {
            s_last_x = x;
            s_last_y = y;
            call_s3e_set_pos(fn_set_pos, x, y, (void *)1, 1);
        }
    } else if (s_touch_down) {
        s_touch_down = 0;
        l_info("[input] touch up (%d, %d)", s_last_x, s_last_y);
        call_s3e_set_touch(fn_set_touch, (void *)1, 0, s_last_x, s_last_y, 1);
    }

    static uint32_t s_prev_buttons = 0;
    static int s_vpad_driven = 0;
    static int s_vpad_last_x = 74, s_vpad_last_y = 246;
    static int s_r_hold_frames = 0;
    static int s_r_entered_aim = 0;
    static int s_l_hold_frames = 0;
    static int s_l_entered_knife = 0;
    SceCtrlData pad;
    memset(&pad, 0, sizeof(pad));
    if (sceCtrlPeekBufferPositive(0, &pad, 1) >= 0) {
        uint32_t btns = pad.buttons;
        int stick_dx = (int)pad.lx - 128;
        int stick_dy = (int)pad.ly - 128;
        if (abs(stick_dx) < 28) stick_dx = 0;
        if (abs(stick_dy) < 28) stick_dy = 0;

        int rstick_dx = (int)pad.rx - 128;
        int rstick_dy = (int)pad.ry - 128;
        if (abs(rstick_dx) < 28) rstick_dx = 0;
        if (abs(rstick_dy) < 28) rstick_dy = 0;

        if (pad.lx < 64)  btns |= SCE_CTRL_LEFT;
        if (pad.lx > 192) btns |= SCE_CTRL_RIGHT;
        if (pad.ly < 64)  btns |= SCE_CTRL_UP;
        if (pad.ly > 192) btns |= SCE_CTRL_DOWN;

        uint32_t chg = btns ^ s_prev_buttons;
        uint32_t just_pressed  = chg & btns;
        uint32_t just_released = chg & ~btns;
        void (*fn_player_act)(void *, uint32_t) = (void (*)(void *, uint32_t))(d + 0x000bfe09);

        /* Distinguish Cutscene HUD (top-right P=15 Skip icon at x > 300) vs 3D Gameplay
         * action animation (top-left P=15 PDA icon at (0,0) when vpad temporarily hides). */
        int is_cutscene_hud = 0;
        int in_3d_gameplay  = vpad_active;
        if (!vpad_active && n_vis == 1) {
            uintptr_t b0 = vis_btns[0];
            int16_t x0 = *(int16_t *)(b0 + 0x40);
            uint32_t n_cb0 = *(uint32_t *)(b0 + 0x6c);
            uint32_t *fns0 = *(uint32_t **)(b0 + 0x68);
            uint32_t fn0 = (n_cb0 && fns0) ? (fns0[0] - d) : 0;
            if (fn0 == 0x000bfe09u) {
                if (x0 > 300) is_cutscene_hud = 1;
                else          in_3d_gameplay  = 1;
            }
        }

        if (s_synth_touch_down && (in_3d_gameplay || is_cutscene_hud || s_touch_down || !(btns & s_synth_mask))) {
            s_synth_touch_down = 0;
            s_synth_mask = 0;
            l_info("[input] synth menu touch up (%d, %d)", s_synth_x, s_synth_y);
            if (!s_touch_down) {
                call_s3e_set_touch(fn_set_touch, (void *)1, 0, s_synth_x, s_synth_y, 1);
            }
        }

        if (s_vpad_driven && (!vpad_active || !vpad_ptr || s_touch_down)) {
            s_vpad_driven = 0;
            if (!s_touch_down) {
                call_s3e_set_touch(fn_set_touch, (void *)1, 0, s_vpad_last_x, s_vpad_last_y, 1);
            }
        }

        if (in_3d_gameplay && !s_touch_down) {
            /* Read ground-truth CPlayer state via *(*(0x00235abc + d) + 4) (verified at 0x000bfe12..0x000bfe42):
             * - [player + 0x114]: uint8_t m_bWalkInputEnabled   (HandleInputWalk   0x000e4b00)
             * - [player + 0x124]: uint8_t m_bCombatInputEnabled (HandleInputCombat 0x000e46dc)
             * - [player + 0x2ec]: uint8_t m_bKnifeEquipped      (0 = Gun Aim Mode, 1 = Knife Mode, set at 0x000e2fce) */
            uintptr_t world    = *(uintptr_t *)(uintptr_t)(0x00235abcu + d);
            uintptr_t player   = world ? *(uintptr_t *)(world + 4) : 0;
            uint8_t walk_in    = player ? *(uint8_t *)(player + 0x114) : 0;
            uint8_t combat_in  = player ? *(uint8_t *)(player + 0x124) : 0;
            uint8_t knife_mod  = player ? *(uint8_t *)(player + 0x2ec) : 0;

            int has_holster_btn = 0;
            for (int i = 0; i < n_vis; i++) {
                uintptr_t b = vis_btns[i];
                uint32_t n_cb = *(uint32_t *)(b + 0x6c);
                uint32_t *ap  = *(uint32_t **)(b + 0x88);
                uint32_t act_p = (n_cb && ap) ? ap[0] : 0x11;
                if (act_p == 16) has_holster_btn = 1;
            }
            int in_combat     = (has_holster_btn || combat_in != 0);
            int in_aim_mode   = (in_combat && knife_mod == 0);
            int in_knife_mode = (in_combat && knife_mod != 0);

            /* Support both Tap-R-to-Toggle-Aim and Hold-R-to-Aim (>= 45 frames) */
            int r_holster_trig = 0;
            int l_holster_trig = 0;
            if (just_pressed & SCE_CTRL_RTRIGGER) {
                s_r_hold_frames = 0;
                if (!in_aim_mode) {
                    s_r_entered_aim = 1;
                } else {
                    s_r_entered_aim = 0;
                    r_holster_trig = 1;
                }
            }
            if (btns & SCE_CTRL_RTRIGGER) {
                s_r_hold_frames++;
            } else {
                if ((just_released & SCE_CTRL_RTRIGGER) && in_aim_mode && s_r_entered_aim && s_r_hold_frames >= 45) {
                    r_holster_trig = 1;
                }
                s_r_hold_frames = 0;
                s_r_entered_aim = 0;
            }

            if (just_pressed & SCE_CTRL_LTRIGGER) {
                s_l_hold_frames = 0;
                if (!in_knife_mode) {
                    s_l_entered_knife = 1;
                }
            }
            if (btns & SCE_CTRL_LTRIGGER) {
                s_l_hold_frames++;
            } else {
                if ((just_released & SCE_CTRL_LTRIGGER) && in_knife_mode && s_l_entered_knife && s_l_hold_frames >= 45) {
                    l_holster_trig = 1;
                }
                s_l_hold_frames = 0;
                s_l_entered_knife = 0;
            }

            /* Drive CVirtualPad (0x000ae350 -> 0x000ae468 -> 0x000ae0dc) with Left Stick / Right Stick / D-Pad
             * via synthetic s3ePointer touch-down at pad center followed by drag deflection.
             * AnalogueFastMoveThreshold is 0.6 (~38 px on 64 px half-width), so 52..54 px runs forward. */
            int dx = (stick_dx * 54) / 128;
            int dy = (stick_dy * 54) / 128;
            if (dx == 0 && dy == 0) {
                dx = (rstick_dx * 54) / 128;
                dy = (rstick_dy * 54) / 128;
            }
            if (dx == 0 && dy == 0) {
                if (btns & SCE_CTRL_LEFT)  dx = -52;
                if (btns & SCE_CTRL_RIGHT) dx =  52;
                if (btns & SCE_CTRL_UP)    dy = -52;
                if (btns & SCE_CTRL_DOWN)  dy =  52;
            }
            if (!in_combat && dy == 0 && (btns & SCE_CTRL_CIRCLE) && !(btns & SCE_CTRL_DOWN)) {
                dy = -54; /* Holding CIRCLE in Walk Mode runs Leon forward */
            }
            if (vpad_active && vpad_ptr) {
                int16_t vx = *(int16_t *)(vpad_ptr + 0x40);
                int16_t vy = *(int16_t *)(vpad_ptr + 0x42);
                int16_t vw = *(int16_t *)(vpad_ptr + 0x44);
                int16_t vh = *(int16_t *)(vpad_ptr + 0x46);
                int cx = vx + (vw > 0 ? vw / 2 : 64);
                int cy = vy + (vh > 0 ? vh / 2 : 64);
                if (dx != 0 || dy != 0) {
                    int tx = cx + dx;
                    int ty = cy + dy;
                    if (!s_vpad_driven) {
                        /* Frame 1: touch down at pad center (cx, cy) so CVirtualPad::OnTouchEvent
                         * (0x000ae388 -> 0x000adec6) latches the pad origin and sets +0x9d = 1. */
                        s_vpad_driven = 1;
                        s_vpad_last_x = cx;
                        s_vpad_last_y = cy;
                        call_s3e_set_touch(fn_set_touch, (void *)1, 1, cx, cy, 1);
                    } else {
                        /* Frame 2+: drag touch to (tx, ty) so CVirtualPad::OnTouchEvent (0x000ae3d8)
                         * and CVirtualPad::UpdateDirection (0x000ae0dc) compute deflection. */
                        s_vpad_last_x = tx;
                        s_vpad_last_y = ty;
                        call_s3e_set_pos(fn_set_pos, tx, ty, (void *)1, 1);
                    }
                } else if (s_vpad_driven) {
                    s_vpad_driven = 0;
                    call_s3e_set_touch(fn_set_touch, (void *)1, 0, s_vpad_last_x, s_vpad_last_y, 1);
                }
            }

            /* Dispatch 3D HUD CTouchButtons via physical buttons using true EPlayerAction (0..16) */
            uint32_t handled_acts = 0;
            for (int i = 0; i < n_vis; i++) {
                uintptr_t b = vis_btns[i];
                uint32_t key0 = *(uint32_t *)(b + 0x60);
                uint32_t n_cb = *(uint32_t *)(b + 0x6c);
                uint32_t *ap  = *(uint32_t **)(b + 0x88);
                uint32_t *ah  = *(uint32_t **)(b + 0x98);
                uint32_t *ar  = *(uint32_t **)(b + 0xa8);
                uint32_t act_p = (n_cb && ap) ? ap[0] : 0x11;
                uint32_t act_h = (n_cb && ah) ? ah[0] : 0x11;
                uint32_t act_r = (n_cb && ar) ? ar[0] : 0x11;
                uint32_t act = (act_p != 0x11) ? act_p : ((act_h != 0x11) ? act_h : act_r);
                int16_t bx = *(int16_t *)(b + 0x40);
                int16_t by = *(int16_t *)(b + 0x42);

                uint32_t mask = 0;
                uint32_t extra_ev = 0;
                if (act < 0x11) {
                    handled_acts |= (1u << act);
                    switch (act) {
                        case 0: /* +action: Fire (Aim Mode) / Switch to Gun (Knife Mode) / Interact & Aim (Walk Mode) */
                            if (in_aim_mode)        mask = SCE_CTRL_CROSS;
                            else if (in_knife_mode) mask = SCE_CTRL_RTRIGGER;
                            else                    mask = SCE_CTRL_CROSS | SCE_CTRL_RTRIGGER;
                            break;
                        case 1:  mask = SCE_CTRL_CROSS; break;                     /* action_held */
                        case 2:  mask = SCE_CTRL_RTRIGGER; break;                  /* aim */
                        case 3: /* knife: Slash Knife (Knife Mode) / Enter Knife Mode (Walk & Aim Modes) */
                            mask = in_knife_mode ? (SCE_CTRL_LTRIGGER | SCE_CTRL_CROSS) : SCE_CTRL_LTRIGGER;
                            break;
                        case 4:  mask = SCE_CTRL_SQUARE; break;                    /* reload: Reload Weapon */
                        case 9:  mask = SCE_CTRL_TRIANGLE; break;                  /* double_AIM: Switch Target / Laser */
                        case 10: mask = SCE_CTRL_TRIANGLE; break;                  /* double_action */
                        case 11: mask = (!in_combat && !(btns & SCE_CTRL_DOWN)) ? SCE_CTRL_CIRCLE : 0; break;
                        case 12: mask = (!in_combat && (btns & SCE_CTRL_DOWN))  ? SCE_CTRL_CIRCLE : 0; break;
                        case 15: mask = SCE_CTRL_START | SCE_CTRL_SELECT; break;   /* back_softkey: Open PDA */
                        case 16: /* advance_softkey: Holster Weapon & Return to Walk Mode */
                            mask = SCE_CTRL_CIRCLE;
                            if (r_holster_trig || l_holster_trig) extra_ev = 2u;
                            break;
                        default: break;
                    }
                }
                if (!mask && !extra_ev) {
                    if (key0 == 0xc9) mask = SCE_CTRL_CROSS;
                    else if (key0 == 0xc8) mask = SCE_CTRL_CIRCLE;
                    else if (key0 == 0x4d) mask = SCE_CTRL_SELECT;
                    else if (key0 == 0x4c || key0 == 0x4f) mask = SCE_CTRL_START;
                    else if (by < 60 && bx > 380) mask = SCE_CTRL_CIRCLE;
                    else if (by < 60 && bx < 100) mask = SCE_CTRL_START | SCE_CTRL_SELECT;
                }
                if (mask || extra_ev) {
                    uint32_t ev = extra_ev;
                    if (just_pressed & mask)  ev |= 2u;
                    if (btns & mask)          ev |= 1u;
                    if (just_released & mask) ev |= 4u;
                    if (ev) {
                        if (ev & 2u) {
                            l_info("[input] 3D HUD press: act=%u ev=0x%x (walk=%u combat=%u knife=%u)",
                                   act, ev, walk_in, combat_in, knife_mod);
                        }
                        *(uint32_t *)(b + 0x48) = 0u;
                        *(uint32_t *)(b + 0x4c) = 5u;
                        ((void (*)(void *, uint32_t))fn_dispatch_cb)((void *)(b + 0x60), ev);
                    }
                }
            }

            /* Direct SendPlayerActionEvent (0x000bfe09) for gesture/button actions without a visible HUD icon */
            if ((just_pressed & SCE_CTRL_LTRIGGER) && !(handled_acts & (1u << 3))) {
                l_info("[input] 3D action: Knife Mode (3)");
                fn_player_act(NULL, 3u);
            }
            if ((just_pressed & SCE_CTRL_SQUARE) && !(handled_acts & (1u << 4))) {
                l_info("[input] 3D action: Reload (4)");
                fn_player_act(NULL, 4u);
            }
            if ((just_pressed & SCE_CTRL_TRIANGLE) && !(handled_acts & (1u << 9))) {
                l_info("[input] 3D action: Switch Target / Laser (9)");
                fn_player_act(NULL, 9u);
            }
            if (!in_combat &&
                (((just_pressed & SCE_CTRL_CIRCLE) && (btns & SCE_CTRL_DOWN)) ||
                 ((just_pressed & SCE_CTRL_DOWN) && (btns & SCE_CTRL_CIRCLE)))) {
                if (!(handled_acts & (1u << 12))) {
                    l_info("[input] 3D action: Quick-Turn 180 (12)");
                    fn_player_act(NULL, 12u);
                }
            }
        } else if (is_cutscene_hud) {
            s_vpad_driven = 0;
            /* Cutscene HUD: START/SELECT skips cutscene (P=15); CROSS/D-Pad trigger QTEs (0, 5, 6, 7, 8) */
            uintptr_t b = vis_btns[0];
            uint32_t skip_mask = SCE_CTRL_START | SCE_CTRL_SELECT;
            uint32_t ev = 0;
            if (just_pressed & skip_mask)  ev |= 2u;
            if (btns & skip_mask)          ev |= 1u;
            if (just_released & skip_mask) ev |= 4u;
            if (ev) {
                *(uint32_t *)(b + 0x48) = (btns & skip_mask) ? 3u : 0u;
                *(uint32_t *)(b + 0x4c) = ev;
                ((void (*)(void *, uint32_t))fn_dispatch_cb)((void *)(b + 0x60), ev);
            }
            if (just_pressed & SCE_CTRL_CROSS) fn_player_act(NULL, 0u);
            if (just_pressed & SCE_CTRL_UP)    fn_player_act(NULL, 5u);
            if (just_pressed & SCE_CTRL_DOWN)  fn_player_act(NULL, 6u);
            if (just_pressed & SCE_CTRL_LEFT)  fn_player_act(NULL, 7u);
            if (just_pressed & SCE_CTRL_RIGHT) fn_player_act(NULL, 8u);
        } else if (!vpad_active) {
            s_vpad_driven = 0;
            /* Scan visible buttons for SendGuiCommandEvent (0x000bfda9, used by PDA & COMMUNICATIONS) */
            uintptr_t gui_cmd_btn[6] = { 0, 0, 0, 0, 0, 0 };
            for (int i = 0; i < n_vis; i++) {
                uintptr_t b = vis_btns[i];
                uint32_t n_cb = *(uint32_t *)(b + 0x6c);
                uint32_t *fns = *(uint32_t **)(b + 0x68);
                uint32_t *ap  = *(uint32_t **)(b + 0x88);
                uint32_t fn0  = (n_cb && fns) ? (fns[0] - d) : 0;
                uint32_t act_p = (n_cb && ap) ? ap[0] : 0x11;
                if (fn0 == 0x000bfda9u && act_p < 6) {
                    gui_cmd_btn[act_p] = b;
                }
            }

            /* Navigate visible 2D menu buttons with D-Pad / Left Stick */
            if (n_vis > 0) {
                int n_items = 0;
                for (int i = 0; i < n_vis; i++) {
                    uint32_t n_cb = *(uint32_t *)(vis_btns[i] + 0x6c);
                    uint32_t *fns = *(uint32_t **)(vis_btns[i] + 0x68);
                    uint32_t fn0  = (n_cb && fns) ? (fns[0] - d) : 0;
                    if (fn0 == 0x000bfeb1u) n_items++;
                }
                int n_nav = (n_items > 0) ? n_items : n_vis;
                if (n_items == 0 && n_vis > 1) {
                    int16_t lx = *(int16_t *)(vis_btns[n_vis - 1] + 0x40);
                    int16_t ly = *(int16_t *)(vis_btns[n_vis - 1] + 0x42);
                    if (lx < 70 && ly > 240) n_nav = n_vis - 1;
                }
                if (s_menu_idx >= n_nav) s_menu_idx = n_nav - 1;

                int moved = 0;
                if (just_pressed & (SCE_CTRL_UP | SCE_CTRL_LEFT)) {
                    s_menu_idx = (s_menu_idx + n_nav - 1) % n_nav;
                    moved = 1;
                }
                if (just_pressed & (SCE_CTRL_DOWN | SCE_CTRL_RIGHT)) {
                    s_menu_idx = (s_menu_idx + 1) % n_nav;
                    moved = 1;
                }
                if (moved) {
                    uintptr_t b = vis_btns[s_menu_idx];
                    uint32_t n_cb = *(uint32_t *)(b + 0x6c);
                    uint32_t *fns = *(uint32_t **)(b + 0x68);
                    uint32_t fn0  = (n_cb && fns) ? (fns[0] - d) : 0;
                    l_info("[input] menu cursor -> %d/%d (btn=%p fn0=0x%x)",
                           s_menu_idx, n_nav, (void *)b, fn0);
                    /* Dispatch CGUIMenuItem onPress (0x000bfeb1 -> Event_SelectMenuItem
                     * with bConfirmed=0) once on cursor move to highlight the item in red/orange. */
                    if (fn0 == 0x000bfeb1u) {
                        ((void (*)(void *, uint32_t))fn_dispatch_cb)((void *)(b + 0x60), 2u);
                    }
                }
            }

            /* Activate 2D menu / PDA / COMMUNICATIONS buttons via synthetic s3ePointer touch */
            if (!s_touch_down && !s_synth_touch_down) {
                uintptr_t target_btn = 0;
                uint32_t trig_mask = 0;

                if ((just_pressed & SCE_CTRL_LTRIGGER) && gui_cmd_btn[2]) {
                    target_btn = gui_cmd_btn[2]; /* PDA Prev Tab (<) */
                    trig_mask = SCE_CTRL_LTRIGGER;
                } else if ((just_pressed & SCE_CTRL_RTRIGGER) && gui_cmd_btn[3]) {
                    target_btn = gui_cmd_btn[3]; /* PDA Next Tab (>) */
                    trig_mask = SCE_CTRL_RTRIGGER;
                } else if ((just_pressed & SCE_CTRL_TRIANGLE) && gui_cmd_btn[0]) {
                    target_btn = gui_cmd_btn[0]; /* PDA Map Zoom In (+) */
                    trig_mask = SCE_CTRL_TRIANGLE;
                } else if ((just_pressed & SCE_CTRL_SQUARE) && gui_cmd_btn[1]) {
                    target_btn = gui_cmd_btn[1]; /* PDA Map Zoom Out (-) */
                    trig_mask = SCE_CTRL_SQUARE;
                } else if ((just_pressed & (SCE_CTRL_CIRCLE | SCE_CTRL_START)) && gui_cmd_btn[5]) {
                    target_btn = gui_cmd_btn[5]; /* PDA Close (X) / COMMUNICATIONS Skip All (>>) */
                    trig_mask = SCE_CTRL_CIRCLE | SCE_CTRL_START;
                } else if ((just_pressed & SCE_CTRL_CROSS) && gui_cmd_btn[4]) {
                    target_btn = gui_cmd_btn[4]; /* COMMUNICATIONS Advance Line (>) */
                    trig_mask = SCE_CTRL_CROSS;
                } else {
                    uint32_t act_mask = (n_vis > 0) ? SCE_CTRL_CROSS : (SCE_CTRL_CROSS | SCE_CTRL_START);
                    if (just_pressed & act_mask) {
                        trig_mask = act_mask;
                        if (n_vis > 0) {
                            target_btn = vis_btns[s_menu_idx];
                        } else {
                            s_synth_x = 240;
                            s_synth_y = 160;
                            s_synth_touch_down = 1;
                            s_synth_mask = trig_mask;
                            l_info("[input] synth title touch down (%d, %d)", s_synth_x, s_synth_y);
                            call_s3e_set_touch(fn_set_touch, (void *)1, 1, s_synth_x, s_synth_y, 1);
                        }
                    } else if ((just_pressed & SCE_CTRL_CIRCLE) && n_vis > 0) {
                        for (int i = n_vis - 1; i >= 0; i--) {
                            uintptr_t b = vis_btns[i];
                            int16_t bx = *(int16_t *)(b + 0x40);
                            int16_t by = *(int16_t *)(b + 0x42);
                            if (*(uint32_t *)(b + 0x60) == 0xc8 || (bx < 70 && by > 240)) {
                                target_btn = b;
                                trig_mask = SCE_CTRL_CIRCLE;
                                break;
                            }
                        }
                    }
                }

                if (target_btn && trig_mask) {
                    int16_t bx = *(int16_t *)(target_btn + 0x40);
                    int16_t by = *(int16_t *)(target_btn + 0x42);
                    int16_t bw = *(int16_t *)(target_btn + 0x44);
                    int16_t bh = *(int16_t *)(target_btn + 0x46);
                    s_synth_x = bx + bw / 2;
                    s_synth_y = by + bh / 2;
                    s_synth_touch_down = 1;
                    s_synth_mask = trig_mask;
                    l_info("[input] synth menu touch down (%d, %d) btn=%p mask=0x%x",
                           s_synth_x, s_synth_y, (void *)target_btn, trig_mask);
                    call_s3e_set_touch(fn_set_touch, (void *)1, 1, s_synth_x, s_synth_y, 1);
                }
            }
        }

        if (chg) {
            /* Only forward s3eKeyboard keys when neither 3D HUD nor 2D CTouchButtons are active,
             * preventing CKeyBinder from double-firing conflicting actions. */
            if (!vpad_active && n_vis == 0) {
                static const struct { uint32_t mask; int key; } kmap[] = {
                    { SCE_CTRL_UP,       14  },
                    { SCE_CTRL_DOWN,     15  },
                    { SCE_CTRL_LEFT,     16  },
                    { SCE_CTRL_RIGHT,    17  },
                    { SCE_CTRL_CROSS,    201 },
                    { SCE_CTRL_CROSS,    18  },
                    { SCE_CTRL_CIRCLE,   200 },
                    { SCE_CTRL_CIRCLE,   13  },
                    { SCE_CTRL_RTRIGGER, 20  },
                    { SCE_CTRL_SQUARE,   21  },
                    { SCE_CTRL_LTRIGGER, 19  },
                    { SCE_CTRL_TRIANGLE, 22  },
                    { SCE_CTRL_SELECT,   77  },
                    { SCE_CTRL_START,    76  },
                    { SCE_CTRL_START,    79  },
                };
                for (unsigned i = 0; i < sizeof(kmap)/sizeof(kmap[0]); i++) {
                    if (chg & kmap[i].mask) {
                        int pressed = (btns & kmap[i].mask) ? 1 : 0;
                        l_info("[input] key %d pressed=%d", kmap[i].key, pressed);
                        call_s3e_set_key(fn_set_key, kmap[i].key, pressed);
                    }
                }
            }
            s_prev_buttons = btns;
        }
    }
}

void redeg_uikit_tick(void) {
    if (s_alert_pending) {
        s_alert_pending = 0;
        id view = s_alert_pending_view;
        l_info("[UIAlertView] deferred dismiss %p", (void *)view);
        if (s_alert_delegate) {
            SEL cb = "alertView:clickedButtonAtIndex:";
            IMP imp = objc_lookup_msg(s_alert_delegate, cb);
            if (imp) {
                l_info("[UIAlertView] calling delegate alertView:clickedButtonAtIndex:0");
                ((void (*)(id, SEL, id, int))imp)(s_alert_delegate, cb, view, 0);
                l_info("[UIAlertView] delegate returned");
            }
        }
    }
    /* Deliver setNeedsDisplay to the views that asked for it. The game's
     * EAGL view presents its frame from drawRect:. */
    for (int i = 0; i < s_nviews; i++) {
        if (s_views[i].needs_display) {
            s_views[i].needs_display = 0;
            SEL dr = "drawRect:";
            IMP imp = objc_lookup_msg(s_views[i].obj, dr);
            if (imp) {
                static unsigned s_draw_count = 0;
                int log_draw = (s_draw_count < 10 || (s_draw_count % 600) == 0);
                s_draw_count++;
                if (log_draw) {
                    l_info("[draw] drawRect #%u on %p (%.0fx%.0f)", s_draw_count,
                           (void *)s_views[i].obj, s_views[i].w, s_views[i].h);
                }
                redeg_cgrect_t r = {{0, 0}, {s_views[i].w, s_views[i].h}};
                ((void (*)(id, SEL, redeg_cgrect_t))imp)(s_views[i].obj, dr, r);
            }
        }
    }

    {
        static int qlog = 0;
        if (qlog++ < 20) l_info("[tick] queue scan %d count=%d", qlog, s_pending_count);
    }
    for (;;) {
        pending_call_t *call;
        if (!pending_lock()) { l_warn("[tick] pending lock busy"); return; }
        if (s_pending_count == 0) { pending_unlock(); return; }
        call = s_pending[0];
        for (int i = 0; i < s_pending_count - 1; i++) s_pending[i] = s_pending[i + 1];
        s_pending_count--;
        pending_unlock();

        l_info("[main] running %s on %p", call->sel, (void *)call->target);
        IMP imp = objc_lookup_msg(call->target, call->sel);
        ((void (*)(id, SEL, id))imp)(call->target, call->sel, call->arg);
        l_info("[main] %s returned", call->sel);
        int heap = call->heap;
        call->done = 1;
        if (heap) free(call);
    }
}

/* ---------------- init ---------------- */

void objc_uikit_init(void) {
    g_UIView = objc_define_class("UIView", NULL, 0x40,
        uiv_methods, sizeof(uiv_methods)/sizeof(uiv_methods[0]), NULL, 0);
    g_UIWindow = objc_define_class("UIWindow", g_UIView, 0x54,
        uiw_methods, sizeof(uiw_methods)/sizeof(uiw_methods[0]), NULL, 0);
    g_UIViewController = objc_define_class("UIViewController", NULL, 0x40,
        uivc_methods, sizeof(uivc_methods)/sizeof(uivc_methods[0]), NULL, 0);
    g_UIScreen = objc_define_class("UIScreen", NULL, 0x10,
        uiscreen_methods, sizeof(uiscreen_methods)/sizeof(uiscreen_methods[0]),
        uiscreen_cmethods, sizeof(uiscreen_cmethods)/sizeof(uiscreen_cmethods[0]));
    g_UIColor = objc_define_class("UIColor", NULL, 0x10,
        NULL, 0, uicolor_cmethods, sizeof(uicolor_cmethods)/sizeof(uicolor_cmethods[0]));
    g_UIImage = objc_define_class("UIImage", NULL, 0x10,
        uiimage_methods, sizeof(uiimage_methods)/sizeof(uiimage_methods[0]),
        uiimage_cmethods, sizeof(uiimage_cmethods)/sizeof(uiimage_cmethods[0]));
    g_UIImageView = objc_define_class("UIImageView", g_UIView, 0x60,
        uiiv_methods, sizeof(uiiv_methods)/sizeof(uiiv_methods[0]), NULL, 0);
    g_UIDevice = objc_define_class("UIDevice", NULL, 0x10,
        uidev_methods, sizeof(uidev_methods)/sizeof(uidev_methods[0]),
        uidev_cmethods, sizeof(uidev_cmethods)/sizeof(uidev_cmethods[0]));
    g_UIAccelerometer = objc_define_class("UIAccelerometer", NULL, 0x10,
        accel_methods, sizeof(accel_methods)/sizeof(accel_methods[0]),
        accel_cmethods, sizeof(accel_cmethods)/sizeof(accel_cmethods[0]));
    g_UIAlertView = objc_define_class("UIAlertView", NULL, 0x40,
        alert_methods, sizeof(alert_methods)/sizeof(alert_methods[0]), NULL, 0);
    g_UITextField = objc_define_class("UITextField", g_UIView, 0x60,
        tf_methods, sizeof(tf_methods)/sizeof(tf_methods[0]), NULL, 0);
    g_UIApplication = objc_define_class("UIApplication", NULL, 0x40,
        app_methods, sizeof(app_methods)/sizeof(app_methods[0]),
        app_cmethods, sizeof(app_cmethods)/sizeof(app_cmethods[0]));
}
