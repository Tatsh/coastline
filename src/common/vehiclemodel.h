#ifndef COASTLINE_VEHICLEMODEL_H
#define COASTLINE_VEHICLEMODEL_H

#include "common/carphysics.h"
#include "common/displaylistops.h"
#include "common/ptr32.h"
#include "common/render.h"

#if TGR_BOSS_RALLY
#include "common/assetnames.h"
#endif

/** The wheels of a vehicle. */
#define VEHICLE_WHEELS 4

/** How a car shows the revolutions of its engine. */
enum {
    TACHOMETER_NEEDLE,     /*!< A rotating needle over the dashboard face. */
    TACHOMETER_BAR_DIMMED, /*!< A segmented bar with unlit segments dimmed. */
    TACHOMETER_BAR_HIDDEN, /*!< A segmented bar with unlit segments hidden. */
};

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
#if TGR_BOSS_RALLY
    u8 abUnknown0[4]; /*!< Unreconstructed bytes before captured texture entries. */ /* +0x00 */
    s32 textureEntries[30];     /*!< Captured backend texture entries for this car. */
    s32 textureEntryCount;      /*!< The number of recorded entries in textureEntries. */
    s32 rearLightTextureEntry;  /*!< The rear-light texture entry replaced by a lighting variant. */
    s32 lightTextureEntries[4]; /*!< Rear-light variants for braking and race lighting. */
    u8 abUnknown94[2]; /*!< Unreconstructed bytes before the drive configuration. */ /* +0x94 */
    s8 rearWheelDrive;               /*!< Whether the specification identifies rear-wheel drive. */
    s8 tableIndex;                   /*!< The car class used by arcade record-time tables. */
    f32 gearRatios[CAR_GEAR_RATIOS]; /*!< The ratio of each gear, starting with neutral. */
    f32 torqueCurve[4];              /*!< Cubic engine torque coefficients, highest power first. */
    f32 finalDrive;                  /*!< The final drive ratio. */
    Vec3f boxSize;                   /*!< Collision-box dimensions in model units. */
    f32 boxCenterZ;                  /*!< Collision-box centre height in model units. */
    s8 gearCount;                    /*!< The number of forward gears. */
    s8 driveMode; /*!< Rear-biased drive for 0, front drive for 1, or all-wheel drive otherwise. */
    u8 abUnknownDa; /*!< An unreconstructed byte before the tachometer selector. */ /* +0xda */
    u8 tachometer; /*!< The TACHOMETER_ presentation mode. */
    u8 abUnknownDc[8]; /*!< Unreconstructed bytes before dashboard dimensions. */ /* +0xdc */
    s8 dashboardWidth;  /*!< The width of the dashboard face in texels. */
    s8 dashboardHeight; /*!< The height of the dashboard face in texels. */
    s8 gearX;           /*!< The gear cell's x coordinate on the dashboard face, in pixels. */
    s8 gearY;           /*!< The gear cell's y coordinate on the dashboard face, in pixels. */
    s8 gearWidth;       /*!< The gear cell width in texels. */
    s8 gearHeight;      /*!< The gear cell height in texels. */
    /** Tachometer placement or segment count, selected by tachometer. */
    union {
        s8 needleX;     /*!< The needle pivot's x coordinate on the dashboard face, in pixels. */
        s8 barSegments; /*!< The bar segment count, using the final palette colours. */
    };
    s8 needleY;          /*!< The needle pivot's y coordinate on the dashboard face, in pixels. */
    f32 needleFullAngle; /*!< The needle angle at full tachometer scale, in radians. */
    f32 needleRestAngle; /*!< The needle angle at rest, in radians. */
    u8 abUnknownF4[8]; /*!< Unreconstructed bytes before the gear-image stride. */ /* +0xf4 */
    s32 gearImageBytes;            /*!< The byte stride between consecutive decoded gear images. */
    u16 dashboardPalettes[2][256]; /*!< Day and night RGBA5551 dashboard palettes. */
    u8 dashboardImages[0x7b00];    /*!< Dashboard face and gear-cell image storage. */
#endif                             /* TGR_BOSS_RALLY */

    u32 triangleCount; /*!< The triangles of the model, counted into the statistics of the frame. */
#if TGR_BOSS_RALLY
    TGR_PTR(void) unreconstructed4; /*!< An unreconstructed pointer. */           /* +0x8004 */
    u32 unreconstructed8; /*!< An unreconstructed word between model pointers. */ /* +0x8008 */
    TGR_PTR(void) unreconstructedC; /*!< An unreconstructed pointer. */           /* +0x800c */
#else
    u8 unreconstructed4[0x10 - 0x4]; /*!< Not read by the routines reconstructed so far. */
#endif
    s32 textureCount;               /*!< The number of textures of the model. */
    TGR_PTR(ModelTexture) textures; /*!< The textures of the model, textureCount of them. */
    TGR_PTR(DisplayListCommand)
    displayLists[VEHICLE_LODS][VEHICLE_DISPLAY_LISTS]; /*!< The display lists of
                                     each level of detail, by the
                                     VEHICLE_DL_ values. */
    TGR_PTR(void)
    unreconstructed90; /*!< An offset turned into a pointer with the rest, not read by the
                    routines reconstructed so far. */
    TGR_PTR(void)
    unreconstructed94; /*!< An offset turned into a pointer with the rest, not read by the
                    routines reconstructed so far. */
    u8 unreconstructed98[0xB0 - 0x98]; /*!< Not read by the routines reconstructed so far. */
    f32 cockpitForward; /*!< How far forward of the vehicle the cockpit camera sits. */
    f32 mirrorForward;  /*!< Half of how far forward of the vehicle the mirror camera sits. */
    f32 cockpitUp;      /*!< How far above the vehicle the cockpit and mirror cameras sit. */
    TGR_PTR(DisplayListCommand)
    wheelDisplayLists[VEHICLE_LODS][VEHICLE_WHEEL_DISPLAY_LISTS]; /*!< The
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
    TGR_PTR(TGR_PTR(u8))
    decalMasks; /*!< The masks of the materials that the paint shop paints, one bit a texel
     with a set bit protecting it, or NULL where every texel can be
     painted. */
    u8 unreconstructed120[0xDF88 - 0x120]; /*!< The rest of the loaded model, not read as fields by
                                                the routines reconstructed so far. */
} VehicleModel;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The models of the cars the vehicles drive, in slots. The link script places them.
 *
 * @ghidraAddress USA: 0x803c8000
 * @ghidraAddress EUR: 0x803c8000
 * @ghidraAddress JPN: 0x803c8000
 * @ghidraAddress BRD3D: 0x100c12a0
 * @ghidraAddress BRGlide: 0x100bcdd0
 */
extern VehicleModel g_aVehicleModels[];

/**
 * The car whose model each slot holds.
 *
 * @ghidraAddress USA: 0x8031b238
 * @ghidraAddress EUR: 0x8031d738
 * @ghidraAddress JPN: 0x8031c368
 * @ghidraAddress BRD3D: 0x106c6558
 * @ghidraAddress BRGlide: 0x106ed5e8
 */
extern s32 g_adwVehicleModelSlotCars[];

/**
 * Load a car model into a slot, fix up its pointers, and record the car.
 *
 * @param slot The slot of g_aVehicleModels.
 * @param car The index of the car in g_aCarModels.
 * @param source A decompressed N64 model to copy, or NULL to load the asset. Windows treats a
 * non-null source as fatal.
 * @ghidraAddress USA: 0x8021d5e4
 * @ghidraAddress EUR: 0x8021e7e4
 * @ghidraAddress JPN: 0x8021e0b4
 * @ghidraAddress BRD3D: 0x10035520
 * @ghidraAddress BRGlide: 0x1002ebd1
 */
void LoadCar(s32 slot, s32 car, void *source);

/**
 * Load a car model from platform asset storage.
 *
 * @param model Where to decompress it.
 * @param car The index of the car in g_aCarModels.
 * @ghidraAddress USA: 0x8021d534
 * @ghidraAddress EUR: 0x8021e734
 * @ghidraAddress JPN: 0x8021e004
 * @ghidraAddress BRD3D: 0x10037740
 * @ghidraAddress BRGlide: 0x10030de0
 */
void LoadVehicleModelFromRom(VehicleModel *model, s32 car);

/**
 * Paint the materials of a model a colour of 5 bits a channel.
 *
 * @param model The model.
 * @param red The red of the colour, from 0 to 31.
 * @param green The green of the colour, from 0 to 31.
 * @param blue The blue of the colour, from 0 to 31.
 * @ghidraAddress USA: 0x8021d140
 * @ghidraAddress EUR: 0x8021e340
 * @ghidraAddress JPN: 0x8021dc10
 * @ghidraAddress BRD3D: 0x100350ee
 * @ghidraAddress BRGlide: 0x1002e79f
 */
void SetMaterialColor(VehicleModel *model, s32 red, s32 green, s32 blue);

/**
 * Relocate loaded model pointers on N64. Windows does not require relocation here.
 *
 * @param model The model.
 * @ghidraAddress USA: 0x8021d32c
 * @ghidraAddress EUR: 0x8021e52c
 * @ghidraAddress JPN: 0x8021ddfc
 * @ghidraAddress BRD3D: 0x1003551b
 * @ghidraAddress BRGlide: 0x1002ebcc
 */
void FixupVehicleModelPointers(VehicleModel *model);

#if TGR_BOSS_RALLY

/**
 * Swap model fields, relocate geometry and texture pointers, and prepare backend car textures.
 *
 * @param model The writable car model loaded in file byte order.
 * @ghidraAddress BRD3D: 0x100370d0
 * @ghidraAddress BRGlide: 0x10030770
 */
void ByteSwapAndRelocateCarModel(VehicleModel *model);

/**
 * Accept the vehicle model without checking cartridge metadata.
 *
 * @return Always 1 for the PC release.
 * @ghidraAddress BRD3D: 0x1003557b
 * @ghidraAddress BRGlide: 0x1002ec2c
 */
s32 IsVehicleModelValid(void);

/**
 * The model whose material textures are being captured.
 *
 * @ghidraAddress BRD3D: 0x106c7c3c
 * @ghidraAddress BRGlide: 0x106eeccc
 */
extern VehicleModel *g_pTextureCaptureVehicleModel;

/**
 * The captured entries that exceeded the model's texture entry array.
 *
 * @ghidraAddress BRD3D: 0x106c7c40
 */
extern s32 g_dwTextureCaptureOverflowCount;

/**
 * Append a texture entry to the current model, counting entries beyond its array capacity.
 *
 * @param entry The captured texture entry.
 * @ghidraAddress BRD3D: 0x10037030
 * @ghidraAddress BRGlide: 0x100306d0
 */
void AppendVehicleModelTextureEntry(s32 entry);

/**
 * Test whether the palette belongs to a CI4 material of the current model.
 *
 * @param palette The palette to find.
 * @return Whether a material uses the palette.
 * @ghidraAddress BRD3D: 0x10037070
 * @ghidraAddress BRGlide: 0x10030710
 */
s32 IsVehicleMaterialPalette(const void *palette);
#endif /* TGR_BOSS_RALLY */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEMODEL_H */
