#include <vitaGL.h>
#include <string.h>

extern EGLSurface eglGetCurrentSurface(EGLint readdraw);

static void wrap_glLightf(GLenum light, GLenum pname, GLfloat param) {
    glLightfv(light, pname, &param);
}
static void wrap_glLightx(GLenum light, GLenum pname, GLfixed param) {
    glLightxv(light, pname, &param);
}
static void wrap_glLightModelf(GLenum pname, GLfloat param) {
    glLightModelfv(pname, &param);
}
static void wrap_glLightModelx(GLenum pname, GLfixed param) {
    glLightModelxv(pname, &param);
}
static void wrap_glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    (void)r; (void)q;
    glMultiTexCoord2f(target, s, t);
}
static void wrap_glMultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q) {
    (void)r; (void)q;
    glMultiTexCoord2f(target, (float)s / 65536.0f, (float)t / 65536.0f);
}
static void wrap_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    if (params) glTexParameterf(target, pname, params[0]);
}
static void wrap_glTexParameterxv(GLenum target, GLenum pname, const GLfixed *params) {
    if (params) glTexParameterx(target, pname, params[0]);
}
static void wrap_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    glColor4f(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
}
static GLboolean wrap_glIsBuffer(GLuint buffer) {
    return buffer != 0 ? GL_TRUE : GL_FALSE;
}
static void wrap_glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
    if (!params) return;
    if (pname == GL_TEXTURE_ENV_COLOR) {
        GLfixed fx[4] = { params[0], params[1], params[2], params[3] };
        glTexEnvxv(target, pname, fx);
    } else {
        glTexEnvi(target, pname, params[0]);
    }
}

typedef struct { const char *name; void *addr; } gl_sym_entry_t;

static const gl_sym_entry_t s_gl_syms[] = {
    { "eglChooseConfig", (void *)eglChooseConfig },
    { "eglCreateContext", (void *)eglCreateContext },
    { "eglCreateWindowSurface", (void *)eglCreateWindowSurface },
    { "eglDestroyContext", (void *)eglDestroyContext },
    { "eglDestroySurface", (void *)eglDestroySurface },
    { "eglGetConfigAttrib", (void *)eglGetConfigAttrib },
    { "eglGetConfigs", (void *)eglGetConfigs },
    { "eglGetCurrentContext", (void *)eglGetCurrentContext },
    { "eglGetCurrentSurface", (void *)eglGetCurrentSurface },
    { "eglGetDisplay", (void *)eglGetDisplay },
    { "eglGetError", (void *)eglGetError },
    { "eglGetProcAddress", (void *)eglGetProcAddress },
    { "eglInitialize", (void *)eglInitialize },
    { "eglMakeCurrent", (void *)eglMakeCurrent },
    { "eglQueryContext", (void *)eglQueryContext },
    { "eglQueryString", (void *)eglQueryString },
    { "eglQuerySurface", (void *)eglQuerySurface },
    { "eglSwapBuffers", (void *)eglSwapBuffers },
    { "eglSwapInterval", (void *)eglSwapInterval },
    { "eglTerminate", (void *)eglTerminate },
    { "glActiveTexture", (void *)glActiveTexture },
    { "glAlphaFunc", (void *)glAlphaFunc },
    { "glAlphaFuncx", (void *)glAlphaFuncx },
    { "glBindBuffer", (void *)glBindBuffer },
    { "glBindFramebufferOES", (void *)glBindFramebuffer },
    { "glBindRenderbufferOES", (void *)glBindRenderbuffer },
    { "glBindTexture", (void *)glBindTexture },
    { "glBlendFunc", (void *)glBlendFunc },
    { "glBufferData", (void *)glBufferData },
    { "glBufferSubData", (void *)glBufferSubData },
    { "glCheckFramebufferStatusOES", (void *)glCheckFramebufferStatus },
    { "glClear", (void *)glClear },
    { "glClearColor", (void *)glClearColor },
    { "glClearColorx", (void *)glClearColorx },
    { "glClearDepthf", (void *)glClearDepthf },
    { "glClearDepthx", (void *)glClearDepthx },
    { "glClearStencil", (void *)glClearStencil },
    { "glClientActiveTexture", (void *)glClientActiveTexture },
    { "glClipPlanef", (void *)glClipPlanef },
    { "glClipPlanex", (void *)glClipPlanex },
    { "glColor4f", (void *)glColor4f },
    { "glColor4ub", (void *)wrap_glColor4ub },
    { "glColor4x", (void *)glColor4x },
    { "glColorMask", (void *)glColorMask },
    { "glColorPointer", (void *)glColorPointer },
    { "glCompressedTexImage2D", (void *)glCompressedTexImage2D },
    { "glCopyTexImage2D", (void *)glCopyTexImage2D },
    { "glCopyTexSubImage2D", (void *)glCopyTexSubImage2D },
    { "glCullFace", (void *)glCullFace },
    { "glDeleteBuffers", (void *)glDeleteBuffers },
    { "glDeleteFramebuffersOES", (void *)glDeleteFramebuffers },
    { "glDeleteRenderbuffersOES", (void *)glDeleteRenderbuffers },
    { "glDeleteTextures", (void *)glDeleteTextures },
    { "glDepthFunc", (void *)glDepthFunc },
    { "glDepthMask", (void *)glDepthMask },
    { "glDepthRangef", (void *)glDepthRangef },
    { "glDepthRangex", (void *)glDepthRangex },
    { "glDisable", (void *)glDisable },
    { "glDisableClientState", (void *)glDisableClientState },
    { "glDrawArrays", (void *)glDrawArrays },
    { "glDrawElements", (void *)glDrawElements },
    { "glEnable", (void *)glEnable },
    { "glEnableClientState", (void *)glEnableClientState },
    { "glFinish", (void *)glFinish },
    { "glFlush", (void *)glFlush },
    { "glFogf", (void *)glFogf },
    { "glFogfv", (void *)glFogfv },
    { "glFogx", (void *)glFogx },
    { "glFogxv", (void *)glFogxv },
    { "glFramebufferRenderbufferOES", (void *)glFramebufferRenderbuffer },
    { "glFramebufferTexture2DOES", (void *)glFramebufferTexture2D },
    { "glFrontFace", (void *)glFrontFace },
    { "glFrustumf", (void *)glFrustumf },
    { "glFrustumx", (void *)glFrustumx },
    { "glGenBuffers", (void *)glGenBuffers },
    { "glGenFramebuffersOES", (void *)glGenFramebuffers },
    { "glGenRenderbuffersOES", (void *)glGenRenderbuffers },
    { "glGenTextures", (void *)glGenTextures },
    { "glGetBooleanv", (void *)glGetBooleanv },
    { "glGetBufferParameteriv", (void *)glGetBufferParameteriv },
    { "glGetError", (void *)glGetError },
    { "glGetFloatv", (void *)glGetFloatv },
    { "glGetIntegerv", (void *)glGetIntegerv },
    { "glGetPointerv", (void *)glGetPointerv },
    { "glGetString", (void *)glGetString },
    { "glGetTexEnviv", (void *)glGetTexEnviv },
    { "glHint", (void *)glHint },
    { "glIsBuffer", (void *)wrap_glIsBuffer },
    { "glIsEnabled", (void *)glIsEnabled },
    { "glIsFramebufferOES", (void *)glIsFramebuffer },
    { "glIsRenderbufferOES", (void *)glIsRenderbuffer },
    { "glIsTexture", (void *)glIsTexture },
    { "glLightModelf", (void *)wrap_glLightModelf },
    { "glLightModelfv", (void *)glLightModelfv },
    { "glLightModelx", (void *)wrap_glLightModelx },
    { "glLightModelxv", (void *)glLightModelxv },
    { "glLightf", (void *)wrap_glLightf },
    { "glLightfv", (void *)glLightfv },
    { "glLightx", (void *)wrap_glLightx },
    { "glLightxv", (void *)glLightxv },
    { "glLineWidth", (void *)glLineWidth },
    { "glLineWidthx", (void *)glLineWidthx },
    { "glLoadIdentity", (void *)glLoadIdentity },
    { "glLoadMatrixf", (void *)glLoadMatrixf },
    { "glLoadMatrixx", (void *)glLoadMatrixx },
    { "glMaterialf", (void *)glMaterialf },
    { "glMaterialfv", (void *)glMaterialfv },
    { "glMaterialx", (void *)glMaterialx },
    { "glMaterialxv", (void *)glMaterialxv },
    { "glMatrixMode", (void *)glMatrixMode },
    { "glMultMatrixf", (void *)glMultMatrixf },
    { "glMultMatrixx", (void *)glMultMatrixx },
    { "glMultiTexCoord4f", (void *)wrap_glMultiTexCoord4f },
    { "glMultiTexCoord4x", (void *)wrap_glMultiTexCoord4x },
    { "glNormal3f", (void *)glNormal3f },
    { "glNormal3x", (void *)glNormal3x },
    { "glNormalPointer", (void *)glNormalPointer },
    { "glOrthof", (void *)glOrthof },
    { "glOrthox", (void *)glOrthox },
    { "glPixelStorei", (void *)glPixelStorei },
    { "glPointSize", (void *)glPointSize },
    { "glPointSizex", (void *)glPointSizex },
    { "glPolygonOffset", (void *)glPolygonOffset },
    { "glPolygonOffsetx", (void *)glPolygonOffsetx },
    { "glPopMatrix", (void *)glPopMatrix },
    { "glPushMatrix", (void *)glPushMatrix },
    { "glReadPixels", (void *)glReadPixels },
    { "glRenderbufferStorageOES", (void *)glRenderbufferStorage },
    { "glRotatef", (void *)glRotatef },
    { "glRotatex", (void *)glRotatex },
    { "glScalef", (void *)glScalef },
    { "glScalex", (void *)glScalex },
    { "glScissor", (void *)glScissor },
    { "glShadeModel", (void *)glShadeModel },
    { "glStencilFunc", (void *)glStencilFunc },
    { "glStencilMask", (void *)glStencilMask },
    { "glStencilOp", (void *)glStencilOp },
    { "glTexCoordPointer", (void *)glTexCoordPointer },
    { "glTexEnvf", (void *)glTexEnvf },
    { "glTexEnvfv", (void *)glTexEnvfv },
    { "glTexEnvi", (void *)glTexEnvi },
    { "glTexEnviv", (void *)wrap_glTexEnviv },
    { "glTexEnvx", (void *)glTexEnvx },
    { "glTexEnvxv", (void *)glTexEnvxv },
    { "glTexImage2D", (void *)glTexImage2D },
    { "glTexParameterf", (void *)glTexParameterf },
    { "glTexParameterfv", (void *)wrap_glTexParameterfv },
    { "glTexParameteri", (void *)glTexParameteri },
    { "glTexParameteriv", (void *)glTexParameteriv },
    { "glTexParameterx", (void *)glTexParameterx },
    { "glTexParameterxv", (void *)wrap_glTexParameterxv },
    { "glTexSubImage2D", (void *)glTexSubImage2D },
    { "glTranslatef", (void *)glTranslatef },
    { "glTranslatex", (void *)glTranslatex },
    { "glVertexPointer", (void *)glVertexPointer },
    { "glViewport", (void *)glViewport },
};

void *redeg_lookup_gl(const char *name) {
    for (unsigned i = 0; i < sizeof(s_gl_syms) / sizeof(s_gl_syms[0]); i++)
        if (s_gl_syms[i].name[0] == name[0] && strcmp(s_gl_syms[i].name, name) == 0)
            return s_gl_syms[i].addr;
    return 0;
}
