#ifndef COASTLINE_VEHICLEMODEL_H
#define COASTLINE_VEHICLEMODEL_H

#include "carphysics.h"
#include "displaylistops.h"
#include "render.h"

/** The wheels of a vehicle. */
#define VEHICLE_WHEELS 4

/* The detail of the model of a car. */
#define VEHICLE_LODS 3           /*!< The levels of detail of the model, the nearest first. */
#define VEHICLE_DISPLAY_LISTS 10 /*!< The display lists of each level of detail. */

/** The display lists of a wheel of a car: the plain wheel, one not drawn, and one in the snow. */
enum {
    VEHICLE_WHEEL_DL_NORMAL,     /*!< The plain wheel. */
    VEHICLE_WHEEL_DL_1,          /*!< A wheel the renderer does not draw. */
    VEHICLE_WHEEL_DL_SNOW,       /*!< The wheel in the snow. */
    VEHICLE_WHEEL_DISPLAY_LISTS, /*!< The number of display lists of a wheel. */
};

/* The textures whose palettes give the colours of a car. */
#define VEHICLE_MATERIALS 12            /*!< The textures of the materials of a car. */
#define VEHICLE_MATERIAL_PAINT 2        /*!< The texture whose palette is the paint. */
#define VEHICLE_MATERIAL_HEADLIGHTS 10  /*!< The texture whose palette is the headlights. */
#define VEHICLE_MATERIAL_REAR_LIGHTS 11 /*!< The texture whose palette is the rear lights. */

/** The display lists of the model of a car that the renderer draws. */
enum {
    VEHICLE_DL_NEAR_DETAIL = 3, /*!< Details drawn only near the camera, at the nearest level. */
    VEHICLE_DL_BODY,            /*!< The paint, under the reflection of the sky. */
    VEHICLE_DL_HEADLIGHT_BEAMS, /*!< The beams of the headlights, at night or in the rain. */
    VEHICLE_DL_UNDERSIDE,       /*!< The underside, when the camera is below the car. */
    VEHICLE_DL_TEXTURED = 8,    /*!< Lit, textured parts, cut out by the alpha of their textures. */
    VEHICLE_DL_HIGHLIGHTED,     /*!< The parts that take the specular highlight. */
};

/**
 * The model of a car as it is loaded from the cartridge. Its fields join this structure as they are
 * reconstructed.
 */
typedef struct VehicleModel {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u8 abUnknown0[4]; /* +0x00 */
    s32 textureEntries[30];
    s32 textureEntryCount;
    u8 abUnknown80[4]; /* +0x80 */
    s32 lightTextureEntries[4];
    u8 abUnknown94[2]; /* +0x94 */
    s8 rearWheelDrive;
    s8 tableIndex;
    f32 gearRatios[CAR_GEAR_RATIOS];
    f32 torqueCurve[4];
    f32 finalDrive;
    Vec3f boxSize;
    f32 boxCenterZ;
    s8 gearCount;
    u8 abUnknownD9[0x7f27]; /* +0xd9 */
#endif
    u32 triangleCount; /*!< The triangles of the model, counted into the statistics of the frame. */
    u8 unreconstructed4[0x10 - 0x4]; /*!< Not read by the routines reconstructed so far. */
    s32 textureCount;                /*!< The number of textures of the model. */
    ModelTexture *textures;          /*!< The textures of the model, textureCount of them. */
    DisplayListCommand *displayLists[VEHICLE_LODS][VEHICLE_DISPLAY_LISTS]; /*!< The display lists of
                                                                 each level of detail, by the
                                                                 VEHICLE_DL_ values. */
    void *unreconstructed90; /*!< An offset turned into a pointer with the rest, not read by the
                                  routines reconstructed so far. */
    void *unreconstructed94; /*!< An offset turned into a pointer with the rest, not read by the
                                  routines reconstructed so far. */
    u8 unreconstructed98[0xB0 - 0x98]; /*!< Not read by the routines reconstructed so far. */
    f32 cockpitForward; /*!< How far forward of the vehicle the cockpit camera sits. */
    f32 mirrorForward;  /*!< Half of how far forward of the vehicle the mirror camera sits. */
    f32 cockpitUp;      /*!< How far above the vehicle the cockpit and mirror cameras sit. */
    DisplayListCommand *wheelDisplayLists[VEHICLE_LODS][VEHICLE_WHEEL_DISPLAY_LISTS]; /*!< The
                                                                            display lists of a wheel
                                                                            at each level of detail,
                                                                            by the VEHICLE_WHEEL_DL_
                                                                            values. */
    Vec3f wheels[VEHICLE_WHEELS]; /*!< Where the four wheels are drawn. The physics takes the
                                       second and the third as a front and a rear wheel of one side
                                       and mirrors them across, and the shadow is sized by the x
                                       and y of the first. */
    u8 materialTextures[VEHICLE_MATERIALS]; /*!< The textures whose palettes the colours of the car
                                                 are set in, by the VEHICLE_MATERIAL_ values. */
    u8 **decalMasks; /*!< The masks of the materials that the paint shop paints, one bit a texel
                          with a set bit protecting it, or NULL where every texel can be
                          painted. */
    u8 unreconstructed120[0xDF88 - 0x120]; /*!< The rest of the loaded model, not read as fields by
                                                the routines reconstructed so far. */
} VehicleModel;

#endif
