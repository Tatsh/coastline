#ifndef COASTLINE_MODEL_H
#define COASTLINE_MODEL_H

/**
 * @file
 * The models of the menus and splash screens: loading, animating and drawing them.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "vecmath.h"

/** A part of a model: its display list, how it is drawn, and where. */
typedef struct {
    Gfx *displayList; /*!< The display list that draws the part, or NULL. */
    u16 flags;        /*!< The MODEL_PART_ flags. */
    Vec3f position;   /*!< Where the part is drawn, relative to the model. */
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
    s32 vertexCount;        /*!< The number of vertices the animation moves. */
    Vtx *vertices;          /*!< The vertices the animation moves. */
    void *unreconstructed8; /*!< Turned into a pointer with the rest, and never read. */
    s32 keyCount;           /*!< The number of keys. */
    u16 flags;              /*!< The MODEL_ANIMATION_ flags. */
    u16 startKey;           /*!< The key a search for the time starts from. */
    f32 startTime;          /*!< The time the animation starts at, in seconds. */
    f32 endTime;            /*!< The time the animation ends at, in seconds. */
    f32 time;               /*!< The current time of the animation, in seconds. */
    ModelKeyframe *keys[];  /*!< The keys, in order of time. */
} ModelAnimation;

/** The animations of a model. */
typedef struct {
    s32 count;                 /*!< The number of animations. */
    ModelAnimation *entries[]; /*!< The animations. */
} ModelAnimationList;

/**
 * A model loaded from the cartridge, with its display lists and the animations of its parts. Its
 * fields join this structure as they are reconstructed.
 */
typedef struct Model {
    u32 partCount;                  /*!< The number of parts. */
    ModelAnimationList *animations; /*!< The animations of the vertices of the model. */
    ModelPart parts[];              /*!< The parts. */
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
 */
extern s32 g_dwDrawModelListNoZBuffer;

/**
 * Draw the parts of a model, the opaque ones first, under a matrix.
 *
 * @param model The model.
 * @param m The matrix of the model.
 * @ghidraAddress USA: 0x80209d70
 * @ghidraAddress EUR: 0x80209b50
 * @ghidraAddress JPN: 0x80209750
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
 * Decompress a model from the cartridge to a buffer and turn its offsets into pointers.
 *
 * @param dst The buffer to decompress to.
 * @param romOffset The offset of the compressed model on the cartridge.
 * @return The buffer.
 * @ghidraAddress USA: 0x8021de2c
 * @ghidraAddress EUR: 0x8021f02c
 * @ghidraAddress JPN: 0x8021e8fc
 */
Model *DecompressModel(void *dst, u32 romOffset);

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
 */
void FixupDisplayListPointers(Gfx *displayList, u32 lo, u32 hi, u32 base);

/**
 * Make every animation of a model repeat by turning back at its end.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d828
 * @ghidraAddress EUR: 0x8021ea28
 * @ghidraAddress JPN: 0x8021e2f8
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
 */
void SetModelAnimationFlags(Model *model, s32 set, s32 clear);

/**
 * Make every animation of a model wrap to its start at its end, instead of turning back.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d804
 * @ghidraAddress EUR: 0x8021ea04
 * @ghidraAddress JPN: 0x8021e2d4
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
 */
void AdvanceModelAnimations(Model *model);

/**
 * Make the animations of a model stop at their ends rather than repeat.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d7e0
 * @ghidraAddress EUR: 0x8021e9e0
 * @ghidraAddress JPN: 0x8021e2b0
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

#endif
