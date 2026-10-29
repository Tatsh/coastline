#ifndef COASTLINE_MODEL_H
#define COASTLINE_MODEL_H

/**
 * @file
 * The models of the menus and splash screens: loading, animating and drawing them.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "common/ptr32.h"
#include "common/vecmath.h"

#if TGR_BOSS_RALLY
#include "common/gbicommand.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** A part of a model: its display list, how it is drawn, and where. */
typedef struct {
#if TGR_BOSS_RALLY
    TGR_PTR(GbiCommand) displayList;
#else
    TGR_PTR(Gfx) displayList; /*!< The display list that draws the part, or NULL. */
#endif
    u16 flags; /*!< The MODEL_PART_ flags. */
#if TGR_BOSS_RALLY
    u16 wUnknown6; /* +0x06 */
#endif
    Vec3f position; /*!< Where the part is drawn, relative to the model. */
} ModelPart;

/** The bits of the flags of a part that pick its light from g_adwModelShadeColors. */
#define MODEL_PART_LIGHT_RAMP 0x0003

/** The flag of a part drawn from both sides. */
#define MODEL_PART_TWO_SIDED 0x0004

/** The flag of a part drawn after the opaque ones. */
#define MODEL_PART_TRANSLUCENT 0x0008

/** The flag of a part drawn with flat shading. */
#define MODEL_PART_FLAT 0x0080

/** The flag of a part lit by its light ramp. */
#define MODEL_PART_RAMPED 0x0400

/**
 * A key of an animation: its time, then the position of each vertex, then its normal. The normals
 * follow the positions of all the vertices, so they have no field.
 */
typedef struct {
    f32 time;           /*!< The time of the key in the animation, in seconds. */
    s16 positions[][3]; /*!< The position of each vertex, followed by the normals. */
} ModelKeyframe;

/* The flags of an animation. */
#define MODEL_ANIMATION_REPEAT 0x1    /*!< The animation starts again at its end. */
#define MODEL_ANIMATION_BOUNCE 0x2    /*!< The animation turns back at its end. */
#define MODEL_ANIMATION_REVERSING 0x4 /*!< The animation runs backwards. */

/** An animation of vertices of a model, blended between its keys. */
typedef struct {
    s32 vertexCount; /*!< The number of vertices the animation moves. */
#if TGR_BOSS_RALLY
    ModelVertexData vertices;
#else
    TGR_PTR(Vtx) vertices; /*!< The vertices the animation moves. */
#endif
    TGR_PTR(void) unreconstructed8; /*!< Turned into a pointer with the rest, and never read. */
    s32 keyCount;                   /*!< The number of keys. */
    u16 flags;                      /*!< The MODEL_ANIMATION_ flags. */
    u16 startKey;                   /*!< The key a search for the time starts from. */
    f32 startTime;                  /*!< The time the animation starts at, in seconds. */
    f32 endTime;                    /*!< The time the animation ends at, in seconds. */
    f32 time;                       /*!< The current time of the animation, in seconds. */
    TGR_PTR(ModelKeyframe) keys[];  /*!< The keys, in order of time. */
} ModelAnimation;

/** The animations of a model. */
typedef struct {
    s32 count;                         /*!< The number of animations. */
    TGR_PTR(ModelAnimation) entries[]; /*!< The animations. */
} ModelAnimationList;

/**
 * A model loaded from the cartridge, with its display lists and the animations of its parts. Its
 * fields join this structure as they are reconstructed.
 */
typedef struct Model {
#if TGR_BOSS_RALLY
    u16 wUnknown0; /* +0x00 */
    u16 partCount;
#else
    u32 partCount; /*!< The number of parts. */
#endif
    TGR_PTR(ModelAnimationList) animations; /*!< The animations of the vertices of the model. */
    ModelPart parts[];                      /*!< The parts. */
} Model;

/**
 * The ambient light of the menus that draw models.
 *
 * @ghidraAddress USA: 0x80271f88
 * @ghidraAddress EUR: 0x80273f68
 * @ghidraAddress JPN: 0x80273a28
 */
extern Ambient g_MenuAmbientLight;

/**
 * The directional light of the menus that draw models.
 *
 * @ghidraAddress USA: 0x80271f90
 * @ghidraAddress EUR: 0x80273f70
 * @ghidraAddress JPN: 0x80273a30
 */
extern Light g_MenuDirectionalLight;

/**
 * Whether DrawModelList() draws without the depth buffer.
 *
 * @ghidraAddress USA: 0x80271d98
 * @ghidraAddress EUR: 0x80273d78
 * @ghidraAddress JPN: 0x80273838
 * @ghidraAddress BRD3D: 0x10690a1c
 * @ghidraAddress BRGlide: 0x105ccbc8
 */
extern s32 g_dwDrawModelListNoZBuffer;

/**
 * The light of the parts of a model lit by their light ramp, by its index.
 *
 * @ghidraAddress USA: 0x80271d9c
 * @ghidraAddress EUR: 0x80273d7c
 * @ghidraAddress JPN: 0x8027383c
 * @ghidraAddress BRD3D: 0x100aa5d0
 * @ghidraAddress BRGlide: 0x100a9930
 */
extern u32 g_adwModelShadeColors[4];

/**
 * Draw the parts of a model, the opaque ones first, under a matrix.
 *
 * @param model The model.
 * @param m The matrix of the model.
 * @ghidraAddress USA: 0x80209d70
 * @ghidraAddress EUR: 0x80209b50
 * @ghidraAddress JPN: 0x80209750
 * @ghidraAddress BRD3D: 0x1002fb20
 * @ghidraAddress BRGlide: 0x1001d1b0
 */
void DrawModelList(Model *model, f32 m[4][4]);

/**
 * Draw a model turned about its own axis by an angle in radians, at a 32nd of its size, stood up
 * and placed at a position. A mirrored track mirrors the turn and the scale.
 *
 * @param model The model to draw.
 * @param x The X coordinate of the position.
 * @param y The Y coordinate of the position.
 * @param z The Z coordinate of the position.
 * @param angle The turn in radians.
 * @ghidraAddress USA: 0x8020a524
 * @ghidraAddress EUR: 0x8020a304
 * @ghidraAddress JPN: 0x80209f04
 */
void DrawRotatedModel(Model *model, f32 x, f32 y, f32 z, f32 angle);

/**
 * Draw a model turned about its own axis by an angle in radians, scaled, stood up and placed at a
 * position. A mirrored track mirrors the turn and the scale.
 *
 * @param model The model to draw.
 * @param x The X coordinate of the position.
 * @param y The Y coordinate of the position.
 * @param z The Z coordinate of the position.
 * @param angle The turn in radians.
 * @param scale The scale.
 * @ghidraAddress USA: 0x8020a6a4
 * @ghidraAddress EUR: 0x8020a484
 * @ghidraAddress JPN: 0x8020a084
 */
void DrawModelAt(Model *model, f32 x, f32 y, f32 z, f32 angle, f32 scale);

/**
 * Load a model into a buffer and turn its offsets into pointers.
 *
 * @param dst The destination buffer.
 * @param source The archive path on Windows, or the cartridge offset on the console.
 * @return The loaded model.
 * @ghidraAddress USA: 0x8021de2c
 * @ghidraAddress EUR: 0x8021f02c
 * @ghidraAddress JPN: 0x8021e8fc
 * @ghidraAddress BRD3D: 0x10036bd0
 * @ghidraAddress BRGlide: 0x10030270
 */
#if TGR_BOSS_RALLY
Model *DecompressModel(void *dst, const char *source);
#else
Model *DecompressModel(void *dst, u32 source);
#endif

/**
 * Move a pointer that falls in a range of offsets to the same place from a base.
 *
 * @param pointer The pointer, as the word it is stored in.
 * @param lo The start of the range.
 * @param hi The end of the range.
 * @param base Where the start of the range moves to.
 * @ghidraAddress USA: 0x8021d070
 * @ghidraAddress EUR: 0x8021e270
 * @ghidraAddress JPN: 0x8021db40
 * @ghidraAddress BRD3D: 0x10035060
 * @ghidraAddress BRGlide: 0x1002e711
 */
void RemapPointer(u32 *pointer, u32 lo, u32 hi, u32 base);

/**
 * Remap the addresses of the vertices and textures of a display list, up to its end. The display
 * lists it calls are not followed.
 *
 * @param displayList The display list, or NULL.
 * @param lo The start of the range of the addresses.
 * @param hi The end of the range.
 * @param base Where the start of the range moves to.
 * @ghidraAddress USA: 0x8021d098
 * @ghidraAddress EUR: 0x8021e298
 * @ghidraAddress JPN: 0x8021db68
 * @ghidraAddress BRD3D: 0x10035089
 * @ghidraAddress BRGlide: 0x1002e73a
 */
void FixupDisplayListPointers(Gfx *displayList, u32 lo, u32 hi, u32 base);

#if TGR_PLATFORM == TGR_PLATFORM_SDL && !TGR_BOSS_RALLY
/**
 * Turn a model of the console, as it inflates from the cartridge, into a model of the host: its
 * fields in the order of the host and its offsets into pointers.
 *
 * @param model The model.
 * @param size The bytes of the model.
 */
void ReadConsoleModel(Model *model, u32 size);
#endif

/**
 * Make every animation of a model repeat by turning back at its end.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d828
 * @ghidraAddress EUR: 0x8021ea28
 * @ghidraAddress JPN: 0x8021e2f8
 * @ghidraAddress BRD3D: 0x10035625
 * @ghidraAddress BRGlide: 0x1002ecd6
 */
void BounceModelAnimations(Model *model);

/**
 * Turn the offsets a model is loaded with into pointers: its animations, their vertices and keys,
 * and the display lists of its parts.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021dc34
 * @ghidraAddress EUR: 0x8021ee34
 * @ghidraAddress JPN: 0x8021e704
 * @ghidraAddress BRD3D: 0x10036c00
 * @ghidraAddress BRGlide: 0x100302a0
 */
void RelocateModelPointers(Model *model);

/**
 * Set flags on every animation of a model, then clear others.
 *
 * @param model The model.
 * @param set The flags to set.
 * @param clear The flags to clear.
 * @ghidraAddress USA: 0x8021d6dc
 * @ghidraAddress EUR: 0x8021e8dc
 * @ghidraAddress JPN: 0x8021e1ac
 * @ghidraAddress BRD3D: 0x10035585
 * @ghidraAddress BRGlide: 0x1002ec36
 */
void SetModelAnimationFlags(Model *model, s32 set, s32 clear);

/**
 * Make every animation of a model wrap to its start at its end, instead of turning back.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d804
 * @ghidraAddress EUR: 0x8021ea04
 * @ghidraAddress JPN: 0x8021e2d4
 * @ghidraAddress BRD3D: 0x10035610
 * @ghidraAddress BRGlide: 0x1002ecc1
 */
void LoopModelAnimations(Model *model);

/**
 * Advance every animation of a model by the time of the frame, blending its vertices between
 * keyframes.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d84c
 * @ghidraAddress EUR: 0x8021ea4c
 * @ghidraAddress JPN: 0x8021e31c
 * @ghidraAddress BRD3D: 0x1003563a
 * @ghidraAddress BRGlide: 0x1002eceb
 */
void AdvanceModelAnimations(Model *model);

/**
 * Make the animations of a model stop at their ends rather than repeat.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d7e0
 * @ghidraAddress EUR: 0x8021e9e0
 * @ghidraAddress JPN: 0x8021e2b0
 * @ghidraAddress BRD3D: 0x100355fb
 * @ghidraAddress BRGlide: 0x1002ecac
 */
void PlayModelAnimationsOnce(Model *model);

/**
 * Read a model from the cartridge and turn its offsets into pointers.
 *
 * @param dst The memory to read it into.
 * @param romStart The cartridge offset of the model.
 * @param romEnd The cartridge offset of its end.
 * @return The model, at dst.
 * @ghidraAddress USA: 0x8021ddfc
 * @ghidraAddress EUR: 0x8021effc
 * @ghidraAddress JPN: 0x8021e8cc
 */
Model *DmaReadAndFixupTrackModel(void *dst, u32 romStart, u32 romEnd);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_MODEL_H */
