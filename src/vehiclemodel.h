#ifndef COASTLINE_VEHICLEMODEL_H
#define COASTLINE_VEHICLEMODEL_H

#include "carphysics.h"
#include "displaylistops.h"
#include "render.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/assetnames.h"
#endif

/** The wheels of a vehicle. */
#define VEHICLE_WHEELS 4

/** How a car shows the revolutions of its engine. */
enum {
    TACHOMETER_NEEDLE,
    TACHOMETER_BAR_DIMMED,
    TACHOMETER_BAR_HIDDEN,
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u8 abUnknown0[4]; /* +0x00 */
    s32 textureEntries[30];
    s32 textureEntryCount;
    s32 rearLightTextureEntry;
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
    s8 driveMode;
    u8 abUnknownDa; /* +0xda */
    u8 tachometer;
    u8 abUnknownDc[8]; /* +0xdc */
    s8 dashboardWidth;
    s8 dashboardHeight;
    s8 gearX;
    s8 gearY;
    s8 gearWidth;
    s8 gearHeight;
    union {
        s8 needleX;
        s8 barSegments;
    };
    s8 needleY;
    f32 needleFullAngle;
    f32 needleRestAngle;
    u8 abUnknownF4[8]; /* +0xf4 */
    s32 gearImageBytes;
    u16 dashboardPalettes[2][256];
    u8 dashboardImages[0x7b00];
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

    u32 triangleCount; /*!< The triangles of the model, counted into the statistics of the frame. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    void *unreconstructed4; /* +0x8004 */
    u32 unreconstructed8;   /* +0x8008 */
    void *unreconstructedC; /* +0x800c */
#else
    u8 unreconstructed4[0x10 - 0x4]; /*!< Not read by the routines reconstructed so far. */
#endif
    s32 textureCount;       /*!< The number of textures of the model. */
    ModelTexture *textures; /*!< The textures of the model, textureCount of them. */
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN

/**
 * @ghidraAddress BRD3D: 0x100370d0
 * @ghidraAddress BRGlide: 0x10030770
 */
void ByteSwapAndRelocateCarModel(VehicleModel *model);

/**
 * Accept the vehicle model without checking cartridge metadata.
 *
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_VEHICLEMODEL_H */
