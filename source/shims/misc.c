#include "objc/objc_classes.h"
#include "objc/cgttypes.h"
#include "utils/logger.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Framework data symbols                                             */
/* ------------------------------------------------------------------ */

void *kEAGLDrawablePropertyColorFormat = NULL;
void *kEAGLDrawablePropertyRetainedBacking = NULL;
void *kEAGLColorFormatRGBA8 = NULL;

double kCLLocationAccuracyBest = -1.0;
double kCLDistanceFilterNone = -1.0;
void *kCLErrorDomain = NULL;

void *MPMoviePlayerContentPreloadDidFinishNotification = NULL;
void *MPMoviePlayerPlaybackDidFinishNotification = NULL;

/* ------------------------------------------------------------------ */
/* SystemConfiguration / CFNetwork                                    */
/* ------------------------------------------------------------------ */

void *SCNetworkReachabilityCreateWithAddress(void *alloc, const void *addr) {
    (void)alloc; (void)addr;
    return NULL;
}
int SCNetworkReachabilityGetFlags(void *ref, uint32_t *flags) {
    (void)ref;
    if (flags) *flags = 0;
    return 1;
}

void *CFNetServiceBrowserCreate(void *alloc, void *client, void *flags) { (void)alloc; (void)client; (void)flags; return NULL; }
void CFNetServiceBrowserInvalidate(void *b) { (void)b; }
void CFNetServiceBrowserScheduleWithRunLoop(void *b, void *rl, void *mode) { (void)b; (void)rl; (void)mode; }
int CFNetServiceBrowserSearchForServices(void *b, void *domain, void *type, void *err) { (void)b; (void)domain; (void)type; (void)err; return -1; }
int CFNetServiceBrowserStopSearch(void *b, void *err) { (void)b; (void)err; return -1; }
void CFNetServiceBrowserUnscheduleFromRunLoop(void *b, void *rl, void *mode) { (void)b; (void)rl; (void)mode; }
void CFNetServiceCancel(void *s) { (void)s; }
void *CFNetServiceCreate(void *alloc, void *domain, void *type, void *name, int port) { (void)alloc; (void)domain; (void)type; (void)name; (void)port; return NULL; }
void *CFNetServiceGetAddressing(void *s) { (void)s; return NULL; }
void *CFNetServiceGetDomain(void *s) { (void)s; return NULL; }
void *CFNetServiceGetName(void *s) { (void)s; return NULL; }
int CFNetServiceGetPortNumber(void *s) { (void)s; return 0; }
void *CFNetServiceGetTXTData(void *s) { (void)s; return NULL; }
void *CFNetServiceGetTargetHost(void *s) { (void)s; return NULL; }
void *CFNetServiceGetType(void *s) { (void)s; return NULL; }
int CFNetServiceRegisterWithOptions(void *s, uint32_t opts, void *err) { (void)s; (void)opts; (void)err; return -1; }
int CFNetServiceResolveWithTimeout(void *s, uint32_t timeout_lo, uint32_t timeout_hi, void *err) { (void)s; (void)timeout_lo; (void)timeout_hi; (void)err; return -1; }
void CFNetServiceScheduleWithRunLoop(void *s, void *rl, void *mode) { (void)s; (void)rl; (void)mode; }
void CFNetServiceSetClient(void *s, void *client, void *user) { (void)s; (void)client; (void)user; }
int CFNetServiceSetTXTData(void *s, void *data) { (void)s; (void)data; return -1; }
void CFNetServiceUnscheduleFromRunLoop(void *s, void *rl, void *mode) { (void)s; (void)rl; (void)mode; }

/* ------------------------------------------------------------------ */
/* CLLocation / CLLocationManager                                     */
/* ------------------------------------------------------------------ */

Class g_CLLocationManager, g_CLLocation;

static id clm_init(id self, SEL sel) { (void)sel; return self; }
static void clm_set_delegate(id self, SEL sel, id d) { (void)self; (void)sel; (void)d; }
static void clm_set_distance(id self, SEL sel, double v) { (void)self; (void)sel; (void)v; }
static void clm_set_accuracy(id self, SEL sel, double v) { (void)self; (void)sel; (void)v; }
static int clm_services_enabled(id self, SEL sel) { (void)self; (void)sel; return 0; }
static void clm_start(id self, SEL sel) { (void)self; (void)sel; l_info("[location] startUpdatingLocation (stubbed)"); }
static void clm_stop(id self, SEL sel) { (void)self; (void)sel; }

static const objc_method_def_t clm_methods[] = {
    {"init", clm_init},
    {"setDelegate:", clm_set_delegate},
    {"setDistanceFilter:", clm_set_distance},
    {"setDesiredAccuracy:", clm_set_accuracy},
    {"locationServicesEnabled", clm_services_enabled},
    {"startUpdatingLocation", clm_start},
    {"stopUpdatingLocation", clm_stop},
};
static const objc_method_def_t clm_cmethods[] = {
    {"locationServicesEnabled", clm_services_enabled},
};

static redeg_clcoord_t cl_coordinate(id self, SEL sel) {
    (void)self; (void)sel;
    redeg_clcoord_t r = {0, 0};
    return r;
}
static double cl_hacc(id self, SEL sel) { (void)self; (void)sel; return 0; }
static double cl_vacc(id self, SEL sel) { (void)self; (void)sel; return 0; }
static double cl_alt(id self, SEL sel) { (void)self; (void)sel; return 0; }
static id cl_timestamp(id self, SEL sel) { (void)self; (void)sel; return NULL; }
static const objc_method_def_t cl_methods[] = {
    {"coordinate", cl_coordinate},
    {"horizontalAccuracy", cl_hacc},
    {"verticalAccuracy", cl_vacc},
    {"altitude", cl_alt},
    {"timestamp", cl_timestamp},
};

/* ------------------------------------------------------------------ */
/* MPMoviePlayerController                                            */
/* ------------------------------------------------------------------ */

Class g_MPMoviePlayerController;

static id mp_init_url(id self, SEL sel, id url) {
    (void)sel; (void)url;
    l_info("[movie] MPMoviePlayerController content set (video skipped)");
    return self;
}
static void mp_set_scaling(id self, SEL sel, int mode) { (void)self; (void)sel; (void)mode; }
static void mp_set_controls(id self, SEL sel, int mode) { (void)self; (void)sel; (void)mode; }
static void mp_play(id self, SEL sel) { (void)self; (void)sel; }
static void mp_stop(id self, SEL sel) { (void)self; (void)sel; }
static const objc_method_def_t mp_methods[] = {
    {"initWithContentURL:", mp_init_url},
    {"setScalingMode:", mp_set_scaling},
    {"setMovieControlMode:", mp_set_controls},
    {"play", mp_play},
    {"stop", mp_stop},
};

void objc_misc_classes_init(void) {
    g_CLLocationManager = objc_define_class("CLLocationManager", NULL, 0x20,
        clm_methods, sizeof(clm_methods)/sizeof(clm_methods[0]),
        clm_cmethods, sizeof(clm_cmethods)/sizeof(clm_cmethods[0]));
    g_CLLocation = objc_define_class("CLLocation", NULL, 0x30,
        cl_methods, sizeof(cl_methods)/sizeof(cl_methods[0]), NULL, 0);
    g_MPMoviePlayerController = objc_define_class("MPMoviePlayerController", NULL, 0x40,
        mp_methods, sizeof(mp_methods)/sizeof(mp_methods[0]), NULL, 0);

    kCLErrorDomain = redeg_nsstring_new("kCLErrorDomain");
    MPMoviePlayerContentPreloadDidFinishNotification = redeg_nsstring_new("MPMoviePlayerContentPreloadDidFinishNotification");
    MPMoviePlayerPlaybackDidFinishNotification = redeg_nsstring_new("MPMoviePlayerPlaybackDidFinishNotification");
}
