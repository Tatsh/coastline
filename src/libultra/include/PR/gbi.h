#ifndef PR_GBI_H
#define PR_GBI_H

/**
 * @file
 * The graphics binary interface types the gu routines fill in for the RSP microcode.
 */

#include <PR/ultratypes.h>

/** A 4 by 4 matrix in s15.16 fixed point, with the integer halves before the fractions. */
typedef union {
    s32 m[4][4]; /*!< The integer halves of the 16 elements in its first 8 words, then the
                      fractions, two elements to a word. */
    long long force_structure_alignment; /*!< Aligns the matrix to 8 bytes for the RSP. */
} Mtx;

/** A directional light with its colour, a copy of the colour and its direction. */
typedef struct {
    u8 col[3];  /*!< The red, green, and blue of the light. */
    s8 pad1;    /*!< Padding after the colour. */
    u8 colc[3]; /*!< A copy of the colour for the RSP. */
    s8 pad2;    /*!< Padding after the copy of the colour. */
    s8 dir[3];  /*!< The unit direction toward the light, each component times 127. */
    s8 pad3;    /*!< Padding after the direction. */
} Light_t;

/** A directional light, aligned for the RSP. */
typedef union {
    Light_t l;                              /*!< The light. */
    long long force_structure_alignment[2]; /*!< Aligns the light to 8 bytes for the RSP. */
} Light;

/** The two directions the RSP uses to generate texture coordinates for reflection mapping. */
typedef struct {
    Light l[2]; /*!< The directions of the s and t texture axes, in the form of two lights. */
} LookAt;

/** The texture coordinates of the two specular highlights. */
typedef struct {
    s32 x1, y1, x2, y2; /*!< The s and t of the first highlight, then of the second, in 10.2
                             fixed point texels. */
} Hilite_t;

/** The coordinates of two specular highlights, aligned for the RSP. */
typedef union {
    Hilite_t h;                       /*!< The coordinates of the highlights. */
    s32 force_structure_alignment[4]; /*!< Aligns the highlights to 4 bytes. */
} Hilite;

/** A vertex with a texture coordinate and a color. */
typedef struct {
    s16 ob[3]; /*!< The x, y, and z of the vertex in model coordinates. */
    u16 flag;  /*!< Unused by the microcode. */
    s16 tc[2]; /*!< The s and t texture coordinates, in s10.5 fixed point. */
    u8 cn[4];  /*!< The red, green, blue, and alpha of the vertex. */
} Vtx_t;

/** A vertex with a texture coordinate and a normal, for lighting. */
typedef struct {
    s16 ob[3]; /*!< The x, y, and z of the vertex in model coordinates. */
    u16 flag;  /*!< Unused by the microcode. */
    s16 tc[2]; /*!< The s and t texture coordinates, in s10.5 fixed point. */
    s8 n[3];   /*!< The unit normal, each component times 127. */
    u8 a;      /*!< The alpha of the vertex. */
} Vtx_tn;

/** A vertex for the RSP, with a colour or with a normal. */
typedef union {
    Vtx_t v;                             /*!< The vertex with a colour. */
    Vtx_tn n;                            /*!< The vertex with a normal. */
    long long force_structure_alignment; /*!< Aligns the vertex to 8 bytes for the RSP. */
} Vtx;

/** The two words of a display list command. */
typedef struct {
    u32 w0; /*!< The first word, with the opcode in its top byte. */
    u32 w1; /*!< The second word, often a segmented address or an operand. */
} Gwords;

/** A display list command for the Fast3D microcode. */
typedef union {
    Gwords words;                        /*!< The two words of the command. */
    long long force_structure_alignment; /*!< Aligns the command to 8 bytes for the RSP. */
} Gfx;

#define _SHIFTL(v, s, siz) (((u32)(v) & ((0x01 << (siz)) - 1)) << (s))

/* The command opcodes of the F3DEX microcode, which keeps those of Fast3D. */
#define G_MTX 0x01               /*!< Load or multiply a matrix. */
#define G_VTX 0x04               /*!< Load vertices. */
#define G_DL 0x06                /*!< Call or branch to a display list. */
#define G_TRI2 0xB1              /*!< Draw two triangles. */
#define G_ENDDL 0xB8             /*!< End a display list. */
#define G_TRI1 0xBF              /*!< Draw one triangle. */
#define G_LOADTLUT 0xF0          /*!< Load a palette into texture memory. */
#define G_TEXTURE 0xBB           /*!< Set the texture scale and tile. */
#define G_POPMTX 0xBD            /*!< Pop the modelview matrix. */
#define G_SETGEOMETRYMODE 0xB7   /*!< Set bits of the geometry mode. */
#define G_CLEARGEOMETRYMODE 0xB6 /*!< Clear bits of the geometry mode. */
#define G_MOVEWORD 0xBC          /*!< Write a word of the microcode state. */
#define G_SETOTHERMODE_L 0xB9    /*!< Set bits of the low other mode word. */
#define G_SETOTHERMODE_H 0xBA    /*!< Set bits of the high other mode word. */
#define G_RDPPIPESYNC 0xE7       /*!< Wait for the RDP pipeline to drain. */
#define G_RDPTILESYNC 0xE8       /*!< Wait for the tile descriptors to settle. */
#define G_SETTILE 0xF5           /*!< Set a tile descriptor. */
#define G_SETENVCOLOR 0xFB       /*!< Set the environment colour. */
#define G_SETFOGCOLOR 0xF8       /*!< Set the fog colour. */
#define G_SETBLENDCOLOR 0xF9     /*!< Set the blend colour. */
#define G_SETPRIMDEPTH 0xEE      /*!< Set the primitive depth. */
#define G_SETCOMBINE 0xFC        /*!< Set the colour combiner. */
#define G_RDPHALF_2 0xB3         /*!< The second extra word of a long command. */
#define G_RDPHALF_1 0xB4         /*!< The first extra word of a long command. */
#define G_TEXRECT 0xE4           /*!< Draw a textured rectangle. */
#define G_RDPLOADSYNC 0xE6       /*!< Wait for a texture load to finish. */
#define G_SETTILESIZE 0xF2       /*!< Set the extent of a tile. */
#define G_LOADBLOCK 0xF3         /*!< Load a run of texels into texture memory. */
#define G_SETPRIMCOLOR 0xFA      /*!< Set the primitive colour. */
#define G_SETTIMG 0xFD           /*!< Set the texture image. */
#define G_LOADTILE 0xF4          /*!< Load a rectangle of texels into texture memory. */
#define G_FILLRECT 0xF6          /*!< Fill a rectangle. */
#define G_SETFILLCOLOR 0xF7      /*!< Set the fill colour. */
#define G_SETSCISSOR 0xED        /*!< Set the scissor box. */
#define G_SETCIMG 0xFF           /*!< Set the colour image. */
#define G_SETZIMG 0xFE           /*!< Set the depth image. */
#define G_RDPFULLSYNC 0xE9       /*!< Wait for the RDP to finish and raise its interrupt. */

/* The parameters of gSPMatrix(). */
#define G_MTX_MODELVIEW 0x00  /*!< The modelview matrix. */
#define G_MTX_PROJECTION 0x01 /*!< The projection matrix. */
#define G_MTX_MUL 0x00        /*!< Multiply the current matrix. */
#define G_MTX_LOAD 0x02       /*!< Replace the current matrix. */
#define G_MTX_NOPUSH 0x00     /*!< Do not push the current matrix first. */
#define G_MTX_PUSH 0x04       /*!< Push the current matrix first. */

/* The geometry modes. */
#define G_ZBUFFER 0x00000001            /*!< Compute depth. */
#define G_SHADE 0x00000004              /*!< Compute the shade colour. */
#define G_SHADING_SMOOTH 0x00000200     /*!< Interpolate the shade across a triangle. */
#define G_CULL_FRONT 0x00001000         /*!< Discard triangles facing the camera. */
#define G_CULL_BACK 0x00002000          /*!< Discard triangles facing away. */
#define G_CULL_BOTH 0x00003000          /*!< Discard every triangle. */
#define G_FOG 0x00010000                /*!< Compute fog. */
#define G_LIGHTING 0x00020000           /*!< Light vertices from their normals. */
#define G_TEXTURE_GEN 0x00040000        /*!< Generate texture coordinates from normals. */
#define G_TEXTURE_GEN_LINEAR 0x00080000 /*!< Generate them linearly. */
#define G_LOD 0x00100000                /*!< Compute the level of detail. */
#define G_CLIPPING 0x00800000           /*!< Clip triangles to the frustum. */

/* The fields of the other modes, by their lowest bit. */
#define G_MDSFT_ALPHACOMPARE 0 /*!< The alpha compare. */
#define G_MDSFT_ZSRCSEL 2      /*!< The source of the depth. */
#define G_MDSFT_RENDERMODE 3   /*!< The render mode. */
#define G_MDSFT_ALPHADITHER 4  /*!< The alpha dither. */
#define G_MDSFT_RGBDITHER 6    /*!< The colour dither. */
#define G_MDSFT_COMBKEY 8      /*!< The chroma key. */
#define G_MDSFT_TEXTCONV 9     /*!< The texture conversion. */
#define G_MDSFT_TEXTFILT 12    /*!< The texture filter. */
#define G_MDSFT_TEXTLUT 14     /*!< The texture palette. */
#define G_MDSFT_TEXTLOD 16     /*!< The texture level of detail. */
#define G_MDSFT_TEXTDETAIL 17  /*!< The texture detail. */
#define G_MDSFT_TEXTPERSP 19   /*!< The texture perspective correction. */
#define G_MDSFT_CYCLETYPE 20   /*!< The cycle type. */

/* Values of the cycle type, dither, alpha compare, and depth source. */
#define G_CYC_1CYCLE (0 << G_MDSFT_CYCLETYPE)      /*!< One cycle per pixel. */
#define G_CYC_FILL (3 << G_MDSFT_CYCLETYPE)        /*!< Fill with the fill colour. */
#define G_CYC_2CYCLE (1 << G_MDSFT_CYCLETYPE)      /*!< Two cycles per pixel. */
#define G_CD_NOISE (2 << G_MDSFT_RGBDITHER)        /*!< Dither the colour with noise. */
#define G_CD_DISABLE (3 << G_MDSFT_RGBDITHER)      /*!< Do not dither the colour. */
#define G_AC_THRESHOLD (1 << G_MDSFT_ALPHACOMPARE) /*!< Compare alpha with the blend alpha. */
#define G_AC_DITHER (3 << G_MDSFT_ALPHACOMPARE)    /*!< Compare alpha with a random value. */
#define G_ZS_PIXEL (0 << G_MDSFT_ZSRCSEL)          /*!< Take the depth of each pixel. */
#define G_ZS_PRIM (1 << G_MDSFT_ZSRCSEL)           /*!< Take the primitive depth. */

/* The scissor mode and the values of the texture fields. */
#define G_SC_NON_INTERLACE 0                    /*!< Draw every line. */
#define G_TL_TILE (0 << G_MDSFT_TEXTLOD)        /*!< Take the tile of the command. */
#define G_TL_LOD (1 << G_MDSFT_TEXTLOD)         /*!< Pick the tile by level of detail. */
#define G_TD_CLAMP (0 << G_MDSFT_TEXTDETAIL)    /*!< Clamp the level of detail. */
#define G_TC_FILT (6 << G_MDSFT_TEXTCONV)       /*!< Filter without colour conversion. */
#define G_CK_NONE (0 << G_MDSFT_COMBKEY)        /*!< No chroma key. */
#define G_TT_NONE (0 << G_MDSFT_TEXTLUT)        /*!< No palette. */
#define G_TP_NONE (0 << G_MDSFT_TEXTPERSP)      /*!< No perspective correction. */
#define G_TP_PERSP (1 << G_MDSFT_TEXTPERSP)     /*!< Perspective correction. */
#define G_TF_POINT (0 << G_MDSFT_TEXTFILT)      /*!< Point sampling. */
#define G_TF_BILERP (2 << G_MDSFT_TEXTFILT)     /*!< Bilinear filtering. */
#define G_CD_BAYER (1 << G_MDSFT_RGBDITHER)     /*!< Dither the colour with a Bayer matrix. */
#define G_AD_PATTERN (0 << G_MDSFT_ALPHADITHER) /*!< Dither alpha with a pattern. */
#define G_TT_RGBA16 (2 << G_MDSFT_TEXTLUT)      /*!< A palette of 16-bit RGBA colours. */

/* The coverage, depth and blending bits of a render mode. */
#define AA_EN 0x0008         /*!< Antialias. */
#define Z_CMP 0x0010         /*!< Compare depth. */
#define Z_UPD 0x0020         /*!< Write depth. */
#define IM_RD 0x0040         /*!< Read the colour image. */
#define CLR_ON_CVG 0x0080    /*!< Write colour only when coverage overflows. */
#define CVG_DST_CLAMP 0x0000 /*!< Clamp the coverage. */
#define CVG_DST_WRAP 0x0100  /*!< Wrap the coverage. */
#define CVG_DST_FULL 0x0200  /*!< Force full coverage. */
#define CVG_DST_SAVE 0x0300  /*!< Preserve the stored coverage. */
#define ZMODE_OPA 0x0000     /*!< Opaque depth. */
#define ZMODE_XLU 0x0800     /*!< Translucent depth. */
#define ZMODE_DEC 0x0C00     /*!< Decal depth. */
#define CVG_X_ALPHA 0x1000   /*!< Multiply coverage by alpha. */
#define ALPHA_CVG_SEL 0x2000 /*!< Take alpha from coverage. */
#define FORCE_BL 0x4000      /*!< Blend every pixel. */

/* The inputs of the blender. */
#define G_BL_CLR_IN 0  /*!< The colour of the pixel. */
#define G_BL_CLR_MEM 1 /*!< The colour in memory. */
#define G_BL_A_IN 0    /*!< The alpha of the pixel. */
#define G_BL_A_MEM 1   /*!< The alpha in memory. */
#define G_BL_A_FOG 1   /*!< The alpha of the fog colour. */
#define G_BL_CLR_FOG 3 /*!< The fog colour. */
#define G_BL_A_SHADE 2 /*!< The shade alpha. */
#define G_BL_1MA 0     /*!< One minus the first alpha. */
#define G_BL_1 2       /*!< One. */
#define G_BL_0 3       /*!< Zero. */

/** The blender equation of the first cycle of a render mode. */
#define GBL_c1(m1a, m1b, m2a, m2b)                                                                 \
    ((u32)(m1a) << 30 | (u32)(m1b) << 26 | (u32)(m2a) << 22 | (u32)(m2b) << 18)

/** The blender equation of the second cycle of a render mode. */
#define GBL_c2(m1a, m1b, m2a, m2b)                                                                 \
    ((u32)(m1a) << 28 | (u32)(m1b) << 24 | (u32)(m2a) << 20 | (u32)(m2b) << 16)

/** The opaque surface without antialiasing or depth, for the first cycle. */
#define G_RM_OPA_SURF                                                                              \
    (CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1))
/** The opaque surface without antialiasing or depth, for the second cycle. */
#define G_RM_OPA_SURF2                                                                             \
    (CVG_DST_CLAMP | FORCE_BL | ZMODE_OPA | GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1))

/** The translucent surface without antialiasing, for the first cycle. */
#define G_RM_XLU_SURF                                                                              \
    (IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA |                                                 \
     GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA))
/** The translucent surface without antialiasing, for the second cycle. */
#define G_RM_XLU_SURF2                                                                             \
    (IM_RD | CVG_DST_FULL | FORCE_BL | ZMODE_OPA |                                                 \
     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA))

/* The inputs of the colour combiner, named for G_CCMUX_ and G_ACMUX_ by gDPSetCombineLERP(). */
#define G_CCMUX_COMBINED 0         /*!< The output of the first cycle. */
#define G_CCMUX_TEXEL0 1           /*!< The colour of texel 0. */
#define G_CCMUX_TEXEL1 2           /*!< The colour of texel 1. */
#define G_CCMUX_PRIMITIVE 3        /*!< The primitive colour. */
#define G_CCMUX_SHADE 4            /*!< The shade colour. */
#define G_CCMUX_ENVIRONMENT 5      /*!< The environment colour. */
#define G_CCMUX_TEXEL0_ALPHA 8     /*!< The alpha of texel 0. */
#define G_CCMUX_PRIMITIVE_ALPHA 10 /*!< The primitive alpha. */
#define G_CCMUX_SHADE_ALPHA 11     /*!< The shade alpha. */
#define G_CCMUX_ENV_ALPHA 12       /*!< The environment alpha. */
#define G_CCMUX_LOD_FRACTION 13    /*!< The fraction of the level of detail. */
#define G_CCMUX_1 6                /*!< One. */
#define G_CCMUX_0 31               /*!< Zero. */

#define G_ACMUX_COMBINED 0     /*!< The alpha output of the first cycle. */
#define G_ACMUX_LOD_FRACTION 0 /*!< The fraction of the level of detail. */
#define G_ACMUX_TEXEL0 1       /*!< The alpha of texel 0. */
#define G_ACMUX_TEXEL1 2       /*!< The alpha of texel 1. */
#define G_ACMUX_PRIMITIVE 3    /*!< The primitive alpha. */
#define G_ACMUX_SHADE 4        /*!< The shade alpha. */
#define G_ACMUX_ENVIRONMENT 5  /*!< The environment alpha. */
#define G_ACMUX_1 6            /*!< One. */
#define G_ACMUX_0 7            /*!< Zero. */

/* The texture formats, sizes and tiles. */
#define G_IM_FMT_RGBA 0 /*!< RGBA texels. */
#define G_IM_FMT_CI 2   /*!< Palette indices. */
#define G_IM_FMT_IA 3   /*!< Intensity and alpha. */
#define G_IM_FMT_I 4    /*!< Intensity. */
#define G_IM_SIZ_4b 0   /*!< Texels of 4 bits. */
#define G_IM_SIZ_8b 1   /*!< Texels of 8 bits. */
#define G_IM_SIZ_16b 2  /*!< Texels of 16 bits. */

/* A load block moves texels of 8 and 16 bits as 16-bit texels. These are the size it loads them
 * as, the rounding and the shift of the count, the bytes of a texel, and the bytes of a texel in
 * a row of the texture memory. */
#define G_IM_SIZ_8b_LOAD_BLOCK G_IM_SIZ_16b  /*!< 8-bit texels load as 16-bit texels. */
#define G_IM_SIZ_8b_INCR 1                   /*!< The rounding of an 8-bit texel count. */
#define G_IM_SIZ_8b_SHIFT 1                  /*!< The shift of an 8-bit texel count. */
#define G_IM_SIZ_8b_BYTES 1                  /*!< The bytes of an 8-bit texel. */
#define G_IM_SIZ_8b_LINE_BYTES 1             /*!< The bytes of an 8-bit texel in a row. */
#define G_IM_SIZ_16b_LOAD_BLOCK G_IM_SIZ_16b /*!< 16-bit texels load unchanged. */
#define G_IM_SIZ_16b_INCR 0                  /*!< The rounding of a 16-bit texel count. */
#define G_IM_SIZ_16b_SHIFT 0                 /*!< The shift of a 16-bit texel count. */
#define G_IM_SIZ_16b_BYTES 2                 /*!< The bytes of a 16-bit texel. */
#define G_IM_SIZ_16b_LINE_BYTES 2            /*!< The bytes of a 16-bit texel in a row. */

/* Tiles, texture wrap modes, and switches. */
#define G_TX_RENDERTILE 0 /*!< The tile drawing uses. */
#define G_TX_LOADTILE 7   /*!< The tile loads use. */
#define G_TX_WRAP 0       /*!< Repeat the texture. */
#define G_TX_MIRROR 1     /*!< Mirror the texture. */
#define G_TX_CLAMP 2      /*!< Clamp to the edge texel. */
#define G_TX_NOLOD 0      /*!< No level of detail. */
#define G_ON 1            /*!< On. */
#define G_OFF 0           /*!< Off. */

/* The fixed-point fractions of texture coordinates and of the load stride. */
#define G_TEXTURE_IMAGE_FRAC 2 /*!< The fraction bits of an image coordinate. */
#define G_TX_DXT_FRAC 11       /*!< The fraction bits of the row step of a load block. */

/** The most texels one load block takes, less one. */
#define G_TX_LDBLK_MAX_TXL 2047

/** The 64-bit words of a row of a 4-bit texture, at least one. */
#define TXL2WORDS_4b(txls) ((txls) / 16 > 1 ? (txls) / 16 : 1)

/** The step of the row counter of a load block for a 4-bit texture of a width. */
#define CALC_DXT_4b(width) (((1 << G_TX_DXT_FRAC) + TXL2WORDS_4b(width) - 1) / TXL2WORDS_4b(width))

/** The 64-bit words of a row of a texture with texels of b_txl bytes, at least one. */
#define TXL2WORDS(txls, b_txl) ((txls) * (b_txl) / 8 > 1 ? (txls) * (b_txl) / 8 : 1)

/** The step of the row counter of a load block for a texture with texels of b_txl bytes. */
#define CALC_DXT(width, b_txl)                                                                     \
    (((1 << G_TX_DXT_FRAC) + TXL2WORDS(width, b_txl) - 1) / TXL2WORDS(width, b_txl))

/* The words of the clipping ratio. */
#define G_MW_CLIP 0x04      /*!< The index of the clipping ratio. */
#define G_MWO_CLIP_RNX 0x04 /*!< The offset of the negative x ratio. */
#define G_MWO_CLIP_RNY 0x0C /*!< The offset of the negative y ratio. */
#define G_MWO_CLIP_RPX 0x14 /*!< The offset of the positive x ratio. */
#define G_MWO_CLIP_RPY 0x1C /*!< The offset of the positive y ratio. */
#define FRUSTRATIO_1 1      /*!< A clipping box the size of the screen. */
#define FRUSTRATIO_6 6      /*!< A clipping box six times the size of the screen. */

/* The lights of the F3DEX microcode. */
#define G_MOVEMEM 0x03      /*!< Load a block of the microcode state. */
#define G_MV_L0 0x86        /*!< The block of light 0. */
#define G_MW_NUMLIGHT 0x02  /*!< The index of the light count. */
#define G_MWO_NUMLIGHT 0x00 /*!< The offset of the light count. */
#define NUMLIGHTS_1 1       /*!< One directional light. */
#define NUMLIGHTS_4 4       /*!< Four directional lights. */
#define LIGHT_1 1           /*!< The first light. */
#define LIGHT_2 2           /*!< The second light. */

/* The other memory and words the F3DEX microcode loads. */
#define G_MV_LOOKATY 0x82   /*!< The block of the y look-at vector. */
#define G_MV_LOOKATX 0x84   /*!< The block of the x look-at vector. */
#define G_MV_MATRIX_1 0x9E  /*!< The block of the first quarter of a matrix. */
#define G_MV_MATRIX_2 0x98  /*!< The block of the second quarter of a matrix. */
#define G_MV_MATRIX_3 0x9A  /*!< The block of the third quarter of a matrix. */
#define G_MV_MATRIX_4 0x9C  /*!< The block of the fourth quarter of a matrix. */
#define G_MW_SEGMENT 0x06   /*!< The index of the segment table. */
#define G_MW_LIGHTCOL 0x0A  /*!< The index of the light colours. */
#define G_MW_PERSPNORM 0x0E /*!< The index of the perspective normalisation. */
#define G_MV_VIEWPORT 0x80  /*!< The block of the viewport. */
#define G_MW_FOG 0x08       /*!< The index of the fog parameters. */
#define G_MWO_FOG 0x00      /*!< The offset of the fog parameters. */

/** The scale and translation of a viewport, in 10.2 fixed point, each with the depth third. */
typedef struct {
    s16 vscale[4]; /*!< Half the width, height, and depth range, then 0; a negative x mirrors. */
    s16 vtrans[4]; /*!< The x and y of the centre and the middle of the depth range, then 0. */
} Vp_t;

/** A viewport, aligned for the RSP. */
typedef union {
    Vp_t vp;                                /*!< The viewport. */
    long long force_structure_alignment[2]; /*!< Aligns the viewport to 8 bytes for the RSP. */
} Vp;

/** The word gSPNumLights() sends for a number of directional lights. */
#define NUML(n) (((n) + 1) * 32 + 0x80000000)

/** An ambient light: its color, twice. */
typedef struct {
    u8 col[3];  /*!< The red, green, and blue of the light. */
    char pad1;  /*!< Padding after the colour. */
    u8 colc[3]; /*!< A copy of the colour for the RSP. */
    char pad2;  /*!< Padding after the copy of the colour. */
} Ambient_t;

/** An ambient light, aligned for the RSP. */
typedef union {
    Ambient_t l;                            /*!< The light. */
    long long force_structure_alignment[1]; /*!< Aligns the light to 8 bytes for the RSP. */
} Ambient;

/** An ambient light and one directional light. */
typedef struct {
    Ambient a;  /*!< The ambient light. */
    Light l[1]; /*!< The directional light. */
} Lights1;

/** Write a command of two words at pkt, which is evaluated once. */
#define gDisplayListWords(pkt, c0, c1)                                                             \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = (c0);                                                                       \
        _g->words.w1 = (c1);                                                                       \
    }

/** Write command cmd with no parameters. */
#define gDPNoParam(pkt, cmd) gDisplayListWords(pkt, _SHIFTL(cmd, 24, 8), 0)
/** Write command cmd with data as its second word. */
#define gImmp1(pkt, cmd, data) gDisplayListWords(pkt, _SHIFTL(cmd, 24, 8), (u32)(data))
/** Write data to the word at offset in the microcode state of index. */
#define gMoveWd(pkt, index, offset, data)                                                          \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_MOVEWORD, 24, 8) | _SHIFTL(offset, 8, 16) | _SHIFTL(index, 0, 8),  \
                      (u32)(data))
/** Replace len bits of an other mode word from bit sft with data. */
#define gSPSetOtherMode(pkt, cmd, sft, len, data)                                                  \
    gDisplayListWords(                                                                             \
        pkt, _SHIFTL(cmd, 24, 8) | _SHIFTL(sft, 8, 8) | _SHIFTL(len, 0, 8), (u32)(data))

/** Load or multiply matrix m by the G_MTX_* flags of p. */
#define gSPMatrix(pkt, m, p)                                                                       \
    gDisplayListWords(                                                                             \
        pkt, _SHIFTL(G_MTX, 24, 8) | _SHIFTL(p, 16, 8) | _SHIFTL(sizeof(Mtx), 0, 16), (u32)(m))
/** Pop the modelview matrix; n is the matrix stack. */
#define gSPPopMatrix(pkt, n) gImmp1(pkt, G_POPMTX, n)
/** Call display list dl. */
#define gSPDisplayList(pkt, dl) gDisplayListWords(pkt, _SHIFTL(G_DL, 24, 8), (u32)(dl))
/** End the display list. */
#define gSPEndDisplayList(pkt) gDPNoParam(pkt, G_ENDDL)
/** Load n vertices into the vertex buffer of the F3DEX microcode from index v0. */
#define gSPVertex(pkt, v, n, v0)                                                                   \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_VTX, 24, 8) | _SHIFTL((v0) * 2, 16, 8) | _SHIFTL(n, 10, 6) |       \
                          _SHIFTL(sizeof(Vtx) * (n) - 1, 0, 10),                                   \
                      (u32)(v))

#define __gsSP1Triangle_w1f(v0, v1, v2, flag)                                                      \
    ((flag) == 0 ? _SHIFTL((v0) * 2, 16, 8) | _SHIFTL((v1) * 2, 8, 8) | _SHIFTL((v2) * 2, 0, 8) :  \
     (flag) == 1 ? _SHIFTL((v1) * 2, 16, 8) | _SHIFTL((v2) * 2, 8, 8) | _SHIFTL((v0) * 2, 0, 8) :  \
                   _SHIFTL((v2) * 2, 16, 8) | _SHIFTL((v0) * 2, 8, 8) | _SHIFTL((v1) * 2, 0, 8))

/** Draw two triangles of the vertex buffer. */
#define gSP2Triangles(pkt, v00, v01, v02, flag0, v10, v11, v12, flag1)                             \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_TRI2, 24, 8) | __gsSP1Triangle_w1f(v00, v01, v02, flag0),          \
                      __gsSP1Triangle_w1f(v10, v11, v12, flag1))

/** Set the geometry mode bits of word. */
#define gSPSetGeometryMode(pkt, word) gImmp1(pkt, G_SETGEOMETRYMODE, word)
/** Clear the geometry mode bits of word. */
#define gSPClearGeometryMode(pkt, word) gImmp1(pkt, G_CLEARGEOMETRYMODE, word)
/** Set the texture scale s and t, the levels of detail, the tile, and whether texturing is on. */
#define gSPTexture(pkt, s, t, level, tile, on)                                                     \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(level, 11, 3) | _SHIFTL(tile, 8, 3) |    \
                          _SHIFTL(on, 0, 8),                                                       \
                      _SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))

/** Set the ratio of the clipping box to the screen with four commands. */
#define gSPClipRatio(pkt, r)                                                                       \
    {                                                                                              \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNX, (u16)(r));                                         \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNY, (u16)(r));                                         \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPX, (u16) - (r));                                      \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPY, (u16) - (r));                                      \
    }

/** Set the number of directional lights. */
#define gSPNumLights(pkt, n) gMoveWd(pkt, G_MW_NUMLIGHT, G_MWO_NUMLIGHT, NUML(n))

/** Set the colour of light n, from 1, as its two copies. */
#define gSPLightColor(pkt, n, col)                                                                 \
    {                                                                                              \
        gMoveWd(pkt, G_MW_LIGHTCOL, ((n) - 1) * 0x20, col);                                        \
        gMoveWd(pkt, G_MW_LIGHTCOL, ((n) - 1) * 0x20 + 4, col);                                    \
    }

/** Set the normalisation of the perspective, which gu's perspective routines return. */
#define gSPPerspNormalize(pkt, s) gMoveWd(pkt, G_MW_PERSPNORM, 0, s)

/** Load a viewport. */
#define gSPViewport(pkt, v)                                                                        \
    gDisplayListWords(                                                                             \
        pkt, _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_VIEWPORT, 16, 8) | sizeof(Vp), (u32)(v))

/** Set the depths the fog starts and ends at, from 0 at the near plane to 1000 at the far. */
#define gSPFogPosition(pkt, min, max)                                                              \
    gMoveWd(pkt,                                                                                   \
            G_MW_FOG,                                                                              \
            G_MWO_FOG,                                                                             \
            (_SHIFTL((128000 / ((max) - (min))), 16, 16) |                                         \
             _SHIFTL(((500 - (min)) * 256 / ((max) - (min))), 0, 16)))

/** Set the base of a segment. */
#define gSPSegment(pkt, segment, base) gMoveWd(pkt, G_MW_SEGMENT, (segment) * 4, base)

/** Load the look-at of the reflections, its x then its y. */
#define gSPLookAt(pkt, la)                                                                         \
    {                                                                                              \
        gDisplayListWords(pkt,                                                                     \
                          _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_LOOKATX, 16, 8) |               \
                              _SHIFTL(sizeof(Light), 0, 16),                                       \
                          (u32)(la));                                                              \
        gDisplayListWords(pkt,                                                                     \
                          _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_LOOKATY, 16, 8) |               \
                              _SHIFTL(sizeof(Light), 0, 16),                                       \
                          (u32)(la) + 16);                                                         \
    }

/** Load a matrix as the product of the model view and the projection, a row pair at a time. */
#define gSPForceMatrix(pkt, m)                                                                     \
    {                                                                                              \
        gDisplayListWords(                                                                         \
            pkt, _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_MATRIX_1, 16, 8) | 16, (u32)(m));        \
        gDisplayListWords(                                                                         \
            pkt, _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_MATRIX_2, 16, 8) | 16, (u32)(m) + 16);   \
        gDisplayListWords(                                                                         \
            pkt, _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_MATRIX_3, 16, 8) | 16, (u32)(m) + 32);   \
        gDisplayListWords(                                                                         \
            pkt, _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(G_MV_MATRIX_4, 16, 8) | 16, (u32)(m) + 48);   \
    }

/** Load a light into light slot n, from 1. The ambient light follows the directional ones. */
#define gSPLight(pkt, l, n)                                                                        \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_MOVEMEM, 24, 8) | _SHIFTL(((n) - 1) * 2 + G_MV_L0, 16, 8) |        \
                          _SHIFTL(sizeof(Light), 0, 16),                                           \
                      (u32)(l))

/** Wait for the RDP pipeline to drain before a mode changes. */
#define gDPPipeSync(pkt) gDPNoParam(pkt, G_RDPPIPESYNC)
/** Wait for the tile descriptors to settle before a tile changes. */
#define gDPTileSync(pkt) gDPNoParam(pkt, G_RDPTILESYNC)
/** Set the G_CYC_* cycle type. */
#define gDPSetCycleType(pkt, type)                                                                 \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
/** Set the G_TF_* texture filter. */
#define gDPSetTextureFilter(pkt, type)                                                             \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2, type)
/** Set the G_TL_* texture level of detail. */
#define gDPSetTextureLOD(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1, type)
/** Set the G_TD_* texture detail. */
#define gDPSetTextureDetail(pkt, type)                                                             \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTDETAIL, 2, type)
/** Set the G_TC_* texture conversion. */
#define gDPSetTextureConvert(pkt, type)                                                            \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTCONV, 3, type)
/** Set the G_CK_* chroma key. */
#define gDPSetCombineKey(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_COMBKEY, 1, type)
/** Set the G_CD_* colour dither. */
#define gDPSetColorDither(pkt, mode)                                                               \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_RGBDITHER, 2, mode)
/** Set the G_AD_* alpha dither. */
#define gDPSetAlphaDither(pkt, mode)                                                               \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 2, mode)
/** Set the G_AC_* alpha compare. */
#define gDPSetAlphaCompare(pkt, type)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_ALPHACOMPARE, 2, type)
/** Set the G_ZS_* depth source. */
#define gDPSetDepthSource(pkt, src) gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_ZSRCSEL, 1, src)
/** Set the blend colour. */
#define gDPSetBlendColor(pkt, r, g, b, a)                                                          \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETBLENDCOLOR, 24, 8),                                             \
                      _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
/** Set the depth of a primitive whose depth source is G_ZS_PRIM. */
#define gDPSetPrimDepth(pkt, z, dz)                                                                \
    gDisplayListWords(pkt, _SHIFTL(G_SETPRIMDEPTH, 24, 8), _SHIFTL(z, 16, 16) | _SHIFTL(dz, 0, 16))

/** Set the scissor rectangle, whose corners are converted to 10.2 fixed point. */
#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry)                                               \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETSCISSOR, 24, 8) | _SHIFTL((s32)((f32)(ulx) * 4.0f), 12, 12) |   \
                          _SHIFTL((s32)((f32)(uly) * 4.0f), 0, 12),                                \
                      _SHIFTL(mode, 24, 2) | _SHIFTL((s32)((f32)(lrx) * 4.0f), 12, 12) |           \
                          _SHIFTL((s32)((f32)(lry) * 4.0f), 0, 12))
/** Set the render mode from the modes of the first and second cycles. */
#define gDPSetRenderMode(pkt, c0, c1)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, (c0) | (c1))
/** Set the environment colour. */
#define gDPSetEnvColor(pkt, r, g, b, a)                                                            \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETENVCOLOR, 24, 8),                                               \
                      _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
/** Set the fog colour. */
#define gDPSetFogColor(pkt, r, g, b, a)                                                            \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETFOGCOLOR, 24, 8),                                               \
                      _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
/** Set the format, texel size, row length, address, palette, and wrapping of a tile. */
#define gDPSetTile(                                                                                \
    pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)              \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) |      \
                          _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9),                               \
                      _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) |       \
                          _SHIFTL(maskt, 14, 4) | _SHIFTL(shiftt, 10, 4) | _SHIFTL(cms, 8, 2) |    \
                          _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4))

/** Set the colour combiner from the a, b, c and d inputs of (a - b) * c + d for each cycle. */
#define gDPSetCombineLERP(                                                                         \
    pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)                   \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(G_CCMUX_##a0, 20, 4) |                \
                          _SHIFTL(G_CCMUX_##c0, 15, 5) | _SHIFTL(G_ACMUX_##Aa0, 12, 3) |           \
                          _SHIFTL(G_ACMUX_##Ac0, 9, 3) | _SHIFTL(G_CCMUX_##a1, 5, 4) |             \
                          _SHIFTL(G_CCMUX_##c1, 0, 5),                                             \
                      _SHIFTL(G_CCMUX_##b0, 28, 4) | _SHIFTL(G_CCMUX_##b1, 24, 4) |                \
                          _SHIFTL(G_ACMUX_##Aa1, 21, 3) | _SHIFTL(G_ACMUX_##Ac1, 18, 3) |          \
                          _SHIFTL(G_CCMUX_##d0, 15, 3) | _SHIFTL(G_ACMUX_##Ab0, 12, 3) |           \
                          _SHIFTL(G_ACMUX_##Ad0, 9, 3) | _SHIFTL(G_CCMUX_##d1, 6, 3) |             \
                          _SHIFTL(G_ACMUX_##Ab1, 3, 3) | _SHIFTL(G_ACMUX_##Ad1, 0, 3))

/** Wait for a texture load to finish. */
#define gDPLoadSync(pkt) gDPNoParam(pkt, G_RDPLOADSYNC)
/** Wait for the RDP to finish the frame and raise its interrupt. */
#define gDPFullSync(pkt) gDPNoParam(pkt, G_RDPFULLSYNC)
/** Set the depth buffer to i. */
#define gDPSetDepthImage(pkt, i) gDisplayListWords(pkt, _SHIFTL(G_SETZIMG, 24, 8), (u32)(i))
/** Set the colour buffer to i, of format f, texel size s, and width w. */
#define gDPSetColorImage(pkt, f, s, w, i)                                                          \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETCIMG, 24, 8) | _SHIFTL(f, 21, 3) | _SHIFTL(s, 19, 2) |          \
                          _SHIFTL((w) - 1, 0, 12),                                                 \
                      (u32)(i))
/** A colour of RGBA 5551 from components of 8 bits and an alpha of 1 bit. */
#define GPACK_RGBA5551(r, g, b, a)                                                                 \
    ((((r) << 8) & 0xF800) | (((g) << 3) & 0x7C0) | (((b) >> 2) & 0x3E) | ((a) & 0x1))

/** A depth and its slope, as the depth buffer stores them. */
#define GPACK_ZDZ(z, dz) ((z) << 2 | (dz))

/** The farthest depth. */
#define G_MAXFIXED 0x3FFF

/** Set the fill colour, a 32-bit word of one or two packed pixels. */
#define gDPSetFillColor(pkt, d) gDisplayListWords(pkt, _SHIFTL(G_SETFILLCOLOR, 24, 8), (u32)(d))
/** Fill a rectangle, its corners inclusive, with the fill colour. */
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)                                                  \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL(lrx, 14, 10) | _SHIFTL(lry, 2, 10),     \
                      _SHIFTL(ulx, 14, 10) | _SHIFTL(uly, 2, 10))
/** Set the G_TT_* texture palette. */
#define gDPSetTextureLUT(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2, type)
/** Set the G_TP_* texture perspective correction. */
#define gDPSetTexturePersp(pkt, type)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1, type)
/** Set the primitive colour with the minimum level of detail m and its fraction l. */
#define gDPSetPrimColor(pkt, m, l, r, g, b, a)                                                     \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8),        \
                      _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
/** Set the texture image to i, of format f, texel size s, and width w. */
#define gDPSetTextureImage(pkt, f, s, w, i)                                                        \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETTIMG, 24, 8) | _SHIFTL(f, 21, 3) | _SHIFTL(s, 19, 2) |          \
                          _SHIFTL((w) - 1, 0, 12),                                                 \
                      (u32)(i))
/** Load texels uls to lrs of the texture image through a tile, with row step dxt. */
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)                                                \
    gDisplayListWords(                                                                             \
        pkt,                                                                                       \
        _SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),                  \
        _SHIFTL(tile, 24, 3) |                                                                     \
            _SHIFTL((lrs) < G_TX_LDBLK_MAX_TXL ? (lrs) : G_TX_LDBLK_MAX_TXL, 12, 12) |             \
            _SHIFTL(dxt, 0, 12))
/** Set the extent of tile t, in 10.2 fixed point. */
#define gDPSetTileSize(pkt, t, uls, ult, lrs, lrt)                                                 \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETTILESIZE, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),  \
                      _SHIFTL(t, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12))
/** Load count + 1 colors of a palette through a tile. */
#define gDPLoadTLUTCmd(pkt, tile, count)                                                           \
    gDisplayListWords(                                                                             \
        pkt, _SHIFTL(G_LOADTLUT, 24, 8), _SHIFTL(tile, 24, 3) | _SHIFTL(count, 14, 10))

/** Load a palette of 256 colors into the upper half of the texture memory. */
#define gDPLoadTLUT_pal256(pkt, dram)                                                              \
    {                                                                                              \
        gDPSetTextureImage(pkt, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram);                             \
        gDPTileSync(pkt);                                                                          \
        gDPSetTile(pkt, 0, 0, 0, 256, G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);                         \
        gDPLoadSync(pkt);                                                                          \
        gDPLoadTLUTCmd(pkt, G_TX_LOADTILE, 255);                                                   \
        gDPPipeSync(pkt);                                                                          \
    }

/** Load a palette of 16 colors into one of the sixteen palettes of the upper texture memory. */
#define gDPLoadTLUT_pal16(pkt, pal, dram)                                                          \
    {                                                                                              \
        gDPSetTextureImage(pkt, G_IM_FMT_RGBA, G_IM_SIZ_16b, 1, dram);                             \
        gDPTileSync(pkt);                                                                          \
        gDPSetTile(pkt, 0, 0, 0, 256 + (((pal) & 0xF) * 16), G_TX_LOADTILE, 0, 0, 0, 0, 0, 0, 0);  \
        gDPLoadSync(pkt);                                                                          \
        gDPLoadTLUTCmd(pkt, G_TX_LOADTILE, 15);                                                    \
        gDPPipeSync(pkt);                                                                          \
    }

/** Place a tile of a size at the first specular highlight. */
#define gDPSetHilite1Tile(pkt, tile, hilite, width, height)                                        \
    gDPSetTileSize(pkt,                                                                            \
                   tile,                                                                           \
                   (hilite)->h.x1 & 0xFFF,                                                         \
                   (hilite)->h.y1 & 0xFFF,                                                         \
                   ((((width) - 1) * 4) + (hilite)->h.x1) & 0xFFF,                                 \
                   ((((height) - 1) * 4) + (hilite)->h.y1) & 0xFFF)

/** Load a rectangle of texels. The coordinates are in 10.2 fixed point. */
#define gDPLoadTile(pkt, t, uls, ult, lrs, lrt)                                                    \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_LOADTILE, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),     \
                      _SHIFTL(t, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12))

/** Load a 4-bit texture into the texture memory and set up the render tile to draw it. */
#define gDPLoadTextureBlock_4b(                                                                    \
    pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)                    \
    {                                                                                              \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);                                       \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   G_IM_SIZ_16b,                                                                   \
                   0,                                                                              \
                   0,                                                                              \
                   G_TX_LOADTILE,                                                                  \
                   0,                                                                              \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPLoadSync(pkt);                                                                          \
        gDPLoadBlock(                                                                              \
            pkt, G_TX_LOADTILE, 0, 0, (((width) * (height) + 3) >> 2) - 1, CALC_DXT_4b(width));    \
        gDPPipeSync(pkt);                                                                          \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   G_IM_SIZ_4b,                                                                    \
                   (((width) >> 1) + 7) >> 3,                                                      \
                   0,                                                                              \
                   G_TX_RENDERTILE,                                                                \
                   pal,                                                                            \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPSetTileSize(pkt,                                                                        \
                       G_TX_RENDERTILE,                                                            \
                       0,                                                                          \
                       0,                                                                          \
                       ((width) - 1) << G_TEXTURE_IMAGE_FRAC,                                      \
                       ((height) - 1) << G_TEXTURE_IMAGE_FRAC);                                    \
    }

/** Load an 8-bit or 16-bit texture into the texture memory and set up the render tile. */
#define gDPLoadTextureBlock(                                                                       \
    pkt, timg, fmt, siz, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)               \
    {                                                                                              \
        gDPSetTextureImage(pkt, fmt, siz##_LOAD_BLOCK, 1, timg);                                   \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   siz##_LOAD_BLOCK,                                                               \
                   0,                                                                              \
                   0,                                                                              \
                   G_TX_LOADTILE,                                                                  \
                   0,                                                                              \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPLoadSync(pkt);                                                                          \
        gDPLoadBlock(pkt,                                                                          \
                     G_TX_LOADTILE,                                                                \
                     0,                                                                            \
                     0,                                                                            \
                     (((width) * (height) + siz##_INCR) >> siz##_SHIFT) - 1,                       \
                     CALC_DXT(width, siz##_BYTES));                                                \
        gDPPipeSync(pkt);                                                                          \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   siz,                                                                            \
                   (((width) * siz##_LINE_BYTES) + 7) >> 3,                                        \
                   0,                                                                              \
                   G_TX_RENDERTILE,                                                                \
                   pal,                                                                            \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPSetTileSize(pkt,                                                                        \
                       G_TX_RENDERTILE,                                                            \
                       0,                                                                          \
                       0,                                                                          \
                       ((width) - 1) << G_TEXTURE_IMAGE_FRAC,                                      \
                       ((height) - 1) << G_TEXTURE_IMAGE_FRAC);                                    \
    }

/** Load a rectangle of a 4-bit texture into the texture memory and set up the render tile. */
#define gDPLoadTextureTile_4b(pkt,                                                                 \
                              timg,                                                                \
                              fmt,                                                                 \
                              width,                                                               \
                              height,                                                              \
                              uls,                                                                 \
                              ult,                                                                 \
                              lrs,                                                                 \
                              lrt,                                                                 \
                              pal,                                                                 \
                              cms,                                                                 \
                              cmt,                                                                 \
                              masks,                                                               \
                              maskt,                                                               \
                              shifts,                                                              \
                              shiftt)                                                              \
    {                                                                                              \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_8b, ((width) >> 1), timg);                           \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   G_IM_SIZ_8b,                                                                    \
                   (((((lrs) - (uls) + 1) >> 1) + 7) >> 3),                                        \
                   0,                                                                              \
                   G_TX_LOADTILE,                                                                  \
                   0,                                                                              \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPLoadSync(pkt);                                                                          \
        gDPLoadTile(pkt,                                                                           \
                    G_TX_LOADTILE,                                                                 \
                    (uls) << (G_TEXTURE_IMAGE_FRAC - 1),                                           \
                    (ult) << (G_TEXTURE_IMAGE_FRAC),                                               \
                    (lrs) << (G_TEXTURE_IMAGE_FRAC - 1),                                           \
                    (lrt) << (G_TEXTURE_IMAGE_FRAC));                                              \
        gDPPipeSync(pkt);                                                                          \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   G_IM_SIZ_4b,                                                                    \
                   (((((lrs) - (uls) + 1) >> 1) + 7) >> 3),                                        \
                   0,                                                                              \
                   G_TX_RENDERTILE,                                                                \
                   pal,                                                                            \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPSetTileSize(pkt,                                                                        \
                       G_TX_RENDERTILE,                                                            \
                       (uls) << G_TEXTURE_IMAGE_FRAC,                                              \
                       (ult) << G_TEXTURE_IMAGE_FRAC,                                              \
                       (lrs) << G_TEXTURE_IMAGE_FRAC,                                              \
                       (lrt) << G_TEXTURE_IMAGE_FRAC);                                             \
    }

/** Draw a textured rectangle in screen coordinates of 10.2 fixed point. */
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)                           \
    {                                                                                              \
        gDisplayListWords(pkt,                                                                     \
                          _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12),    \
                          _SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12));        \
        gImmp1(pkt, G_RDPHALF_1, _SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16));                          \
        gImmp1(pkt, G_RDPHALF_2, _SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16));                    \
    }

#endif
