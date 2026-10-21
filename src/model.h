#ifndef COASTLINE_MODEL_H
#define COASTLINE_MODEL_H

/**
 * @file
 * The models of the menus and splash screens: loading, animating and drawing them.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

/** A model loaded from the cartridge, with its display lists and the animations of its parts. */
typedef struct Model Model;

/** The ambient light of the menus that draw models. @ghidraAddress USA: 0x80271f88 */
extern Ambient g_MenuAmbientLight;

/** The directional light of the menus that draw models. @ghidraAddress USA: 0x80271f90 */
extern Light g_MenuDirectionalLight;

/** Whether DrawModelList() draws without the depth buffer. @ghidraAddress USA: 0x80271d98 */
extern s32 g_dwDrawModelListNoZBuffer;

/**
 * Draw a model turned about its own axis by an angle in radians, scaled, stood up and placed at a
 * position. A mirrored track mirrors the turn and the scale.
 *
 * @ghidraAddress USA: 0x8020a6a4
 */
void DrawModelAt(Model *model, f32 x, f32 y, f32 z, f32 angle, f32 scale);

/**
 * Decompress a model from the cartridge to a buffer and turn its offsets into pointers.
 *
 * @return The buffer.
 * @ghidraAddress USA: 0x8021de2c
 */
Model *DecompressModel(void *dst, u32 romOffset);

/**
 * Set flags on every animation of a model, then clear others.
 *
 * @ghidraAddress USA: 0x8021d6dc
 */
void SetModelAnimationFlags(Model *model, u16 set, u16 clear);

/**
 * Make every animation of a model wrap to its start at its end, instead of turning back.
 *
 * @ghidraAddress USA: 0x8021d804
 */
void LoopModelAnimations(Model *model);

/**
 * Advance every animation of a model by the time of the frame, blending its vertices between
 * keyframes.
 *
 * @ghidraAddress USA: 0x8021d84c
 */
void AdvanceModelAnimations(Model *model);

#endif
