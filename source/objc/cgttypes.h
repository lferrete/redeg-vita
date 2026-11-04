#ifndef REDEG_CGTYPES_H
#define REDEG_CGTYPES_H

typedef struct { float x, y; } redeg_cgpoint_t;
typedef struct { float w, h; } redeg_cgsize_t;
typedef struct { redeg_cgpoint_t origin; redeg_cgsize_t size; } redeg_cgrect_t;
typedef struct { float a, b, c, d, tx, ty; } redeg_cgaffine_t;
typedef struct { double latitude, longitude; } redeg_clcoord_t;

#endif
