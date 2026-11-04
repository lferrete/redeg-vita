#include "utils/glutil.h"
#include "utils/logger.h"

#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

unsigned int gl_swap_count = 0;

/* vitaGL's debug_utils.h declares this sink; the game port routes it to the
 * same file log instead of sceClibPrintf (invisible on retail). */
void vgl_log(const char *fmt, ...) {
    static int s_vgl_log_count = 0;
    if (s_vgl_log_count >= 32) return;
    s_vgl_log_count++;
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    size_t n = strlen(buf);
    while (n && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = 0;
    if (n) l_error("[vitaGL] %s", buf);
}

void gl_preload(void) {
    sceIoMkdir(DATA_PATH "shader_cache", 0777);
    vglSetShaderCachePath(DATA_PATH "shader_cache");
    vglSetupRenderTargetScenesNum(8, 8);
}

void gl_init(void) {
    static int inited = 0;
    if (inited) return;
    inited = 1;
    gl_preload();
    /* 960x544 display; the game renders its 480x320 viewport inside it. */
    vglInitExtended(0, 960, 544, 16 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE);
    l_success("vitaGL initialized");
}

void gl_swap(void) {
    vglSwapBuffers(GL_FALSE);
    gl_swap_count++;
    if (gl_swap_count <= 5 || (gl_swap_count % 60) == 0)
        l_info("[gl] %u frames", gl_swap_count);
}
