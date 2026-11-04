#ifndef REDEG_OBJC_CLASSES_H
#define REDEG_OBJC_CLASSES_H

#include "objc/objc.h"

/* Foundation */
extern Class g_NSString, g_NSNumber, g_NSURL, g_NSDictionary, g_NSArray, g_NSEnumerator;
extern Class g_NSNotificationCenter, g_NSAutoreleasePool, g_NSLocale;

/* UIKit */
extern Class g_UIApplication, g_UIWindow, g_UIView, g_UIViewController;
extern Class g_UIScreen, g_UIDevice, g_UIColor, g_UIImage, g_UIImageView;
extern Class g_UIAccelerometer, g_UIAlertView, g_UITextField;

/* OpenGL ES / QuartzCore */
extern Class g_EAGLContext, g_CAEAGLLayer;

/* Other frameworks */
extern Class g_CLLocationManager, g_CLLocation, g_MPMoviePlayerController;

void objc_foundation_init(void);
void objc_uikit_init(void);
void objc_egl_init(void);
void objc_misc_classes_init(void);

/* UIApplicationMain replacement. */
int redeg_UIApplicationMain(int argc, char **argv, const char *principal, const char *delegate);

/* Frame/runloop hooks used by main.c. */
void redeg_uikit_tick(void);

id redeg_nsstring_new(const char *s);
const char *redeg_string_cstr(id s);
id redeg_make_nsarray1(id item);

/* Data symbols the game expects from the frameworks. */
extern void *kEAGLDrawablePropertyColorFormat;
extern void *kEAGLDrawablePropertyRetainedBacking;
extern void *kEAGLColorFormatRGBA8;
extern double kCLLocationAccuracyBest;
extern double kCLDistanceFilterNone;
extern void *kCLErrorDomain;
extern void *MPMoviePlayerContentPreloadDidFinishNotification;
extern void *MPMoviePlayerPlaybackDidFinishNotification;
extern Class __CFConstantStringClassReference;

/* Fake CoreFoundation string class for __cfstring literals. */
extern Class g_NSCFConstantString;

#endif
