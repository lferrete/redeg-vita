#include <vitaGL.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "utils/logger.h"

extern EGLSurface eglGetCurrentSurface(EGLint readdraw);

static GLint s_vtx_size = 0;
static GLenum s_vtx_type = 0;
static GLsizei s_vtx_stride = 0;
static const GLvoid *s_vtx_ptr = NULL;
static GLuint s_vtx_vbo = 0;
static void *s_vtx_lr = NULL;
static GLuint s_arr_buf = 0;
static GLuint s_elem_buf = 0;
static uint32_t s_gl_frame = 0;
static uint32_t s_gl_draw3d_idx = 0;
static uint32_t s_3d_total = 0;
static uint32_t s_anomaly_logs = 0;

typedef void (*vert_stream_fn_t)(const int16_t *verts, int num_verts);
static vert_stream_fn_t s_orig_vs_model = NULL;
static vert_stream_fn_t s_orig_vs_view = NULL;

#define MAX_SKIN_VERTS 4096
static int32_t s_skin_v32[MAX_SKIN_VERTS * 3];
static const int16_t *s_skin_v32_tag = NULL;
static int s_skin_v32_count = 0;
static uint32_t s_skin_overflow_verts = 0;

static inline int16_t clamp_s16(int32_t v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (int16_t)v;
}

static inline void iwanim_transform_vec(const int32_t *m, const int16_t *v,
                                        int32_t *ox, int32_t *oy, int32_t *oz) {
    int64_t vx = v[0];
    int64_t vy = v[1];
    int64_t vz = v[2];
    *ox = (int32_t)((m[0] * vx + m[3] * vy + m[6] * vz) >> 12) + m[9];
    *oy = (int32_t)((m[1] * vx + m[4] * vy + m[7] * vz) >> 12) + m[10];
    *oz = (int32_t)((m[2] * vx + m[5] * vy + m[8] * vz) >> 12) + m[11];
}

typedef struct {
    uint32_t out_buf;
    const int16_t *in_verts;
    int num_verts;
    int mode;
    void *caller_lr;
    uint32_t flags0;
    uint32_t flags1;
} vs_trace_t;

static vs_trace_t s_vs_ring[128];
static uint32_t s_vs_ring_idx = 0;

static void record_vs_trace(const int16_t *verts, int num_verts, int mode, void *caller_lr) {
    uint8_t *iwgx = *(uint8_t **)0x98236484;
    if (!iwgx) return;
    vs_trace_t *t = &s_vs_ring[s_vs_ring_idx & 127];
    s_vs_ring_idx++;
    t->out_buf = *(uint32_t *)(iwgx + 0x244);
    t->in_verts = verts;
    t->num_verts = num_verts;
    t->mode = mode;
    t->caller_lr = caller_lr;
    t->flags0 = *(uint32_t *)(iwgx + 0x00);
    t->flags1 = *(uint32_t *)(iwgx + 0x04);
}

static void wrap_vs_model(const int16_t *verts, int num_verts) {
    void *lr = __builtin_return_address(0);
    s_orig_vs_model(verts, num_verts);
    if (s_skin_v32_tag && verts == s_skin_v32_tag && num_verts == s_skin_v32_count && num_verts <= MAX_SKIN_VERTS) {
        uint8_t *iwgx = *(uint8_t **)0x98236484;
        uint8_t *v_out = iwgx ? *(uint8_t **)(iwgx + 0x244) : NULL;
        if (iwgx && v_out) {
            uint32_t flags = *(uint32_t *)(iwgx + 4);
            int32_t near_z = *(int32_t *)(iwgx + 0x1ac) << 6;
            int32_t far_z  = *(int32_t *)(iwgx + 0x1b0) << 6;
            uint16_t pad18 = *(uint16_t *)(iwgx + 0x3f8);
            if (flags & 0x10) {
                const int32_t *m = (const int32_t *)(iwgx + 0x174);
                for (int i = 0; i < num_verts; i++) {
                    int64_t vx = s_skin_v32[i * 3 + 0];
                    int64_t vy = s_skin_v32[i * 3 + 1];
                    int64_t vz = s_skin_v32[i * 3 + 2];
                    int32_t ox = ((int32_t)((m[3] * vy + m[0] * vx + m[6] * vz) >> 14) + m[9]) >> 8;
                    int32_t oy = ((int32_t)((m[4] * vy + m[1] * vx + m[7] * vz) >> 14) + m[10]) >> 8;
                    int32_t oz = ((int32_t)((m[5] * vy + m[2] * vx + m[8] * vz) >> 14) + m[11]) >> 8;
                    uint8_t *dst = v_out + i * 20;
                    *(int32_t *)(dst + 4)  = ox;
                    *(int32_t *)(dst + 8)  = oy;
                    *(int32_t *)(dst + 12) = oz;
                    uint16_t clip = 0x80;
                    if (ox > oz)  clip |= 4;
                    if (-ox > oz) clip |= 8;
                    if (oy > oz)  clip |= 16;
                    if (-oy > oz) clip |= 32;
                    if (oz < near_z) clip |= 1;
                    else if (oz > far_z) clip |= 2;
                    *(uint16_t *)(dst + 16) = clip;
                    *(uint16_t *)(dst + 18) = pad18;
                }
            } else {
                const int32_t *m = (const int32_t *)(iwgx + 0x100);
                for (int i = 0; i < num_verts; i++) {
                    int64_t vx = s_skin_v32[i * 3 + 0];
                    int64_t vy = s_skin_v32[i * 3 + 1];
                    int64_t vz = s_skin_v32[i * 3 + 2];
                    int32_t ox = (int32_t)((m[3] * vy + m[0] * vx + m[6] * vz) >> 12) + m[9];
                    int32_t oy = (int32_t)((m[4] * vy + m[1] * vx + m[7] * vz) >> 12) + m[10];
                    int32_t oz = (int32_t)((m[5] * vy + m[2] * vx + m[8] * vz) >> 12) + m[11];
                    uint8_t *dst = v_out + i * 20;
                    *(int32_t *)(dst + 4)  = ox;
                    *(int32_t *)(dst + 8)  = oy;
                    *(int32_t *)(dst + 12) = oz;
                    uint16_t clip = 0x80;
                    if (ox > oz)  clip |= 4;
                    if (-ox > oz) clip |= 8;
                    if (oy > oz)  clip |= 16;
                    if (-oy > oz) clip |= 32;
                    if (oz < near_z) clip |= 1;
                    else if (oz > far_z) clip |= 2;
                    *(uint16_t *)(dst + 16) = clip;
                    *(uint16_t *)(dst + 18) = pad18;
                }
            }
        }
    }
    record_vs_trace(verts, num_verts, 1, lr);
}

static void wrap_vs_view(const int16_t *verts, int num_verts) {
    void *lr = __builtin_return_address(0);
    s_orig_vs_view(verts, num_verts);
    record_vs_trace(verts, num_verts, 2, lr);
}

static void hook_vs_table_if_needed(void) {
    uint32_t *vtbl = (uint32_t *)0x98243b28;
    if (vtbl[2] && vtbl[2] != (uint32_t)wrap_vs_model) {
        s_orig_vs_model = (vert_stream_fn_t)vtbl[2];
        vtbl[2] = (uint32_t)wrap_vs_model;
    }
    if (vtbl[3] && vtbl[3] != (uint32_t)wrap_vs_view) {
        s_orig_vs_view = (vert_stream_fn_t)vtbl[3];
        vtbl[3] = (uint32_t)wrap_vs_view;
    }
}

int redeg_IwAnimSkin_Offset(uint8_t *model, uint8_t *block) {
    hook_vs_table_if_needed();
    uint8_t *anim = *(uint8_t **)0x98236d78;
    uint8_t *skel = *(uint8_t **)(anim + 4);
    uint8_t *skin = *(uint8_t **)(anim + 8);
    ((void (*)(int32_t))0x98041944)(*(int32_t *)(skel + 32));

    uint16_t num_verts = *(uint16_t *)(block + 14);
    static int s_dbg67_count = 0;
    static int s_dbg_extreme_count = 0;
    int dbg_call = 0;
    if (num_verts == 67 && s_gl_frame >= 285 && s_dbg67_count < 2) {
        dbg_call = 1;
        s_dbg67_count++;
    }
    int16_t *out_s16 = *(int16_t **)(anim + 12);
    if (dbg_call) {
        uint32_t nb = *(uint32_t *)(skel + 20);
        uint8_t **b_arr = *(uint8_t ***)(skel + 16);
        l_info("[SKIN-DBG] f=%u nv=%u skel=%p scale=%d nb=%u out_s16=%p dup=%u..%u",
                 s_gl_frame, num_verts, skel, *(int32_t *)(skel + 32), nb, out_s16,
                 *(const uint16_t *)(model + 0x5a), *(const uint16_t *)(model + 0x58));
        for (uint32_t bi = 0; bi < nb; bi++) {
            const int32_t *bm = (const int32_t *)(b_arr[bi] + 36);
            l_info("[SKIN-BONE] b=%u flg=0x%x r0=(%d,%d,%d) r1=(%d,%d,%d) r2=(%d,%d,%d) t=(%d,%d,%d)",
                     bi, *(uint16_t *)(b_arr[bi] + 20),
                     bm[0], bm[3], bm[6],
                     bm[1], bm[4], bm[7],
                     bm[2], bm[5], bm[8],
                     bm[9], bm[10], bm[11]);
        }
    }
    if (out_s16 != NULL) {
        uint32_t num_bones = *(uint32_t *)(skel + 20);
        uint8_t **bones = *(uint8_t ***)(skel + 16);
        for (uint32_t i = 0; i < num_bones; i++) {
            uint16_t bf = *(uint16_t *)(bones[i] + 20);
            if (bf & 2) *(uint16_t *)(bones[i] + 20) = bf & ~2u;
        }
    } else {
        uint8_t *iwgx = *(uint8_t **)0x98236484;
        uint32_t alloc_bytes = ((uint32_t)num_verts * 6u + 3u) & ~3u;
        out_s16 = *(int16_t **)(iwgx + 0x258);
        *(int16_t **)(iwgx + 0x254) = out_s16;
        *(uint8_t **)(iwgx + 0x258) = (uint8_t *)out_s16 + alloc_bytes;
    }

    *(uint32_t *)0x98236d54 = *(uint32_t *)(block + 28);
    *(uint32_t *)0x98236d58 = 0;

    uint8_t **groups = *(uint8_t ***)(skin + 28);
    uint32_t num_groups = *(uint32_t *)(skin + 32);
    uint8_t **bones = *(uint8_t ***)(skel + 16);

    for (uint32_t g = 0; g < num_groups; g++) {
        uint8_t *grp = groups[g];
        uint32_t vcount = *(uint32_t *)(grp + 12);
        const uint8_t *b = *(const uint8_t **)(grp + 20);
        int n_inf;
        const uint8_t *p;
        if (b[1] == 0xff) {
            n_inf = 1;
            p = b + 2;
        } else if (b[2] == 0xff) {
            n_inf = 2;
            p = b + 4;
        } else if (b[3] == 0xff) {
            n_inf = 3;
            p = b + 4;
        } else {
            n_inf = 4;
            p = b + 4;
        }
        if (vcount == 0) continue;

        const int32_t *m0 = (const int32_t *)(bones[b[0]] + 36);
        if (n_inf == 1) {
            for (uint32_t v = 0; v < vcount; v++) {
                uint16_t raw_vi = *(const uint16_t *)p;
                uint16_t vi = raw_vi & ~0xc000u;
                const int16_t *off0 = (const int16_t *)(p + 2);
                p += 10;
                int32_t x, y, z;
                iwanim_transform_vec(m0, off0, &x, &y, &z);
                int is_ext = (x > 48000 || x < -48000 || y > 48000 || y < -48000 || z > 48000 || z < -48000);
                if ((dbg_call && (vi == 52 || vi == 36 || vi == 32 || vi == 65 || vi == 81 || vi == 53 || vi == 64 || vi == 40 || vi == 33 || vi == 6 || vi == 12 || vi == 7 || vi == 9 || vi == 8 || vi == 10 || vi == 5 || vi == 104 || vi == 15 || vi == 14 || vi == 16)) ||
                    (is_ext && s_dbg_extreme_count < 30)) {
                    if (is_ext) s_dbg_extreme_count++;
                    l_info("[SKIN-V1] f=%u nv=%u g=%u vi=%u(0x%x) b0=%u off0=(%d,%d,%d) m0_t=(%d,%d,%d) -> xyz=(%d,%d,%d)",
                             s_gl_frame, num_verts, g, vi, raw_vi, b[0], off0[0], off0[1], off0[2], m0[9], m0[10], m0[11], x, y, z);
                }
                if (x > 32767 || x < -32768 || y > 32767 || y < -32768 || z > 32767 || z < -32768) {
                    s_skin_overflow_verts++;
                }
                if (vi < MAX_SKIN_VERTS) {
                    s_skin_v32[vi * 3 + 0] = x;
                    s_skin_v32[vi * 3 + 1] = y;
                    s_skin_v32[vi * 3 + 2] = z;
                }
                out_s16[vi * 3 + 0] = clamp_s16(x);
                out_s16[vi * 3 + 1] = clamp_s16(y);
                out_s16[vi * 3 + 2] = clamp_s16(z);
            }
        } else if (n_inf == 2) {
            const int32_t *m1 = (const int32_t *)(bones[b[1]] + 36);
            for (uint32_t v = 0; v < vcount; v++) {
                uint16_t raw_vi = *(const uint16_t *)p;
                uint16_t vi = raw_vi & ~0xc000u;
                const int16_t *off0 = (const int16_t *)(p + 2);
                const int16_t *off1 = (const int16_t *)(p + 8);
                uint8_t w0 = p[14];
                uint8_t w1 = p[15];
                p += 16;
                int32_t x0, y0, z0, x1, y1, z1, x, y, z;
                iwanim_transform_vec(m0, off0, &x0, &y0, &z0);
                iwanim_transform_vec(m1, off1, &x1, &y1, &z1);
                if (w0 == 0x40) {
                    x = (x0 >> 1) + (x1 >> 1);
                    y = (y0 >> 1) + (y1 >> 1);
                    z = (z0 >> 1) + (z1 >> 1);
                } else {
                    int64_t s0 = (int32_t)w0 << 5;
                    int64_t s1 = (int32_t)w1 << 5;
                    x = (int32_t)((x0 * s0) >> 12) + (int32_t)((x1 * s1) >> 12);
                    y = (int32_t)((y0 * s0) >> 12) + (int32_t)((y1 * s1) >> 12);
                    z = (int32_t)((z0 * s0) >> 12) + (int32_t)((z1 * s1) >> 12);
                }
                int is_ext = (x > 48000 || x < -48000 || y > 48000 || y < -48000 || z > 48000 || z < -48000);
                if ((dbg_call && (vi == 52 || vi == 36 || vi == 32 || vi == 65 || vi == 81 || vi == 53 || vi == 64 || vi == 40 || vi == 33 || vi == 6 || vi == 12 || vi == 7 || vi == 9 || vi == 8 || vi == 10 || vi == 5 || vi == 104 || vi == 15 || vi == 14 || vi == 16)) ||
                    (is_ext && s_dbg_extreme_count < 30)) {
                    if (is_ext) s_dbg_extreme_count++;
                    l_info("[SKIN-V2] f=%u nv=%u g=%u vi=%u(0x%x) b=(%u,%u) w=(%u,%u) off0=(%d,%d,%d) off1=(%d,%d,%d) m0_t=(%d,%d,%d) m1_t=(%d,%d,%d) v0=(%d,%d,%d) v1=(%d,%d,%d) -> xyz=(%d,%d,%d)",
                             s_gl_frame, num_verts, g, vi, raw_vi, b[0], b[1], w0, w1,
                             off0[0], off0[1], off0[2], off1[0], off1[1], off1[2],
                             m0[9], m0[10], m0[11], m1[9], m1[10], m1[11],
                             x0, y0, z0, x1, y1, z1, x, y, z);
                }
                if (x > 32767 || x < -32768 || y > 32767 || y < -32768 || z > 32767 || z < -32768) {
                    s_skin_overflow_verts++;
                }
                if (vi < MAX_SKIN_VERTS) {
                    s_skin_v32[vi * 3 + 0] = x;
                    s_skin_v32[vi * 3 + 1] = y;
                    s_skin_v32[vi * 3 + 2] = z;
                }
                out_s16[vi * 3 + 0] = clamp_s16(x);
                out_s16[vi * 3 + 1] = clamp_s16(y);
                out_s16[vi * 3 + 2] = clamp_s16(z);
            }
        } else if (n_inf == 3) {
            const int32_t *m1 = (const int32_t *)(bones[b[1]] + 36);
            const int32_t *m2 = (const int32_t *)(bones[b[2]] + 36);
            for (uint32_t v = 0; v < vcount; v++) {
                uint16_t raw_vi = *(const uint16_t *)p;
                uint16_t vi = raw_vi & ~0xc000u;
                const int16_t *off0 = (const int16_t *)(p + 2);
                const int16_t *off1 = (const int16_t *)(p + 8);
                const int16_t *off2 = (const int16_t *)(p + 14);
                uint8_t w0 = p[20], w1 = p[21], w2 = p[22];
                int64_t s0 = (int32_t)w0 << 5;
                int64_t s1 = (int32_t)w1 << 5;
                int64_t s2 = (int32_t)w2 << 5;
                p += 24;
                int32_t x0, y0, z0, x1, y1, z1, x2, y2, z2;
                iwanim_transform_vec(m0, off0, &x0, &y0, &z0);
                iwanim_transform_vec(m1, off1, &x1, &y1, &z1);
                iwanim_transform_vec(m2, off2, &x2, &y2, &z2);
                int32_t x = (int32_t)((x0 * s0) >> 12) + (int32_t)((x1 * s1) >> 12) + (int32_t)((x2 * s2) >> 12);
                int32_t y = (int32_t)((y0 * s0) >> 12) + (int32_t)((y1 * s1) >> 12) + (int32_t)((y2 * s2) >> 12);
                int32_t z = (int32_t)((z0 * s0) >> 12) + (int32_t)((z1 * s1) >> 12) + (int32_t)((z2 * s2) >> 12);
                int is_ext = (x > 48000 || x < -48000 || y > 48000 || y < -48000 || z > 48000 || z < -48000);
                if ((dbg_call && (vi == 52 || vi == 36 || vi == 32 || vi == 65 || vi == 81 || vi == 53 || vi == 64 || vi == 40 || vi == 33 || vi == 6 || vi == 12 || vi == 7 || vi == 9 || vi == 8 || vi == 10 || vi == 5 || vi == 104 || vi == 15 || vi == 14 || vi == 16)) ||
                    (is_ext && s_dbg_extreme_count < 30)) {
                    if (is_ext) s_dbg_extreme_count++;
                    l_info("[SKIN-V3] f=%u nv=%u g=%u vi=%u(0x%x) b=(%u,%u,%u) w=(%u,%u,%u) off0=(%d,%d,%d) off1=(%d,%d,%d) off2=(%d,%d,%d) m0_t=(%d,%d,%d) m1_t=(%d,%d,%d) m2_t=(%d,%d,%d) v0=(%d,%d,%d) v1=(%d,%d,%d) v2=(%d,%d,%d) -> xyz=(%d,%d,%d)",
                             s_gl_frame, num_verts, g, vi, raw_vi, b[0], b[1], b[2], w0, w1, w2,
                             off0[0], off0[1], off0[2], off1[0], off1[1], off1[2], off2[0], off2[1], off2[2],
                             m0[9], m0[10], m0[11], m1[9], m1[10], m1[11], m2[9], m2[10], m2[11],
                             x0, y0, z0, x1, y1, z1, x2, y2, z2, x, y, z);
                }
                if (x > 32767 || x < -32768 || y > 32767 || y < -32768 || z > 32767 || z < -32768) {
                    s_skin_overflow_verts++;
                }
                if (vi < MAX_SKIN_VERTS) {
                    s_skin_v32[vi * 3 + 0] = x;
                    s_skin_v32[vi * 3 + 1] = y;
                    s_skin_v32[vi * 3 + 2] = z;
                }
                out_s16[vi * 3 + 0] = clamp_s16(x);
                out_s16[vi * 3 + 1] = clamp_s16(y);
                out_s16[vi * 3 + 2] = clamp_s16(z);
            }
        } else {
            const int32_t *m1 = (const int32_t *)(bones[b[1]] + 36);
            const int32_t *m2 = (const int32_t *)(bones[b[2]] + 36);
            const int32_t *m3 = (const int32_t *)(bones[b[3]] + 36);
            for (uint32_t v = 0; v < vcount; v++) {
                uint16_t raw_vi = *(const uint16_t *)p;
                uint16_t vi = raw_vi & ~0xc000u;
                const int16_t *off0 = (const int16_t *)(p + 2);
                const int16_t *off1 = (const int16_t *)(p + 8);
                const int16_t *off2 = (const int16_t *)(p + 14);
                const int16_t *off3 = (const int16_t *)(p + 20);
                uint8_t w0 = p[26], w1 = p[27], w2 = p[28], w3 = p[29];
                int64_t s0 = (int32_t)w0 << 5;
                int64_t s1 = (int32_t)w1 << 5;
                int64_t s2 = (int32_t)w2 << 5;
                int64_t s3 = (int32_t)w3 << 5;
                p += 30;
                int32_t x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
                iwanim_transform_vec(m0, off0, &x0, &y0, &z0);
                iwanim_transform_vec(m1, off1, &x1, &y1, &z1);
                iwanim_transform_vec(m2, off2, &x2, &y2, &z2);
                iwanim_transform_vec(m3, off3, &x3, &y3, &z3);
                int32_t x = (int32_t)((x0 * s0) >> 12) + (int32_t)((x1 * s1) >> 12) + (int32_t)((x2 * s2) >> 12) + (int32_t)((x3 * s3) >> 12);
                int32_t y = (int32_t)((y0 * s0) >> 12) + (int32_t)((y1 * s1) >> 12) + (int32_t)((y2 * s2) >> 12) + (int32_t)((y3 * s3) >> 12);
                int32_t z = (int32_t)((z0 * s0) >> 12) + (int32_t)((z1 * s1) >> 12) + (int32_t)((z2 * s2) >> 12) + (int32_t)((z3 * s3) >> 12);
                int is_ext = (x > 48000 || x < -48000 || y > 48000 || y < -48000 || z > 48000 || z < -48000);
                if ((dbg_call && (vi == 52 || vi == 36 || vi == 32 || vi == 65 || vi == 81 || vi == 53 || vi == 64 || vi == 40 || vi == 33 || vi == 6 || vi == 12 || vi == 7 || vi == 9 || vi == 8 || vi == 10 || vi == 5 || vi == 104 || vi == 15 || vi == 14 || vi == 16)) ||
                    (is_ext && s_dbg_extreme_count < 30)) {
                    if (is_ext) s_dbg_extreme_count++;
                    l_info("[SKIN-V4] f=%u nv=%u g=%u vi=%u(0x%x) b=(%u,%u,%u,%u) w=(%u,%u,%u,%u) off0=(%d,%d,%d) off1=(%d,%d,%d) off2=(%d,%d,%d) off3=(%d,%d,%d) m0_t=(%d,%d,%d) m1_t=(%d,%d,%d) m2_t=(%d,%d,%d) m3_t=(%d,%d,%d) v0=(%d,%d,%d) v1=(%d,%d,%d) v2=(%d,%d,%d) v3=(%d,%d,%d) -> xyz=(%d,%d,%d)",
                             s_gl_frame, num_verts, g, vi, raw_vi, b[0], b[1], b[2], b[3], w0, w1, w2, w3,
                             off0[0], off0[1], off0[2], off1[0], off1[1], off1[2], off2[0], off2[1], off2[2], off3[0], off3[1], off3[2],
                             m0[9], m0[10], m0[11], m1[9], m1[10], m1[11], m2[9], m2[10], m2[11], m3[9], m3[10], m3[11],
                             x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, x, y, z);
                }
                if (x > 32767 || x < -32768 || y > 32767 || y < -32768 || z > 32767 || z < -32768) {
                    s_skin_overflow_verts++;
                }
                if (vi < MAX_SKIN_VERTS) {
                    s_skin_v32[vi * 3 + 0] = x;
                    s_skin_v32[vi * 3 + 1] = y;
                    s_skin_v32[vi * 3 + 2] = z;
                }
                out_s16[vi * 3 + 0] = clamp_s16(x);
                out_s16[vi * 3 + 1] = clamp_s16(y);
                out_s16[vi * 3 + 2] = clamp_s16(z);
            }
        }
    }

    uint16_t dup_end = *(const uint16_t *)(model + 0x58);
    uint16_t dup_start = *(const uint16_t *)(model + 0x5a);
    if (dup_start < dup_end) {
        const uint16_t *dup_map = *(const uint16_t **)(model + 0x64);
        for (uint16_t idx = dup_start; idx < dup_end; idx++) {
            uint16_t src = dup_map[(uint32_t)idx * 3u];
            if (dbg_call && (idx == 52 || idx == 36 || idx == 32 || idx == 65 || idx == 81 || idx == 53 || idx == 64 || idx == 40 || idx == 33 || idx == 6 || idx == 12 || idx == 7 || idx == 9 || idx == 8 || idx == 10 || idx == 5 || idx == 104 || idx == 15 || idx == 14 || idx == 16)) {
                l_info("[SKIN-DUP] f=%u nv=%u idx=%u <- src=%u xyz=(%d,%d,%d)",
                         s_gl_frame, num_verts, idx, src,
                         s_skin_v32[src * 3 + 0], s_skin_v32[src * 3 + 1], s_skin_v32[src * 3 + 2]);
            }
            if (idx < MAX_SKIN_VERTS && src < MAX_SKIN_VERTS) {
                s_skin_v32[idx * 3 + 0] = s_skin_v32[src * 3 + 0];
                s_skin_v32[idx * 3 + 1] = s_skin_v32[src * 3 + 1];
                s_skin_v32[idx * 3 + 2] = s_skin_v32[src * 3 + 2];
            }
            out_s16[idx * 3 + 0] = out_s16[src * 3 + 0];
            out_s16[idx * 3 + 1] = out_s16[src * 3 + 1];
            out_s16[idx * 3 + 2] = out_s16[src * 3 + 2];
        }
    }

    s_skin_v32_tag = out_s16;
    s_skin_v32_count = num_verts;
    vert_stream_fn_t fn = *(vert_stream_fn_t *)(0x98243b28 + 8);
    fn(out_s16, num_verts);
    s_skin_v32_tag = NULL;
    return 0;
}

static void wrap_glClear(GLbitfield mask) {
    hook_vs_table_if_needed();
    if (mask & GL_DEPTH_BUFFER_BIT) {
        s_gl_frame++;
        s_gl_draw3d_idx = 0;
        if ((s_gl_frame % 60) == 0 && s_skin_overflow_verts > 0) {
            l_info("[skin32] frame=%u rescued_overflow_verts=%u anom_logs=%u",
                   s_gl_frame, s_skin_overflow_verts, s_anomaly_logs);
        }
    }
    glClear(mask);
}

static void wrap_glBindBuffer(GLenum target, GLuint buffer) {
    if (target == GL_ARRAY_BUFFER) s_arr_buf = buffer;
    else if (target == GL_ELEMENT_ARRAY_BUFFER) s_elem_buf = buffer;
    glBindBuffer(target, buffer);
}

static void wrap_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    s_vtx_size = size;
    s_vtx_type = type;
    s_vtx_stride = stride;
    s_vtx_ptr = pointer;
    s_vtx_vbo = s_arr_buf;
    s_vtx_lr = __builtin_return_address(0);
    glVertexPointer(size, type, stride, pointer);
}

static const uint8_t *get_vtx_base(void) {
    if (s_vtx_vbo) {
        void *vbo_ptr = *(void **)s_vtx_vbo;
        if (!vbo_ptr) return NULL;
        return (const uint8_t *)vbo_ptr + (uintptr_t)s_vtx_ptr;
    }
    if ((uintptr_t)s_vtx_ptr < 0x81000000) return NULL;
    return (const uint8_t *)s_vtx_ptr;
}

static int read_vtx_xyz(const uint8_t *base, uint16_t idx, float *x, float *y, float *z, uint16_t *cf) {
    if (!base) return 0;
    if (cf) *cf = 0;
    if (s_vtx_type == GL_SHORT) {
        int stride = s_vtx_stride ? s_vtx_stride : 6;
        const int16_t *v = (const int16_t *)(base + (uint32_t)idx * stride);
        *x = (float)v[0];
        *y = (float)v[1];
        *z = (float)v[2];
        return 1;
    } else if (s_vtx_type == 0x140C) {
        int stride = s_vtx_stride ? s_vtx_stride : 12;
        const int32_t *v = (const int32_t *)(base + (uint32_t)idx * stride);
        *x = (float)v[0] * (1.0f / 65536.0f);
        *y = (float)v[1] * (1.0f / 65536.0f);
        *z = (float)v[2] * (1.0f / 65536.0f);
        if (cf && stride >= 16) {
            *cf = ((const uint16_t *)v)[6];
        }
        return 1;
    } else if (s_vtx_type == GL_FLOAT) {
        int stride = s_vtx_stride ? s_vtx_stride : 12;
        const float *v = (const float *)(base + (uint32_t)idx * stride);
        *x = v[0];
        *y = v[1];
        *z = v[2];
        return 1;
    } else if (s_vtx_type == GL_BYTE) {
        int stride = s_vtx_stride ? s_vtx_stride : 3;
        const int8_t *v = (const int8_t *)(base + (uint32_t)idx * stride);
        *x = (float)v[0];
        *y = (float)v[1];
        *z = (float)v[2];
        return 1;
    }
    return 0;
}

static void wrap_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    void *draw_lr = __builtin_return_address(0);
    if (s_vtx_size == 3 && count >= 3 && type == GL_UNSIGNED_SHORT &&
        (mode == GL_TRIANGLES || mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN)) {
        s_3d_total++;
        const uint8_t *vbase = get_vtx_base();
        const uint16_t *idx = NULL;
        if (s_elem_buf) {
            void *ibo_ptr = *(void **)s_elem_buf;
            if (ibo_ptr) idx = (const uint16_t *)((const uint8_t *)ibo_ptr + (uintptr_t)indices);
        } else if ((uintptr_t)indices >= 0x81000000) {
            idx = (const uint16_t *)indices;
        }
        if (vbase && idx) {
            int num_tris = 0;
            int near_tris = 0;
            float sum_edge = 0.0f;
            float max_edge = 0.0f;
            int max_t = -1;
            uint16_t max_i0 = 0, max_i1 = 0, max_i2 = 0;
            uint16_t min_idx = 0xFFFF, max_idx = 0;
            for (int i = 0; i < count; i++) {
                if (idx[i] < min_idx) min_idx = idx[i];
                if (idx[i] > max_idx) max_idx = idx[i];
            }
            int limit = count - 2;
            int step = (mode == GL_TRIANGLES) ? 3 : 1;
            for (int i = 0; i < limit; i += step) {
                uint16_t i0 = (mode == GL_TRIANGLE_FAN) ? idx[0] : idx[i];
                uint16_t i1 = idx[i + 1];
                uint16_t i2 = idx[i + 2];
                if (i0 == i1 || i1 == i2 || i0 == i2) continue;
                float x0, y0, z0, x1, y1, z1, x2, y2, z2;
                uint16_t cf0, cf1, cf2;
                if (!read_vtx_xyz(vbase, i0, &x0, &y0, &z0, &cf0)) break;
                read_vtx_xyz(vbase, i1, &x1, &y1, &z1, &cf1);
                read_vtx_xyz(vbase, i2, &x2, &y2, &z2, &cf2);
                if (s_vtx_type == 0x140C && (z0 <= 0.005f || z1 <= 0.005f || z2 <= 0.005f)) {
                    near_tris++;
                }
                float d01 = fabsf(x1 - x0) + fabsf(y1 - y0) + fabsf(z1 - z0);
                float d12 = fabsf(x2 - x1) + fabsf(y2 - y1) + fabsf(z2 - z1);
                float d20 = fabsf(x0 - x2) + fabsf(y0 - y2) + fabsf(z0 - z2);
                float e = d01 > d12 ? d01 : d12;
                if (d20 > e) e = d20;
                sum_edge += e;
                num_tris++;
                if (e > max_edge) {
                    max_edge = e;
                    max_t = i;
                    max_i0 = i0;
                    max_i1 = i1;
                    max_i2 = i2;
                }
            }
            float avg_edge = num_tris > 0 ? (sum_edge / (float)num_tris) : 0.0f;
            float thresh = (s_vtx_type == 0x140C) ? 0.8f : 200.0f;
            int periodic = 0;
            int anomalous = (num_tris >= 5 && max_edge > avg_edge * 3.5f && max_edge > thresh && s_anomaly_logs < 25);
            static int s_ok121_logs = 0;
            int ok121 = (!anomalous && num_tris >= 20 && s_ok121_logs < 3);
            if (periodic || anomalous || ok121) {
                if (ok121) s_ok121_logs++;
                float x0 = 0, y0 = 0, z0 = 0, x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
                uint16_t cf0 = 0, cf1 = 0, cf2 = 0;
                read_vtx_xyz(vbase, max_i0, &x0, &y0, &z0, &cf0);
                read_vtx_xyz(vbase, max_i1, &x1, &y1, &z1, &cf1);
                read_vtx_xyz(vbase, max_i2, &x2, &y2, &z2, &cf2);
                l_info("[3D%s] #%u f=%u d=%u m=%u cnt=%d tris=%d near=%d idx=%u..%u vt=0x%x/%d vptr=%p dlr=%p avg=%.2f max=%.2f @t=%d",
                       anomalous ? "-ANOM" : (ok121 ? "-OK121" : ""),
                       s_3d_total, s_gl_frame, s_gl_draw3d_idx, mode, count, num_tris, near_tris, min_idx, max_idx,
                       s_vtx_type, s_vtx_stride, s_vtx_ptr, draw_lr,
                       avg_edge, max_edge, max_t);
                l_info("  [max-tri] (%u,%u,%u) v0=(%.2f,%.2f,%.2f:c%02x) v1=(%.2f,%.2f,%.2f:c%02x) v2=(%.2f,%.2f,%.2f:c%02x)",
                       max_i0, max_i1, max_i2, x0, y0, z0, cf0, x1, y1, z1, cf1, x2, y2, z2, cf2);
                if (anomalous || ok121) {
                    uint32_t target_buf = (uint32_t)s_vtx_ptr - 4;
                    for (int k = 0; k < 128; k++) {
                        const vs_trace_t *t = &s_vs_ring[(s_vs_ring_idx - 1 - k) & 127];
                        if (t->out_buf == target_buf && (uintptr_t)t->in_verts >= 0x81000000) {
                            const int16_t *iv = t->in_verts;
                            l_info("  [vs-trace] mode=%d lr=%p nv=%d flg0=0x%08x flg1=0x%08x in=%p",
                                   t->mode, t->caller_lr, t->num_verts, t->flags0, t->flags1, iv);
                            if (max_i0 < t->num_verts && max_i1 < t->num_verts && max_i2 < t->num_verts) {
                                l_info("  [in-s16] (%u,%u,%u) s0=(%d,%d,%d) s1=(%d,%d,%d) s2=(%d,%d,%d)",
                                       max_i0, max_i1, max_i2,
                                       iv[max_i0*3+0], iv[max_i0*3+1], iv[max_i0*3+2],
                                       iv[max_i1*3+0], iv[max_i1*3+1], iv[max_i1*3+2],
                                       iv[max_i2*3+0], iv[max_i2*3+1], iv[max_i2*3+2]);
                            }
                            break;
                        }
                    }
                    if ((uintptr_t)indices >= 0x81001000) {
                        const uint8_t *cmd = (const uint8_t *)indices - 0xf0;
                        if (*(const uint32_t *)(cmd + 0x7c) == target_buf) {
                            const int32_t *vm = (const int32_t *)(cmd + 0x1c);
                            const int32_t *mm = (const int32_t *)(cmd + 0x4c);
                            l_info("  [cmd] flg0=0x%08x mode=%u nv=%u vm_t=(%d,%d,%d) mm_t=(%d,%d,%d)",
                                   *(const uint32_t *)(cmd + 0x10), *(const uint32_t *)(cmd + 0x14),
                                   *(const uint32_t *)(cmd + 0x90),
                                   vm[9], vm[10], vm[11], mm[9], mm[10], mm[11]);
                            l_info("  [cmd-vm-r] (%d,%d,%d) (%d,%d,%d) (%d,%d,%d)",
                                   vm[0], vm[1], vm[2], vm[3], vm[4], vm[5], vm[6], vm[7], vm[8]);
                        }
                    }
                }
                if (anomalous) {
                    s_anomaly_logs++;
                    int logged_bad = 0;
                    for (int i = 0; i < limit && logged_bad < 6; i += step) {
                        uint16_t i0 = (mode == GL_TRIANGLE_FAN) ? idx[0] : idx[i];
                        uint16_t i1 = idx[i + 1];
                        uint16_t i2 = idx[i + 2];
                        if (i0 == i1 || i1 == i2 || i0 == i2 || i == max_t) continue;
                        float ax0, ay0, az0, ax1, ay1, az1, ax2, ay2, az2;
                        uint16_t acf0, acf1, acf2;
                        read_vtx_xyz(vbase, i0, &ax0, &ay0, &az0, &acf0);
                        read_vtx_xyz(vbase, i1, &ax1, &ay1, &az1, &acf1);
                        read_vtx_xyz(vbase, i2, &ax2, &ay2, &az2, &acf2);
                        float d01 = fabsf(ax1 - ax0) + fabsf(ay1 - ay0) + fabsf(az1 - az0);
                        float d12 = fabsf(ax2 - ax1) + fabsf(ay2 - ay1) + fabsf(az2 - az1);
                        float d20 = fabsf(ax0 - ax2) + fabsf(ay0 - ay2) + fabsf(az0 - az2);
                        float e = d01 > d12 ? d01 : d12;
                        if (d20 > e) e = d20;
                        if (e > avg_edge * 3.5f && e > thresh) {
                            l_info("  [bad-tri] t=%d (%u,%u,%u) e=%.2f v0=(%.2f,%.2f,%.2f:c%02x) v1=(%.2f,%.2f,%.2f:c%02x) v2=(%.2f,%.2f,%.2f:c%02x)",
                                   i, i0, i1, i2, e, ax0, ay0, az0, acf0, ax1, ay1, az1, acf1, ax2, ay2, az2, acf2);
                            logged_bad++;
                        }
                    }
                }
            }
        }
        s_gl_draw3d_idx++;
    }
    glDrawElements(mode, count, type, indices);
}

static EGLBoolean wrap_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    s_gl_frame++;
    s_gl_draw3d_idx = 0;
    return eglSwapBuffers(dpy, surface);
}

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
    { "eglSwapBuffers", (void *)wrap_eglSwapBuffers },
    { "eglSwapInterval", (void *)eglSwapInterval },
    { "eglTerminate", (void *)eglTerminate },
    { "glActiveTexture", (void *)glActiveTexture },
    { "glAlphaFunc", (void *)glAlphaFunc },
    { "glAlphaFuncx", (void *)glAlphaFuncx },
    { "glBindBuffer", (void *)wrap_glBindBuffer },
    { "glBindFramebufferOES", (void *)glBindFramebuffer },
    { "glBindRenderbufferOES", (void *)glBindRenderbuffer },
    { "glBindTexture", (void *)glBindTexture },
    { "glBlendFunc", (void *)glBlendFunc },
    { "glBufferData", (void *)glBufferData },
    { "glBufferSubData", (void *)glBufferSubData },
    { "glCheckFramebufferStatusOES", (void *)glCheckFramebufferStatus },
    { "glClear", (void *)wrap_glClear },
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
    { "glDrawElements", (void *)wrap_glDrawElements },
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
    { "glVertexPointer", (void *)wrap_glVertexPointer },
    { "glViewport", (void *)glViewport },
};

void *redeg_lookup_gl(const char *name) {
    for (unsigned i = 0; i < sizeof(s_gl_syms) / sizeof(s_gl_syms[0]); i++)
        if (s_gl_syms[i].name[0] == name[0] && strcmp(s_gl_syms[i].name, name) == 0)
            return s_gl_syms[i].addr;
    return 0;
}
