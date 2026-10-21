#ifndef PR_GBI_H
#define PR_GBI_H

/**
 * @file
 * The graphics binary interface types the gu routines fill in for the RSP microcode.
 */

#include <PR/ultratypes.h>

/** A 4 by 4 matrix in s15.16 fixed point, with the integer halves before the fractions. */
typedef union {
    s32 m[4][4];
    long long force_structure_alignment;
} Mtx;

/** A directional light with its colour, a copy of the colour and its direction. */
typedef struct {
    u8 col[3];
    s8 pad1;
    u8 colc[3];
    s8 pad2;
    s8 dir[3];
    s8 pad3;
} Light_t;

typedef union {
    Light_t l;
    long long force_structure_alignment[2];
} Light;

/** The two directions the RSP uses to generate texture coordinates for reflection mapping. */
typedef struct {
    Light l[2];
} LookAt;

/** The texture coordinates of the two specular highlights. */
typedef struct {
    s32 x1, y1, x2, y2;
} Hilite_t;

typedef union {
    Hilite_t h;
    s32 force_structure_alignment[4];
} Hilite;

#endif
