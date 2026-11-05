#include "paintshop.h"

#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "common/camera.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/render.h"
#include "common/rumble.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"
#include "decaldraw.h"
#include "options.h"
#include "pakmenu.h"
#include "racemenu.h"

PaletteSwatch g_aDecalSwatches[DECAL_SWATCHES];

/* The decal slot the paint shop opens on, and the brush and color it opens with. */
#define DEFAULT_DECAL_SLOT 2
#define DEFAULT_BRUSH_SIZE 2
#define DEFAULT_DECAL_COLOR 3

/* Where the cursor starts. */
#define CURSOR_START_X 244
#define CURSOR_START_Y 272

/* The decal slots on each side of the car that a mirror copies between. */
#define SLOT_LEFT_DOOR 0
#define SLOT_RIGHT_DOOR 1
#define SLOT_LEFT_FRONT_FENDER 2
#define SLOT_RIGHT_FRONT_FENDER 3
#define SLOT_ROOF 5
#define SLOT_LEFT_REAR_FENDER 6
#define SLOT_RIGHT_REAR_FENDER 7

/* The slots the texts of the mirror menu cover, the doors and the fenders among them. */
#define MIRROR_TEXT_SLOTS 8

/* The cars that sit higher and lower than the rest in the preview. */
#define CAR_RAISED_IN_PREVIEW 1
#define CAR_LOWERED_IN_PREVIEW 10

/* The frames the preview takes to turn from one decal slot to the next. */
#define PREVIEW_TURN_FRAMES 16

/* The size of the button pictures of the tools. */
#define TOOL_ICON_WIDTH 64
#define TOOL_ICON_HEIGHT 14

/* The size of the pictures of the styles of the rectangle and the oval. */
#define STYLE_ICON_WIDTH 64
#define STYLE_ICON_HEIGHT 42

/* The keys of the keyboard that end and shorten the text. */
#define KEY_BACKSPACE 47
#define KEY_ENTER 49
#define KEYBOARD_COLUMNS 10

/* The frames a pressed key flashes for. */
#define KEY_FLASH_FRAMES 3

/* The file of the decals on the controller pak, and its note name in the font of the pak. */
#if TGR_REGION == TGR_REGION_JP
#define DECAL_FILE_COMPANY 0x3238
#define DECAL_FILE_GAME 0x4E54524A
#elif TGR_REGION == TGR_REGION_EU
#define DECAL_FILE_COMPANY 0x3746
#define DECAL_FILE_GAME 0x4E545250
#else
#define DECAL_FILE_COMPANY 0x3544
#define DECAL_FILE_GAME 0x4E475245
#endif
#define DECAL_FILE_SIZE 0x3E00

/* The bytes of the decals at which the progress bar is full, and the width of the bar. */
#define DECAL_PROGRESS_FULL 0x3D00
#define DECAL_PROGRESS_WIDTH 0xB2

/* The bytes of the palette of the paint saved after the decals. */
#define DECAL_PALETTE_BYTES 0x20

/* The size and the baseline of the title, and the palette of the pictures of the tool buttons. */
#if TGR_REGION == TGR_REGION_JP
#define TITLE_SIZE 30
#define TITLE_Y 37
#define BUTTON_PALETTE g_awPaintShopButtonPalette
#else
#define TITLE_SIZE 34
#define TITLE_Y 38
#define BUTTON_PALETTE NULL
#endif

/* The states of the dialog of the controller pak. */
enum {
    PAK_STATE_FADE_OUT,          /*!< Start to fade out the music and the sound effects. */
    PAK_STATE_WAIT_FADE,         /*!< Wait for the fade, then a few frames more. */
    PAK_STATE_CONFIRM,           /*!< Request confirmation to load or save the decals. */
    PAK_STATE_INIT_PAK,          /*!< Check for a pak and open its file system. */
    PAK_STATE_REPAIR_ID,         /*!< Repair the identity of a pak that could not be read. */
    PAK_STATE_CONFIRM_OVERWRITE, /*!< Request confirmation to overwrite the saved decals. */
    PAK_STATE_FIND_FILE,         /*!< Find the file of the decals on the pak. */
    PAK_STATE_TRANSFER_DECALS,   /*!< Create the file if needed, then move a decal a frame. */
    PAK_STATE_TRANSFER_PALETTE,  /*!< Read or write the palette of the paint. */
    PAK_STATE_NEW_PAK,           /*!< Report that a different pak was inserted. */
    PAK_STATE_REMOVE_RUMBLE_PAK, /*!< Request the swap of a Rumble Pak for a Controller Pak. */
    PAK_STATE_DONE,              /*!< Offer to put the Rumble Pak back after a swap. */
    PAK_STATE_FINISH,            /*!< Fade the sounds back in and close the dialog. */
};

/* The frames the dialog shows its progress before it moves the first decal. */
#define PAK_TRANSFER_DELAY_FRAMES 2

/* The frames after the sounds fade before the controller pak is read. */
#define PAK_FADE_WAIT_FRAMES 3

/* The time of the fades of the sounds around the dialog, in seconds. */
#define PAK_FADE_TIME 0.2f

/* The buttons that dismiss the messages of the dialog. */
#define PAK_DISMISS_BUTTONS (INPUT_Z | INPUT_B | INPUT_A)

/* The choices of the menu of the clear tool: bit 1 for every decal, bit 0 for the body color. */
#define CLEAR_OPTIONS 4
#define CLEAR_ALL_DECALS 2
#define CLEAR_TO_BODY_COLOR 1

/* The choices of the menu of the mirror tool. */
#define MIRROR_OPTIONS 3
#define MIRROR_OPTION_MIRROR 0
#define MIRROR_OPTION_FLIP_HORIZONTAL 1
#define MIRROR_OPTION_FLIP_VERTICAL 2

void LoadTextureFromRom(Sprite *sprite) {
    sprite->pixels = AllocHeapAligned((sprite->width * sprite->height * sprite->bitsPerTexel) >> 3);
    if (DmaReadAssetSize(sprite->romOffset) !=
        (sprite->width * sprite->height * sprite->bitsPerTexel) >> 3) {
#if TGR_REGION != TGR_REGION_JP
        /* The format has five values and is given four, without the computed size. */
        osSyncPrintf(
            "ERROR: AllocPaintShopGfxMem given image %dx%dx%d = %d bytes but uncompressed rom "
            "image is %d bytes\n",
            sprite->width,
            sprite->height,
            sprite->bitsPerTexel,
            DmaReadAssetSize(sprite->romOffset));
#endif /* TGR_REGION != TGR_REGION_JP */
        for (;;) {
        }
    }
    DecompressFromRom(sprite->pixels, sprite->romOffset, NULL);
}

/* Step a vector from one to another over the frames of the turn of the preview. */
static inline void InterpolatePreviewAxis(Vec3f *out, Vec3f *from, Vec3f *to) {
    f32 stepX = (to->x - from->x) / 16.0f;
    f32 stepY = (to->y - from->y) / 16.0f;
    f32 stepZ = (to->z - from->z) / 16.0f;

    out->x = from->x + g_wScreenFrameCount * stepX;
    out->y = from->y + g_wScreenFrameCount * stepY;
    out->z = from->z + g_wScreenFrameCount * stepZ;
}

/* The up axis of the car that shows a decal slot, tilted for the roof. */
static inline void SetPreviewUp(Vec3f *up, u8 slot) {
    if (slot == SLOT_ROOF) {
        up->x = 5.0f;
        up->y = 5.0f;
        up->z = 1.5f;
    } else {
        up->x = 0.0f;
        up->y = 0.0f;
        up->z = 1.0f;
    }
}

void RenderDecalPreview3D(void) {
    Vec3f fromUp = {0.0f, 0.0f, 0.0f};
    Vec3f toUp = {0.0f, 0.0f, 0.0f};
    Vec3f axis, side, to, from;
    s32 halfHeight, halfWidth;

    g_pActiveCameraVehicle = &g_aVehicles[g_bPaintShopPlayer];
    g_pActiveCamera = g_pActiveCameraVehicle->activeCamera =
        &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    /* The car turns in front of a still camera. */
    if (g_bDecalSlotChanged) {
        from.x = s_aPanelViewDirections[g_bPreviousDecalSlot].x;
        from.y = s_aPanelViewDirections[g_bPreviousDecalSlot].y;
        from.z = s_aPanelViewDirections[g_bPreviousDecalSlot].z;
        to.x = s_aPanelViewDirections[g_bDecalSlot].x;
        to.y = s_aPanelViewDirections[g_bDecalSlot].y;
        to.z = s_aPanelViewDirections[g_bDecalSlot].z;
        g_wScreenFrameCount++;
        Vec3fNormalize(&from);
        Vec3fNormalize(&to);
        if (Vec3fDot(&to, &from) < -0.9) {
            /* Nearly opposite directions turn about the vertical through a side step. */
            axis.x = 0.0f;
            axis.y = 0.0f;
            axis.z = 1.0f;
            Vec3fCross(&side, &axis, &from);
            if (g_wScreenFrameCount < PREVIEW_TURN_FRAMES / 2) {
                Vec3fAddInPlace(&to, &side);
            } else {
                Vec3fAddInPlace(&from, &side);
            }
        }
        InterpolatePreviewAxis(&g_pActiveCameraVehicle->frame.forward, &from, &to);
        Vec3fNormalize(&g_pActiveCameraVehicle->frame.forward);
        SetPreviewUp(&toUp, g_bDecalSlot);
        SetPreviewUp(&fromUp, g_bPreviousDecalSlot);
        Vec3fNormalize(&fromUp);
        Vec3fNormalize(&toUp);
        InterpolatePreviewAxis(&g_pActiveCameraVehicle->frame.up, &fromUp, &toUp);
        Vec3fNormalize(&g_pActiveCameraVehicle->frame.up);
        if (g_wScreenFrameCount == PREVIEW_TURN_FRAMES) {
            g_wScreenFrameCount = 0;
            g_bDecalSlotChanged = FALSE;
        }
    } else {
        g_pActiveCameraVehicle->frame.forward.x = s_aPanelViewDirections[g_bDecalSlot].x;
        g_pActiveCameraVehicle->frame.forward.y = s_aPanelViewDirections[g_bDecalSlot].y;
        g_pActiveCameraVehicle->frame.forward.z = s_aPanelViewDirections[g_bDecalSlot].z;
        Vec3fNormalize(&g_pActiveCameraVehicle->frame.forward);
    }
    Vec3fCross(&g_pActiveCameraVehicle->frame.side,
               &g_pActiveCameraVehicle->frame.up,
               &g_pActiveCameraVehicle->frame.forward);
    Vec3fNormalize(&g_pActiveCameraVehicle->frame.side);
    Vec3fCross(&g_pActiveCameraVehicle->frame.up,
               &g_pActiveCameraVehicle->frame.forward,
               &g_pActiveCameraVehicle->frame.side);
    /* The heading is computed twice. */
    g_pActiveCameraVehicle->cameraHeading =
        Atan2f(g_pActiveCamera->forward.x, g_pActiveCamera->forward.y);
    g_pActiveCameraVehicle->cameraHeading =
        Atan2f(g_pActiveCamera->forward.x, g_pActiveCamera->forward.y);
    Vec3fScale(&g_pActiveCameraVehicle->frame.position, &g_pActiveCameraVehicle->frame.up, -0.65f);
    SetupCameraMatrix(
        g_pActiveCamera, g_fCameraFovRadians, g_fCameraProjectionScale * 0.2, 320.0f, 200.0f);
    if (g_aVehicles[g_bPaintShopPlayer].carModel == CAR_RAISED_IN_PREVIEW) {
        halfHeight = g_dwScreenHeight >> 1;
        halfWidth = g_dwScreenWidth >> 1;
        SetViewport(0, halfHeight - 3, halfWidth - 4, halfHeight - 3, TRUE);
    } else if (g_aVehicles[g_bPaintShopPlayer].carModel == CAR_LOWERED_IN_PREVIEW) {
        halfHeight = g_dwScreenHeight >> 1;
        halfWidth = g_dwScreenWidth >> 1;
        SetViewport(0, halfHeight + 2, halfWidth - 4, halfHeight - 3, TRUE);
    } else {
        halfHeight = g_dwScreenHeight >> 1;
        halfWidth = g_dwScreenWidth >> 1;
        SetViewport(0, halfHeight - 6, halfWidth - 4, halfHeight - 3, TRUE);
    }
    MarkVisibleCell(0, 0);
    TransformWheelPositions(g_bPaintShopPlayer);
    g_bPaintShopPreviewActive = TRUE;
    CheckVehicleVisibility(g_pActiveCameraVehicle);
    RenderVehicleModel(g_pActiveCameraVehicle, 0);
}

/* Point the decal globals at the texels, mask, and size of the texture of a decal slot. */
static inline void SelectDecalTexels(VehicleModel *model, s32 slot) {
    g_pDecalTexels = model->textures[model->materialTextures[slot]].image;
    g_pDecalMask = model->decalMasks[slot];
    g_bDecalWidth = model->textures[model->materialTextures[slot]].width;
    g_bDecalHeight = model->textures[model->materialTextures[slot]].height;
}

/* Set a rectangle of the screen. */
static inline void SetScreenRect(ScreenRect *rect, s32 x, s32 y, s32 width, s32 height) {
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
}

void ProcessPaintShopScreen(void) {
    int stackPrefix[5];
    unsigned char done; /* The Pak transfer takes its address. */
    int stackSuffix[13];
    int i;
    int k;
    int r;
    int x;
    int y;
    Vec3f *v;
    union {
        Vehicle *vehicle;
        u8 *paletteStart;
    } car;
    unsigned int n;
    int dx;
    int dy;
    ScreenRect *area;

    (void)stackPrefix;
    (void)stackSuffix;
    area = &g_rcDecalCanvasFrame[0];
    done = 0;
    g_wSerialInterfaceIdle = 0;
    if ((unsigned char)g_bPaintShopInitialized == 0) {
        g_dwVehicleViewModeIndex = 0;
        g_bPaintShopPlayer = g_dwMenuChoosingPlayer;
        g_dwNumActiveVehicles = 1;
        InitVehicleColor(&g_aVehicles[g_bPaintShopPlayer]);
        car.vehicle = &g_aVehicles[g_bPaintShopPlayer];
        car.vehicle->driver = UpdatePhysicsStep;
        n = g_bPaintShopPlayer;
        g_aVehicles[n].carModel = g_aVehicles[0].carModel;
        InstallVehicleModel(&g_aVehicles[n], n, g_aVehicles[n].carModel);
        g_pCurrentVehicleModelData = (VehicleModel *)g_aVehicles[g_bPaintShopPlayer].model;
        {
            Vec3f *p = &g_aVehicles[g_bPaintShopPlayer].frame.position;

            p->z = p->y = p->x = 0.0f;
        }
        {
            Vec3f *p = &g_aVehicles[g_bPaintShopPlayer].frame.up;

            p->z = 1.0f;
            p->y = p->x = 0.0f;
        }
#if TGR_REGION == TGR_REGION_JP
        /* Reuse the dead vehicle pointer storage to match the Japanese register allocation. */
        car.paletteStart = _paintShopPaletteSegmentRomStart;
#endif
        {
            Vec3f *p = &g_aVehicles[g_bPaintShopPlayer].frame.forward;

            p->y = 1.0f;
            p->z = p->x = 0.0f;
        }
        {
            Vec3f *p = &g_aVehicles[g_bPaintShopPlayer].frame.side;

            p->z = p->y = 0.0f;
            p->x = -1.0f;
        }
        if (g_aVehicles[g_bPaintShopPlayer].carModel == 10) {
            v = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].position;
            v->y = v->x = 7.3f;
            v->z = 2.0f;
        } else {
            v = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].position;
            v->y = v->x = 6.5f;
            v->z = 2.0f;
        }
        Vec3fNegate(&g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
                    &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].position);
        Vec3fNormalize(&g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
        v = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].up;
        v->y = v->x = 0.0f;
        v->z = 1.0f;
        Vec3fCross(&g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].side,
                   &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].up,
                   &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
        Vec3fCross(&g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].up,
                   &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].forward,
                   &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].side);
        g_aVehicles[g_bPaintShopPlayer].groundInstanceCount = 0;
        g_aVehicles[g_bPaintShopPlayer].nearbyInstanceCount = 0;
        ResetHeap();
#if TGR_REGION != TGR_REGION_JP
        osSyncPrintf("\nAllocating %d bytes (x2) for decal buffers...\n\n", 0x800);
#endif
        g_pDecalUndoBuffer = AllocHeapAligned(0x800);
        g_pDecalScratchTexels = AllocHeapAligned(0x800);
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texPaintWholeCar);
        g_texPaintWholeCar.style = SPRITE_STYLE_TINTED;
        LoadTextureFromRom(&g_texPanelLeftDoor);
        g_texPanelRightDoor.pixels = g_texPanelLeftDoor.pixels;
        LoadTextureFromRom(&g_texPanelLeftFrontFender);
        g_texPanelRightFrontFender.pixels = g_texPanelLeftFrontFender.pixels;
        LoadTextureFromRom(&g_texPanel4);
        LoadTextureFromRom(&g_texPanel5);
        LoadTextureFromRom(&g_texPanelLeftRearFender);
        g_texPanelRightRearFender.pixels = g_texPanelLeftRearFender.pixels;
        LoadTextureFromRom(&g_texPanel8);
        LoadTextureFromRom(&g_texPanel9);
        LoadTextureFromRom(&g_texPanel8Overlay);
        g_texPanel8Overlay.originX = g_texPanel8.originX;
        g_texPanel8Overlay.originY = g_texPanel8.originY;
        LoadTextureFromRom(&g_texPanel9Overlay);
        g_texPanel9Overlay.originX = g_texPanel9.originX;
        g_texPanel9Overlay.originY = g_texPanel9.originY + 8;
        for (i = 0; i < 10; i++) {
            switch (i) {
            case 0:
            case 1:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 5;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 2;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 10;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 4;
                break;
            case 2:
            case 3:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 4;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 1;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 9;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 4;
                break;
            case 4:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 6;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 4;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 12;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 8;
                break;
            case 5:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 4;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 2;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 8;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 4;
                break;
            case 6:
            case 7:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 2;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 2;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 6;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 4;
                break;
            case 8:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 2;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 2;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 4;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 4;
                break;
            case 9:
                g_aDecalSlotRects[i].x = g_apDecalSlotIcons[i]->originX + 4;
                g_aDecalSlotRects[i].y = g_apDecalSlotIcons[i]->originY + 2;
                g_aDecalSlotRects[i].width = g_apDecalSlotIcons[i]->width - 8;
                g_aDecalSlotRects[i].height = g_apDecalSlotIcons[i]->height - 4;
                break;
            default:
                break;
            }
            g_apDecalSlotIcons[i]->style = 99;
        }
        g_aPaintToolButtonRects[0].x = g_rcDecalCanvasFrame[0].x;
        g_aPaintToolButtonRects[0].y =
            g_rcDecalCanvasFrame[0].height + g_rcDecalCanvasFrame[0].y + 6;
        g_aPaintToolButtonRects[0].width = 0x20;
        g_aPaintToolButtonRects[0].height = 0x5e;
        g_rcPaletteFrameLeft.x = g_aPaintToolButtonRects[0].width + g_aPaintToolButtonRects->x + 2;
        g_rcPaletteFrameLeft.y = g_aPaintToolButtonRects[0].y;
        g_rcPaletteFrameRight.x = g_rcPaletteFrameLeft.x + g_rcPaletteFrameLeft.width + 0xe;
        g_rcPaletteFrameRight.y = g_aPaintToolButtonRects[0].y;
        if (r) {
        }
        y = g_rcPaletteFrameRight.y + g_rcPaletteFrameLeft.height + 2;
        for (r = 0; r < 3; r++) {
            for (i = 0; i < 4; i++) {
                n = (r << 2) + i;
                g_aPaintToolButtonRects[n + 1].x = g_rcPaletteFrameLeft.x + i * 0x4a;
                g_aPaintToolButtonRects[n + 1].width = 0x48;
                g_aPaintToolButtonRects[n + 1].height = 0x16;
                g_aPaintToolButtonRects[n + 1].y = y;
            }
            y += 0x18;
        }
        g_texBrushSizeRound.originY = g_aPaintToolButtonRects[0].y + 4;
        g_texBrushSizeSquare.originY = g_aPaintToolButtonRects[0].y + 4;
        g_texBrushSizeRound.originX = g_aPaintToolButtonRects[0].x;
        g_texBrushSizeSquare.originX = g_aPaintToolButtonRects[0].x;
        LoadTextureFromRom(&g_texBrushSizeRound);
        LoadTextureFromRom(&g_texBrushSizeSquare);
        for (i = 1; i < 13; i++) {
            g_apPaintToolIcons[i]->width = 0x40;
            g_apPaintToolIcons[i]->height = 0xe;
            g_apPaintToolIcons[i]->stripHeight = g_apPaintToolIcons[i]->height;
            g_apPaintToolIcons[i]->drawWidth = g_apPaintToolIcons[i]->width;
            g_apPaintToolIcons[i]->drawHeight = g_apPaintToolIcons[i]->height;
            g_apPaintToolIcons[i]->originX = g_aPaintToolButtonRects[i].x + 4;
            g_apPaintToolIcons[i]->originY = g_aPaintToolButtonRects[i].y + 4;
            g_apPaintToolIcons[i]->style = 0;
            LoadTextureFromRom(g_apPaintToolIcons[i]);
        }
#if TGR_REGION == TGR_REGION_JP
        DmaRead(g_awPaintShopButtonPalette,
                (u32)car.paletteStart,
                _paintShopPaletteSegmentRomEnd - car.paletteStart);
#endif
        LoadTextureFromRom(&g_texPaintCursor);
        LoadTextureFromRom(&g_texPaintFillCursor);
        LoadTextureFromRom(&g_texPaintBrushCursor);
        LoadTextureFromRom(&g_texPaintCrosshairCursor);
        LoadTextureFromRom(&g_texPaintTextCursor);
        LoadTextureFromRom(&g_texPaintEyedropperCursor);
        LoadTextureFromRom(&g_texKeyboardPanel);
        LoadTextureFromRom(&g_texKeyboardBackground);
        LoadTextureFromRom(&g_texKeyboardHighlight);
        LoadTextureFromRom(&g_texButtonA);
        LoadTextureFromRom(&g_texButtonB);
        LoadTextureFromRom(&g_texDecalFont);
        g_texKeyboardBackground.originX = g_texKeyboardPanel.originX + 8;
        g_texKeyboardBackground.originY = g_texKeyboardPanel.originY + 8;
        g_texKeyboardBackground.drawWidth = g_texKeyboardPanel.drawWidth - 16;
        g_texKeyboardBackground.drawHeight = g_texKeyboardPanel.drawHeight - 16;
        for (i = 0; i < 4; i++) {
            g_apRectStyleIcons[i]->bitsPerTexel = g_apOvalStyleIcons[i]->bitsPerTexel = 4;
            g_apRectStyleIcons[i]->width = g_apOvalStyleIcons[i]->width = 0x40;
            g_apRectStyleIcons[i]->height = g_apOvalStyleIcons[i]->height = 0x2a;
            g_apRectStyleIcons[i]->stripHeight = g_apOvalStyleIcons[i]->stripHeight =
                g_apRectStyleIcons[i]->height;
            g_apRectStyleIcons[i]->drawWidth = g_apOvalStyleIcons[i]->drawWidth =
                g_apRectStyleIcons[i]->width;
            g_apRectStyleIcons[i]->drawHeight = g_apOvalStyleIcons[i]->drawHeight =
                g_apRectStyleIcons[i]->height;
            LoadTextureFromRom(g_apRectStyleIcons[i]);
            LoadTextureFromRom(g_apOvalStyleIcons[i]);
        }
        g_pDecalTexels = g_pCurrentVehicleModelData
                             ->textures[g_pCurrentVehicleModelData->materialTextures[g_bDecalSlot]]
                             .image;
        g_pDecalMask = g_pCurrentVehicleModelData->decalMasks[g_bDecalSlot];
        g_bDecalWidth = g_pCurrentVehicleModelData
                            ->textures[g_pCurrentVehicleModelData->materialTextures[g_bDecalSlot]]
                            .width;
        g_bDecalHeight = g_pCurrentVehicleModelData
                             ->textures[g_pCurrentVehicleModelData->materialTextures[g_bDecalSlot]]
                             .height;
        g_pDecalPalette = g_pCurrentVehicleModelData
                              ->textures[g_pCurrentVehicleModelData->materialTextures[g_bDecalSlot]]
                              .palette;
        g_pDecalSlotIcon = g_apDecalSlotIcons[g_bDecalSlot];
        g_aBrushSizeButtonRects[0].x = g_texBrushSizeRound.originX;
        g_aBrushSizeButtonRects[0].y = g_texBrushSizeRound.originY;
        for (i = 1; i < 5; i++) {
            g_aBrushSizeButtonRects[i].x = g_texBrushSizeRound.originX;
            g_aBrushSizeButtonRects[i].y =
                g_aBrushSizeButtonRects[i - 1].y + g_aBrushSizeButtonRects[i - 1].height;
        }
        for (i = 0; i < 2; i++) {
            g_aDecalSwatches[i].rect.x = g_rcPaletteFrameLeft.x + 4 + i * 0x14;
            g_aDecalSwatches[i].rect.width = 0x14;
            g_aDecalSwatches[i].rect.height = 0xe;
            g_aDecalSwatches[i].rect.y = g_rcPaletteFrameLeft.y + 4;
        }
        for (i = 2; i < 16; i++) {
            g_aDecalSwatches[i].rect.width = 0x10;
            g_aDecalSwatches[i].rect.height = 0xe;
            g_aDecalSwatches[i].rect.x = g_rcPaletteFrameRight.x + 4 + (i - 2) * 0x10;
            g_aDecalSwatches[i].rect.y = g_rcDecalCanvasFrame[2].y + 4;
        }
        ConvertPaletteToRgb();
        for (r = 0; r < 5; r++) {
            for (i = 0; i < 10; i++) {
                k = r * 10 + i;
                g_aDecalGlyphs[k].x =
                    g_texKeyboardBackground.originX + (g_texKeyboardHighlight.drawWidth + 4) * i;
                g_aDecalGlyphs[k].y =
                    g_texKeyboardBackground.originY + (g_texKeyboardHighlight.drawHeight + 4) * r;
            }
        }
        g_rcDecalCanvas.width = g_bDecalWidth * 4;
        g_rcDecalCanvas.height = g_bDecalHeight * 4;
        dx = (g_rcDecalCanvasFrame[0].width - g_rcDecalCanvas.width) >> 1;
        dy = (g_rcDecalCanvasFrame[0].height - g_rcDecalCanvas.height) >> 1;
        g_rcDecalCanvas.x = g_rcDecalCanvasFrame[0].x + dx;
        g_rcDecalCanvas.y = g_rcDecalCanvasFrame[0].y + dy;
        g_bPaintShopInitialized = 1;
        gControllerPakState = 0;
        SetFadeTarget(1.0f, 0.2f);
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix((float)g_dwScreenWidth, (float)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 0x71, 0xb, 0x40);
    DrawSpriteAtOrigin(WITH_PALETTE(&g_texLogo, NULL));
#if TGR_REGION == TGR_REGION_JP
    SetTextSize(0x1e);
#else
    SetTextSize(0x22);
#endif
    SetTextAlignCenter();
    SetTextStyleNormal();
#if TGR_REGION == TGR_REGION_JP
    DrawText("%ryPAINT SHOP", 0xa0, 0x25);
#else
    DrawText("%ryPAINT SHOP", 0xa0, 0x26);
#endif
    for (i = 0; i < 10; i++) {
        if (i == g_bDecalSlot) {
            DrawSpriteTinted(g_apDecalSlotIcons[i], 0x20, 200, 0xff);
        } else {
            DrawSpriteTinted(g_apDecalSlotIcons[i], 0x20, 0x10, 0xff);
        }
    }
    DrawSpriteTinted(&g_texPanel8Overlay, 0xff, 0xff, 0xff);
    DrawSpriteTinted(&g_texPanel9Overlay, 0xc0, 0, 0);
    DrawFilledRectHiRes(area->x, area->y, area->width, area->height, 0, 0, 0);
    DrawTexturedTile(g_pCurrentVehicleModelData->materialTextures[g_bDecalSlot],
                     g_pCurrentVehicleModelData->textures);
    EmitScaledTextureRect(g_bDecalWidth,
                          g_bDecalHeight,
                          g_rcDecalCanvas.x,
                          g_rcDecalCanvas.y,
                          g_rcDecalCanvas.width,
                          g_rcDecalCanvas.height);
    DrawDecalColorPalette();
    DrawFilledRectHiRes(g_aPaintToolButtonRects[0].x,
                        g_aPaintToolButtonRects[0].y,
                        g_aPaintToolButtonRects[0].width,
                        g_aPaintToolButtonRects[0].height,
                        0,
                        0,
                        0);
    DrawFilledRectHiRes(g_aPaintToolButtonRects[0].x + 4,
                        g_aPaintToolButtonRects[0].y + 4,
                        g_aPaintToolButtonRects[0].width - 8,
                        g_aPaintToolButtonRects[0].height - 8,
                        0xff,
                        0xca,
                        0);
    if (g_texPaintToolBrush.style == 0) {
        DrawSpriteAtOrigin(WITH_PALETTE(&g_texBrushSizeRound, NULL));
    } else {
        DrawSpriteAtOrigin(WITH_PALETTE(&g_texBrushSizeSquare, NULL));
    }
    DrawFilledRectHiRes(g_aBrushSizeButtonRects[g_bBrushSize].x,
                        g_aBrushSizeButtonRects[g_bBrushSize].y,
                        g_aBrushSizeButtonRects[g_bBrushSize].width,
                        g_aBrushSizeButtonRects[g_bBrushSize].height,
                        0,
                        0,
                        0);
    if (g_texPaintToolBrush.style == 0) {
        DrawScaledTexturedRect(&g_texBrushSizeRound,
                               0,
                               g_texBrushSizeRound.originY + g_texBrushSizeRound.height -
                                   g_aBrushSizeButtonRects[g_bBrushSize].y -
                                   g_aBrushSizeButtonRects[g_bBrushSize].height,
                               g_texBrushSizeRound.width,
                               g_aBrushSizeButtonRects[g_bBrushSize].height,
                               g_texBrushSizeRound.originX - 1,
                               g_aBrushSizeButtonRects[g_bBrushSize].y,
                               g_texBrushSizeRound.width,
                               g_aBrushSizeButtonRects[g_bBrushSize].height,
                               0xff,
                               0xca,
                               0);
    } else {
        DrawScaledTexturedRect(&g_texBrushSizeSquare,
                               0,
                               g_texBrushSizeSquare.originY + g_texBrushSizeSquare.height -
                                   g_aBrushSizeButtonRects[g_bBrushSize].y -
                                   g_aBrushSizeButtonRects[g_bBrushSize].height,
                               g_texBrushSizeSquare.width,
                               g_aBrushSizeButtonRects[g_bBrushSize].height,
                               g_texBrushSizeSquare.originX - 1,
                               g_aBrushSizeButtonRects[g_bBrushSize].y,
                               g_texBrushSizeSquare.width,
                               g_aBrushSizeButtonRects[g_bBrushSize].height,
                               0xff,
                               0xca,
                               0);
    }
    for (i = 1; i < 13; i++) {
        if (i == g_bPaintTool) {
            DrawFilledRectHiRes(g_aPaintToolButtonRects[i].x,
                                g_aPaintToolButtonRects[i].y,
                                g_aPaintToolButtonRects[i].width,
                                g_aPaintToolButtonRects[i].height,
                                0x20,
                                200,
                                0xff);
        } else {
            DrawFilledRectHiRes(g_aPaintToolButtonRects[i].x,
                                g_aPaintToolButtonRects[i].y,
                                g_aPaintToolButtonRects[i].width,
                                g_aPaintToolButtonRects[i].height,
                                0,
                                0,
                                0);
        }
        DrawFilledRectHiRes(g_aPaintToolButtonRects[i].x + 4,
                            g_aPaintToolButtonRects[i].y + 4,
                            g_aPaintToolButtonRects[i].width - 8,
                            g_aPaintToolButtonRects[i].height - 8,
                            0xff,
                            0xca,
                            0);
#if TGR_REGION == TGR_REGION_JP
        DrawSpriteAtOrigin(g_apPaintToolIcons[i], g_awPaintShopButtonPalette);
#else
        DrawSpriteAtOrigin(WITH_PALETTE(g_apPaintToolIcons[i], NULL));
#endif
    }
    if (g_bShowPreview3D != 0) {
        g_dwNightMode = 0;
        SetupFog();
        SetupWeatherLights();
        RenderDecalPreview3D();
        SetupProjectionMatrix((float)g_dwScreenWidth, (float)g_dwScreenHeight);
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    }
    if (IsCursorInRect(&g_rcDecalCanvas)) {
        ProcessDecalEditorInput();
        UpdateCursorFromInput();
        switch (g_bPaintTool) {
        case 1:
            if (g_bEyedropperActive != 0) {
                DrawTexturedSprite(&g_texPaintEyedropperCursor,
                                   g_texPaintCursor.originX + 1,
                                   g_texPaintCursor.originY - 0x18);
            } else {
                DrawTexturedSprite(&g_texPaintBrushCursor,
                                   g_texPaintCursor.originX + 1,
                                   g_texPaintCursor.originY - 0x18);
            }
            break;
        case 3:
        case 5:
        case 6:
            if (g_bEyedropperActive != 0) {
                DrawTexturedSprite(&g_texPaintEyedropperCursor,
                                   g_texPaintCursor.originX,
                                   g_texPaintCursor.originY - 0x18);
            } else {
                DrawTexturedSprite(&g_texPaintCrosshairCursor,
                                   g_texPaintCursor.originX - 8,
                                   g_texPaintCursor.originY - 8);
            }
            break;
        case 4:
            if (g_bDecalDragActive == 0) {
                if (g_bEyedropperActive != 0) {
                    DrawTexturedSprite(&g_texPaintEyedropperCursor,
                                       g_texPaintCursor.originX,
                                       g_texPaintCursor.originY - 0x18);
                } else {
                    DrawTexturedSprite(&g_texPaintTextCursor,
                                       g_texPaintCursor.originX -
                                           (g_texPaintTextCursor.drawWidth >> 1),
                                       g_texPaintCursor.originY - g_texPaintTextCursor.drawHeight);
                }
            } else {
                DrawSpriteAtOrigin(WITH_PALETTE(&g_texPaintCursor, NULL));
            }
            break;
        case 2:
            if (g_bEyedropperActive != 0) {
                DrawTexturedSprite(&g_texPaintEyedropperCursor,
                                   g_texPaintCursor.originX,
                                   g_texPaintCursor.originY - 0x18);
            } else {
                DrawTexturedSprite(&g_texPaintFillCursor,
                                   g_texPaintCursor.originX - 6,
                                   g_texPaintCursor.originY - 0x17);
            }
            break;
        default:
            DrawSpriteAtOrigin(WITH_PALETTE(&g_texPaintCursor, NULL));
            break;
        }
    } else {
        switch (g_bPaintTool) {
        case 7:
            ProcessMirrorFlipMenu();
            break;
        case 8:
            ProcessFillClearMenu();
            break;
        case 9:
            if (gControllerPakState == 0) {
                gControllerPakState = ProcessDecalSaveLoad(
                    g_pCurrentVehicleModelData, g_bPaintShopPlayer, 9, 0, &done);
            } else {
                DrawPakErrorMessage(gControllerPakState, 9, 0);
                if (g_aPlayerInputState[g_bPaintShopPlayer].buttons & 0x30) {
                    TransferFlags(&g_aPlayerInputState[g_bPaintShopPlayer], 0x30);
                    gControllerPakState = 0;
                    g_bPaintTool = g_bPreviousPaintTool;
                }
            }
            break;
        case 10:
            if (gControllerPakState == 0) {
                gControllerPakState = ProcessDecalSaveLoad(
                    g_pCurrentVehicleModelData, g_bPaintShopPlayer, 10, 0, &done);
            } else {
                DrawPakErrorMessage(gControllerPakState, 10, 0);
                if (g_aPlayerInputState[g_bPaintShopPlayer].buttons & 0x30) {
                    TransferFlags(&g_aPlayerInputState[g_bPaintShopPlayer], 0x30);
                    gControllerPakState = 0;
                    g_bPaintTool = g_bPreviousPaintTool;
                }
            }
            break;
        case 12:
            ProcessPaintShopExitConfirm();
            break;
        default:
            if (g_bStyleMenuOpen != 0) {
                if (g_bPaintTool == 1) {
                    ProcessBrushStyleMenu();
                } else if (g_bPaintTool == 4) {
                    ProcessTextStyleMenu();
                } else if (g_bPaintTool == 5) {
                    ProcessRectStyleMenu();
                } else if (g_bPaintTool == 6) {
                    ProcessOvalStyleMenu();
                }
            } else if (g_bColorEditorOpen != 0) {
                ProcessColorEditorDialog();
            } else {
                ProcessDecalEditorInput();
                UpdateCursorFromInput();
                DrawSpriteAtOrigin(WITH_PALETTE(&g_texPaintCursor, NULL));
            }
            break;
        }
    }
    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    if (g_dwHudDebugLevel > 1) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
        DrawTimingBars();
    }
    FlushDisplayList();
    if (g_aPlayerInputState[g_bPaintShopPlayer].buttons & 0x20) {
        if (IsCursorInRect(&g_rcDecalCanvas) && g_bPaintToolStarted == 0 &&
            g_bDecalDragActive == 0) {
            i = g_texPaintCursor.originX;
            r = g_texPaintCursor.originY;
            i = (i - g_rcDecalCanvas.x) >> 2;
            r = (g_rcDecalCanvas.y + g_rcDecalCanvas.height - r) >> 2;
            g_bDecalColor = GetPixel4Bit(i, r);
            g_bEyedropperActive = 1;
        } else if (g_bPaintToolStarted != 0 || g_bDecalDragActive != 0) {
            TransferFlags(&g_aPlayerInputState[g_bPaintShopPlayer], 0x20);
            g_bDecalDragActive = 0;
            g_bPaintToolStarted = 0;
            g_bShowPreview3D = 1;
            g_bPaletteEnabled = 1;
            g_bDecalTextLength = 0;
            g_bDecalTextWidth = 0;
        }
    } else {
        g_bEyedropperActive = 0;
    }
    if (g_bPaintShopExiting != 0 && IsFadedToBlack()) {
        g_bPaintShopExiting = 0;
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        ResetPaintShopState();
        g_bPaintShopInitialized = 0;
        gControllerPakState = 0;
        g_bPaintShopPreviewActive = 0;
        g_wSerialInterfaceIdle = 1;
        SetCurrentScreenCallback(MainMenuEntry);
    }
}

void ResetPaintShopState(void) {
    g_texPaintCursor.originX = CURSOR_START_X;
    g_texPaintCursor.originY = CURSOR_START_Y;
    g_bDecalSlot = DEFAULT_DECAL_SLOT;
    g_bPreviousDecalSlot = g_bDecalSlot;
    g_bUndoDecalSlot = g_bDecalSlot;
    g_pDecalSlotIcon = g_apDecalSlotIcons[g_bDecalSlot];
    g_bBrushSize = DEFAULT_BRUSH_SIZE;
    g_bSavedColor = 0;
    g_bDecalColor = DEFAULT_DECAL_COLOR;
    g_bPreviousPaintTool = PAINT_TOOL_BRUSH;
    g_bPaintTool = PAINT_TOOL_BRUSH;
    g_wScreenFrameCount = 0;
    g_texPaintToolRect.style = 0;
    g_texPaintToolOval.style = 0;
    g_bDecalEditCount = 0;
    g_bPaintToolStarted = FALSE;
    g_bDecalDragActive = FALSE;
    g_bPaletteEnabled = TRUE;
    g_bDecalSlotHasMirror = TRUE;
    g_bDecalSlotChanged = FALSE;
    g_bColorEditorOpen = FALSE;
    g_bShowPreview3D = TRUE;
    g_bUndoAvailable = FALSE;
    g_bStyleMenuOpen = FALSE;
    g_bTextShadowPickerOpen = FALSE;
    g_bEyedropperActive = FALSE;
}

void SaveDecalUndo(void) {
    memcpy(g_pDecalUndoBuffer, g_pDecalTexels, DECAL_BUFFER_SIZE);
    g_bUndoDecalSlot = g_bDecalSlot;
    g_bDecalEditCount++;
    g_bUndoAvailable = TRUE;
}

void UndoDecalEdit(void) {
    if (g_bUndoAvailable) {
        memcpy(g_pCurrentVehicleModelData
                   ->textures[g_pCurrentVehicleModelData->materialTextures[g_bUndoDecalSlot]]
                   .image,
               g_pDecalUndoBuffer,
               DECAL_BUFFER_SIZE);
        g_bDecalEditCount--;
        g_bUndoAvailable = FALSE;
    }
}

/* Draw a grey dialog box with a raised border. */
static inline void DrawDialogBox(s32 x, s32 y, s32 width, s32 height) {
    DrawRectBorder(x, y, width, height, 3, FALSE, 0, 0x80, 0x80, 0x80);
}

/* Draw a sunken frame without filling it. */
static inline void DrawSunkenFrame(s32 x, s32 y, s32 width, s32 height) {
    DrawRectBorder(x, y, width, height, 1, TRUE, 1, 0x80, 0x80, 0x80);
}

/* Set the colors of the text of the dialogs. The second color differs on the car select screen. */
static inline void SetPakTextColors(u8 inCarSelect) {
    if (inCarSelect == 1) {
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    } else {
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
    }
}

#if TGR_REGION == TGR_REGION_JP
void DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect) {
    s32 x, y, width, textY;

    SetTextSize(14);
    SetPakTextColors(inCarSelect);
    SetTextAlignLeft();
    SetTextStyleNormal();
    /* The widths test for a save and the texts for a load. */
    switch (error) {
    case PFS_ERR_NOPACK:
    case PFS_ERR_DEVICE:
        y = inCarSelect == 1 ? 0xEA : 0xCC;
        DrawDialogBox(0xCC, y, 0xE8, 0x48);
        textY = (y + 30) >> 1;
        DrawText("%jコントローラパックが", 0x6E, textY);
        DrawText("%jありません。", 0x6E, textY + 14);
        break;
    case PFS_DATA_FULL:
    case PFS_DIR_FULL:
        y = inCarSelect == 1 ? 0xDB : 0xAF;
        DrawDialogBox(0xB9, y, 0x10E, 0x82);
        textY = (y + 30) >> 1;
        DrawText("%jコントローラパックに", 0x64, textY);
        DrawText("%j空きページが足りません。", 0x64, textY + 12);
        DrawText("%jデカールのセーブには", 0x64, textY + 30);
        DrawText("62%jページ必要です。", 0x64, textY + 42);
        break;
    case PFS_ERR_INCONSISTENT:
        width = operation == PAINT_TOOL_SAVE ? 0x106 : 0x116;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%jコントローラパック", (x + 16) >> 1, textY);
            DrawText("%jから読み込みができません。", (x + 16) >> 1, textY + 14);
        } else {
            DrawText("%jコントローラパック", (x + 16) >> 1, textY);
            DrawText("%jへ書き込みができません。", (x + 16) >> 1, textY + 14);
        }
        break;
    case PFS_ERR_INVALID:
        width = operation == PAINT_TOOL_SAVE ? 0x10C : 0xF2;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%jコントローラパックに", (x + 16) >> 1, textY);
            DrawText("%jデカールがありません。", (x + 16) >> 1, textY + 14);
        } else {
            DrawText("%jデカールセーブ中", (x + 16) >> 1, textY);
            DrawText("%jにエラーが発生しました。", (x + 16) >> 1, textY + 14);
        }
        break;
    case PAK_ERROR_TRANSFER:
        width = operation == PAINT_TOOL_SAVE ? 0x10B : 0x114;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%jデカールロード中", (x + 15) >> 1, textY);
            DrawText("%jにエラーが発生しました。", (x + 15) >> 1, textY + 14);
        } else {
            DrawText("%jデカールセーブ中", (x + 15) >> 1, textY);
            DrawText("%jにエラーが発生しました。", (x + 15) >> 1, textY + 14);
        }
        break;
    case PFS_ERR_BAD_DATA:
        y = inCarSelect == 1 ? 0xE4 : 0xCC;
        DrawDialogBox(0xB6, y, 0x114, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%jデカールロード中に異常", 0x62, textY);
            DrawText("%jデータが発生しました。", 0x62, textY + 14);
        } else {
            DrawText("%jデカールセーブ中に異常", 0x62, textY);
            DrawText("%jデータが発生しました。", 0x62, textY + 14);
        }
        break;
    case PFS_ERR_CONTRFAIL:
        y = inCarSelect == 1 ? 0xDC : 0xB0;
        DrawDialogBox(0xCD, y, 0xE6, 0x80);
        textY = (y + 30) >> 1;
        DrawText("%jコントローラパックが", 0x6D, textY);
        DrawText("%j異常です。", 0x6D, textY + 12);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%jデカールを", 0x6D, textY + 30);
            DrawText("%jロードできません。", 0x6D, textY + 42);
        } else {
            DrawText("%jデカールを", 0x6D, textY + 30);
            DrawText("%jセーブできません。", 0x6D, textY + 42);
        }
        break;
    case PFS_ERR_ID_FATAL:
        y = inCarSelect == 1 ? 0xF8 : 0xCC;
        DrawDialogBox(0x50, y, 0x1E0, 0x48);
        textY = (y + 30) >> 1;
        DrawText("%jコントローラパックが、こわれています。", 0x30, textY);
        DrawText("%jあたらしいコントローラパックを、さして下さい。", 0x30, textY + 14);
        break;
    }
}
#else
void DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect) {
    s32 x, y, width, textY;

    SetTextSize(12);
    SetPakTextColors(inCarSelect);
    SetTextAlignLeft();
    SetTextStyleNormal();
    /* The widths test for a save and the texts for a load. */
    switch (error) {
    case PFS_ERR_NOPACK:
    case PFS_ERR_DEVICE:
        y = inCarSelect == 1 ? 0xEA : 0xCC;
        DrawDialogBox(0xD5, y, 0xD5, 0x48);
        textY = (y + 30) >> 1;
        DrawText("CONTROLLER PAK", 0x72, textY);
        DrawText("IS NOT INSERTED", 0x72, textY + 14);
        break;
    case PFS_DATA_FULL:
    case PFS_DIR_FULL:
        y = inCarSelect == 1 ? 0xC3 : 0x97;
        DrawDialogBox(0xA5, y, 0x135, 0xB2);
        textY = (y + 30) >> 1;
        DrawText("INSUFFICIENT FREE PAGES", 0x5A, textY);
        DrawText("OR FREE NOTES IN THE", 0x5A, textY + 12);
        DrawText("CONTROLLER PAK.", 0x5A, textY + 24);
        DrawText("62 PAGES AND ONE NOTE", 0x5A, textY + 42);
        DrawText("ARE NEEDED TO SAVE THE", 0x5A, textY + 54);
        DrawText("CUSTOM DECALS.", 0x5A, textY + 66);
        break;
    case PFS_ERR_INCONSISTENT:
        width = operation == PAINT_TOOL_SAVE ? 0x106 : 0x116;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("UNABLE TO READ FROM", (x + 16) >> 1, textY);
        } else {
            DrawText("UNABLE TO WRITE TO", (x + 16) >> 1, textY);
        }
        DrawText("THE CONTROLLER PAK", (x + 16) >> 1, textY + 14);
        break;
    case PFS_ERR_INVALID:
        width = operation == PAINT_TOOL_SAVE ? 0x10C : 0xF2;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("DECALS NOT FOUND", (x + 16) >> 1, textY);
            DrawText("IN CONTROLLER PAK", (x + 16) >> 1, textY + 14);
        } else {
            DrawText("ERROR ENCOUNTERED", (x + 16) >> 1, textY);
            DrawText("WHILE SAVING DECALS", (x + 16) >> 1, textY + 14);
        }
        break;
    case PAK_ERROR_TRANSFER:
        width = operation == PAINT_TOOL_SAVE ? 0x10B : 0x114;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        DrawText("ERROR ENCOUNTERED", (x + 15) >> 1, textY);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("WHILE LOADING DECALS", (x + 15) >> 1, textY + 14);
        } else {
            DrawText("WHILE SAVING DECALS", (x + 15) >> 1, textY + 14);
        }
        break;
    case PFS_ERR_BAD_DATA:
        y = inCarSelect == 1 ? 0xE4 : 0xCC;
        DrawDialogBox(0xAD, y, 0x126, 0x48);
        textY = (y + 30) >> 1;
        DrawText("BAD DATA ENCOUNTERED", 0x5D, textY);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("WHILE LOADING DECALS", 0x5D, textY + 14);
        } else {
            DrawText("WHILE SAVING DECALS", 0x5D, textY + 14);
        }
        break;
    case PFS_ERR_CONTRFAIL:
        y = inCarSelect == 1 ? 0xDC : 0xB0;
        DrawDialogBox(0xAE, y, 0x124, 0x80);
        textY = (y + 30) >> 1;
        DrawText("CONTROLLER ERROR HAS", 0x5E, textY);
        DrawText("BEEN DETECTED.", 0x5E, textY + 12);
        DrawText("CUSTOM DECALS CANNOT", 0x5E, textY + 30);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("BE LOADED.", 0x5E, textY + 42);
        } else {
            DrawText("BE SAVED.", 0x5E, textY + 42);
        }
        break;
    case PFS_ERR_ID_FATAL:
        y = inCarSelect == 1 ? 0xF8 : 0xCC;
        DrawDialogBox(0xCD, y, 0xE6, 0x48);
        textY = (y + 30) >> 1;
        DrawText("CONTROLLER PAK", 0x6E, textY);
        DrawText("IS NONFUNCTIONAL", 0x6E, textY + 14);
        break;
    }
}
#endif /* TGR_REGION == TGR_REGION_JP */

void DrawRectBorder(s32 x,
                    s32 y,
                    s32 width,
                    s32 height,
                    s32 border,
                    u8 transparent,
                    u8 sunken,
                    u8 red,
                    u8 green,
                    u8 blue) {
    s32 i;
    u8 light = 0xE0;
    u8 dark = 0x30;

    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
        border >>= 1;
    }
    if (border < 2) {
        border = 1;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    if (!transparent) {
        gDPSetFillColor(gDisplayListPtr++,
                        GPACK_RGBA5551(red, green, blue, 1) << 16 |
                            GPACK_RGBA5551(red, green, blue, 1));
        gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
        gDPPipeSync(gDisplayListPtr++);
    }
    if (sunken == 1) {
        SwapBytes(&light, &dark);
    }
    gDPSetFillColor(gDisplayListPtr++,
                    GPACK_RGBA5551(light, light, light, 1) << 16 |
                        GPACK_RGBA5551(light, light, light, 1));
    for (i = 1; i <= border; i++) {
        gDPFillRectangle(gDisplayListPtr++, x - i, y - i, x - i + 1, y + height + i);
    }
    for (i = 1; i <= border; i++) {
        gDPFillRectangle(gDisplayListPtr++, x - i, y - i, x + width + i, y - i + 1);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++,
                    GPACK_RGBA5551(dark, dark, dark, 1) << 16 |
                        GPACK_RGBA5551(dark, dark, dark, 1));
    for (i = 1; i <= border; i++) {
        gDPFillRectangle(
            gDisplayListPtr++, x + width + i - 1, y - i, x + width + i, y + height + i);
    }
    for (i = 1; i <= border; i++) {
        gDPFillRectangle(
            gDisplayListPtr++, x - i, y + height + i - 1, x + width + i, y + height + i);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

/* Draw the labels and the pictures of the A and B buttons of a menu of a tool. */
static inline void DrawMenuButtons(s32 buttonY, s32 cancelX) {
    s32 textY;

    SetTextSize(10);
    SetTextAlignLeft();
    textY = (buttonY + 0x12) >> 1;
    DrawText("%wwSELECT", (g_texButtonA.width + 0xD9) >> 1, textY);
    DrawText("%wwCANCEL", (cancelX + g_texButtonB.width + 6) >> 1, textY);
    DrawTexturedSprite(&g_texButtonA, 0xD3, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
}

/* Paint every texel of the decal selected, in the color painted with when the choice of the menu
 * is the one given, and in the color of the body otherwise. */
static inline void ClearDecal(u8 coloredOption) {
    s32 x, y;

    for (y = 0; y < g_bDecalHeight; y++) {
        for (x = 0; x < g_bDecalWidth; x++) {
            if (g_texPaintToolClear.style == coloredOption) {
                SetPixel4Bit(x, y, g_bDecalColor);
            } else {
                SetPixel4Bit(x, y, 0);
            }
        }
    }
}

void ProcessFillClearMenu(void) {
#if TGR_REGION == TGR_REGION_JP
    /* The two lines of the Japanese text of each option. */
    const char *optionTexts[CLEAR_OPTIONS][2] = {
        {"%j選んだカラーで", "  %jこのデカールを消します。"},
        {"%jボディカラーで", "  %jこのデカールを消します。"},
        {"%j選んだカラーで", "  %j全てのデカールを消します。"},
        {"%jボディカラーで", "  %j全てのデカールを消します。"},
    };
#else
    const char *scopeTexts[2] = {"CLEAR CURRENT DECAL", "CLEAR ALL DECALS"};
    const char *colorTexts[2] = {"TO SELECTED COLOR", "TO BODY COLOR"};
#endif /* TGR_REGION == TGR_REGION_JP */
    ScreenRect options[CLEAR_OPTIONS];
    s32 buttonY = 0x160 - g_texButtonA.height;
    s32 cancelX = 0x161 - g_texButtonB.width;
    s32 i, x, slotX, slotY, carX, carY;
    Sprite *slot;
    Sprite *car;
    PlayerInputState *input;

    for (i = 0, x = 0xB9; x < 0x1D1; i++, x += 0x46) {
        SetScreenRect(&options[i], x, 0xAC, 0x3C, 0x4E);
    }
    DrawDialogBox(0xA7, 0x78, 0x132, 0xF0);
    for (i = 0; i < CLEAR_OPTIONS; i++) {
        if (i == g_texPaintToolClear.style) {
            DrawRectOutline(
                options[i].x, options[i].y, options[i].width, options[i].height, 1, 0, 0xFF, 0xFF);
        } else {
            DrawRectOutline(options[i].x,
                            options[i].y,
                            options[i].width,
                            options[i].height,
                            1,
                            0x20,
                            0x20,
                            0x20);
        }
    }
    /* The slot is shown at half its size, centred by half its size again. */
    slot = g_apDecalSlotIcons[g_bDecalSlot];
    car = &g_texPaintWholeCar;
    slotX = (options[0].width - (slot->width >> 1)) >> 1;
    slotY = (options[0].height - (slot->height >> 1)) >> 1;
    carX = (options[2].width - car->width) >> 1;
    carY = (options[2].height - car->height) >> 1;
    DrawSpriteRegion(slot,
                     options[0].x + slotX,
                     options[0].y + slotY,
                     slot->drawWidth >> 1,
                     slot->drawHeight >> 1,
                     g_aDecalSwatches[g_bDecalColor].color[0],
                     g_aDecalSwatches[g_bDecalColor].color[1],
                     g_aDecalSwatches[g_bDecalColor].color[2]);
    DrawSpriteRegion(car,
                     options[2].x + carX,
                     options[2].y + carY,
                     car->drawWidth,
                     car->drawHeight,
                     g_aDecalSwatches[g_bDecalColor].color[0],
                     g_aDecalSwatches[g_bDecalColor].color[1],
                     g_aDecalSwatches[g_bDecalColor].color[2]);
    slot = g_apDecalSlotIcons[g_bDecalSlot];
    DrawSpriteRegion(slot,
                     options[1].x + slotX,
                     options[1].y + slotY,
                     slot->drawWidth >> 1,
                     slot->drawHeight >> 1,
                     g_aDecalSwatches[0].color[0],
                     g_aDecalSwatches[0].color[1],
                     g_aDecalSwatches[0].color[2]);
    DrawSpriteRegion(car,
                     options[3].x + carX,
                     options[3].y + carY,
                     car->drawWidth,
                     car->drawHeight,
                     g_aDecalSwatches[0].color[0],
                     g_aDecalSwatches[0].color[1],
                     g_aDecalSwatches[0].color[2]);
    SetTextAlignCenter();
    SetTextStyleNormal();
    SetTextSize(16);
    DrawText("%rySELECT OPTION", 0x9E, 0x4E);
    DrawSunkenFrame(0xBA, options[0].y + options[0].height + 12, 0x10C, 0x3A);
#if TGR_REGION == TGR_REGION_JP
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    SetTextSize(14);
    DrawText(optionTexts[g_texPaintToolClear.style][0], 0x9E, 0x8E);
    DrawText(optionTexts[g_texPaintToolClear.style][1], 0x9E, 0x9B);
#else
    SetTextSize(11);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    DrawText(scopeTexts[g_texPaintToolClear.style >> 1], 0x9E, 0x8E);
    DrawText(colorTexts[g_texPaintToolClear.style & CLEAR_TO_BODY_COLOR], 0x9E, 0x9B);
#endif /* TGR_REGION == TGR_REGION_JP */
    DrawMenuButtons(buttonY, cancelX);
    UpdateStickRepeat(&g_aPlayerInputState[g_bPaintShopPlayer]);
    input = &g_aPlayerInputState[g_bPaintShopPlayer];
    if (input->buttons & INPUT_LEFT) {
        TransferFlags(input, INPUT_LEFT);
        if (g_texPaintToolClear.style == 0) {
            g_texPaintToolClear.style = CLEAR_OPTIONS - 1;
        } else {
            g_texPaintToolClear.style--;
        }
    } else if (input->buttons & INPUT_RIGHT) {
        TransferFlags(input, INPUT_RIGHT);
        if (g_texPaintToolClear.style == CLEAR_OPTIONS - 1) {
            g_texPaintToolClear.style = 0;
        } else {
            g_texPaintToolClear.style++;
        }
    }
    input = &g_aPlayerInputState[g_bPaintShopPlayer];
    if (input->buttons & INPUT_A) {
        TransferFlags(input, INPUT_A);
        switch (g_texPaintToolClear.style) {
        case 0:
        case CLEAR_TO_BODY_COLOR:
            SaveDecalUndo();
            ClearDecal(0);
            g_bPaintTool = g_bPreviousPaintTool;
            break;
        case CLEAR_ALL_DECALS:
        case CLEAR_ALL_DECALS | CLEAR_TO_BODY_COLOR:
            g_bSavedDecalSlot = g_bDecalSlot;
            /* The palette and the button of the slot are not switched with its texels. */
            for (i = 0; i < DECAL_SLOTS; i++) {
                SelectDecalTexels(g_pCurrentVehicleModelData, i);
                ClearDecal(CLEAR_ALL_DECALS);
            }
            g_bDecalSlot = g_bSavedDecalSlot;
            SelectDecalTexels(g_pCurrentVehicleModelData, g_bDecalSlot);
            /* The undo buffer is saved after the clear. Undo cannot bring the decal back. */
            SaveDecalUndo();
            g_texPaintToolClear.style = 0;
            g_bPaintTool = g_bPreviousPaintTool;
            break;
        default:
            g_texPaintToolClear.style = 0;
            g_bPaintTool = g_bPreviousPaintTool;
            break;
        }
    } else if (input->buttons & INPUT_B) {
        TransferFlags(input, INPUT_B);
        g_texPaintToolClear.style = 0;
        g_bPaintTool = g_bPreviousPaintTool;
    }
}

void ProcessMirrorFlipMenu(void) {
#if TGR_REGION == TGR_REGION_JP
    /* The two lines of the Japanese text of each flip, and of each slot a decal mirrors from. */
    const char *flipTexts[2][2] = {
        {"%jこのデカールを", " %j左右反転します。"},
        {"%jこのデカールを", " %j上下反転します。"},
    };
    const char *mirrorTexts[MIRROR_TEXT_SLOTS][2] = {
        {"%jこのデカールを", " %j右ドアに使用します。"},
        {"%jこのデカールを", " %j左ドアに使用します。"},
        {"%jこのデカールを右フロント", " %jフェンダーに使用します。"},
        {"%jこのデカールを左フロント", " %jフェンダーに使用します。"},
        {NULL, NULL},
        {NULL, NULL},
        {"%jこのデカールを右リア", " %jフェンダーに使用します。"},
        {"%jこのデカールを左リア", " %jフェンダーに使用します。"},
    };
#else
    const char *flipTexts[2] = {"FLIP CURRENT DECAL", "FLIP CURRENT DECAL"};
    const char *axisTexts[2] = {"HORIZONTALLY", "VERTICALLY"};
#endif /* TGR_REGION == TGR_REGION_JP */
#if TGR_REGION == TGR_REGION_EU
    /* The two lines of the text of each slot a decal mirrors from. */
    const char *mirrorTexts[MIRROR_TEXT_SLOTS][2] = {
        {"MIRROR CURRENT DECAL", "TO THE RIGHT DOOR"},
        {"MIRROR CURRENT DECAL", "TO THE LEFT DOOR"},
        {"MIRROR CURRENT DECAL TO", "THE RIGHT FRONT FENDER"},
        {"MIRROR CURRENT DECAL TO", "THE LEFT FRONT FENDER"},
        {NULL, NULL},
        {NULL, NULL},
        {"MIRROR CURRENT DECAL TO", "THE RIGHT REAR FENDER"},
        {"MIRROR CURRENT DECAL TO", "THE LEFT REAR FENDER"},
    };
#endif /* TGR_REGION == TGR_REGION_EU */
    ScreenRect options[MIRROR_OPTIONS];
    /* The row of the buttons is placed by the width of the A button. */
    s32 buttonY = 0x155 - g_texButtonA.width;
    s32 cancelX = 0x163 - g_texButtonB.width;
    s32 i;
    PlayerInputState *input;

    if (g_bPaintToolStarted) {
        g_bStyleMenuCursor = g_texPaintToolMirror.style;
        g_bPaintToolStarted = FALSE;
    }
    SetScreenRect(&options[0], 0xAB, 0xBF, 0x66, 0x2C);
    for (i = 1; i < MIRROR_OPTIONS; i++) {
        SetScreenRect(&options[i],
                      options[0].x + options[0].width + 98 * i - 90,
                      options[0].y,
                      90,
                      options[0].height);
    }
    DrawDialogBox(0x99, 0x83, 0x14E, 0xDA);
    SetTextSize(16);
    SetTextAlignCenter();
    SetTextStyleNormal();
    DrawText("%rySELECT OPTION", 0x9F, 0x55);
    SetTextSize(14);
    if (g_bDecalSlotHasMirror) {
        DrawText("%wwMIRROR", 0x6E, 0x6E);
    } else {
        DrawText("%55MIRROR", 0x6E, 0x6E);
    }
    DrawText("%wwFLIP H", 0xA2, 0x6E);
    DrawText("%wwFLIP V", 0xD3, 0x6E);
    for (i = 0; i < MIRROR_OPTIONS; i++) {
        if (i == g_bStyleMenuCursor) {
            DrawRectOutline(options[i].x,
                            options[i].y,
                            options[i].width,
                            options[i].height,
                            1,
                            0x20,
                            0xC8,
                            0xFF);
        } else {
            DrawRectOutline(options[i].x,
                            options[i].y,
                            options[i].width,
                            options[i].height,
                            1,
                            0x20,
                            0x20,
                            0x20);
        }
    }
    DrawSunkenFrame(0xAC, 0xF8, 0x128, 0x3A);
    SetTextSize(11);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    if (g_bStyleMenuCursor != MIRROR_OPTION_MIRROR) {
#if TGR_REGION == TGR_REGION_JP
        DrawText(flipTexts[g_bStyleMenuCursor - 1][0], 0x9F, 0x87);
        DrawText(flipTexts[g_bStyleMenuCursor - 1][1], 0x9F, 0x94);
#else
        DrawText(flipTexts[g_bStyleMenuCursor - 1], 0x9F, 0x87);
        DrawText(axisTexts[g_bStyleMenuCursor - 1], 0x9F, 0x94);
#endif
    } else if (!g_bDecalSlotHasMirror) {
#if TGR_REGION == TGR_REGION_JP
        DrawText("%jこのデカールは", 0x9F, 0x87);
        DrawText(" %j他に使用できません。", 0x9F, 0x94);
#else
        DrawText("MIRROR IS NOT APPLICABLE", 0x9F, 0x87);
        DrawText("TO THE CURRENT DECAL", 0x9F, 0x94);
#endif
    } else {
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
        DrawText(mirrorTexts[g_bDecalSlot][0], 0x9F, 0x87);
        DrawText(mirrorTexts[g_bDecalSlot][1], 0x9F, 0x94);
#else
        switch (g_bDecalSlot) {
        case SLOT_LEFT_DOOR:
            DrawText("MIRROR CURRENT DECAL", 0x9F, 0x87);
            DrawText("TO THE RIGHT DOOR", 0x9F, 0x94);
            break;
        case SLOT_RIGHT_DOOR:
            DrawText("MIRROR CURRENT DECAL", 0x9F, 0x87);
            DrawText("TO THE LEFT DOOR", 0x9F, 0x94);
            break;
        case SLOT_LEFT_FRONT_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE RIGHT FRONT FENDER", 0x9F, 0x94);
            break;
        case SLOT_RIGHT_FRONT_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE LEFT FRONT FENDER", 0x9F, 0x94);
            break;
        case SLOT_LEFT_REAR_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE RIGHT REAR FENDER", 0x9F, 0x94);
            break;
        case SLOT_RIGHT_REAR_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE LEFT REAR FENDER", 0x9F, 0x94);
            break;
        }
#endif /* TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP */
    }
    DrawMenuButtons(buttonY, cancelX);
    UpdateStickRepeat(&g_aPlayerInputState[g_bPaintShopPlayer]);
    input = &g_aPlayerInputState[g_bPaintShopPlayer];
    if (input->buttons & INPUT_RIGHT) {
        TransferFlags(input, INPUT_RIGHT);
        if (g_bStyleMenuCursor == MIRROR_OPTIONS - 1) {
            g_bStyleMenuCursor = 0;
        } else {
            g_bStyleMenuCursor++;
        }
    } else if (input->buttons & INPUT_LEFT) {
        TransferFlags(input, INPUT_LEFT);
        if (g_bStyleMenuCursor == 0) {
            g_bStyleMenuCursor = MIRROR_OPTIONS - 1;
        } else {
            g_bStyleMenuCursor--;
        }
    }
    input = &g_aPlayerInputState[g_bPaintShopPlayer];
    if (input->buttons & INPUT_A) {
        TransferFlags(input, INPUT_A);
        g_texPaintToolMirror.style = g_bStyleMenuCursor;
        if (g_bStyleMenuCursor == MIRROR_OPTION_MIRROR) {
            if (g_bDecalSlotHasMirror) {
                MirrorDecalToOppositePanel();
            }
        } else {
            FlipDecalPixels();
        }
    } else if (input->buttons & INPUT_B) {
        TransferFlags(input, INPUT_B);
        g_texPaintToolMirror.style = 0;
        g_bStyleMenuOpen = FALSE;
        g_bPaintTool = g_bPreviousPaintTool;
    }
}

void MirrorDecalToOppositePanel(void) {
    u8 *source;
    s32 x, y;

    if (!g_bDecalSlotHasMirror) {
        g_bPaintTool = g_bPreviousPaintTool;
        return;
    }
    switch (g_bDecalSlot) {
    case SLOT_LEFT_DOOR:
        g_bMirrorTargetPanel = SLOT_RIGHT_DOOR;
        break;
    case SLOT_RIGHT_DOOR:
        g_bMirrorTargetPanel = SLOT_LEFT_DOOR;
        break;
    case SLOT_LEFT_FRONT_FENDER:
        g_bMirrorTargetPanel = SLOT_RIGHT_FRONT_FENDER;
        break;
    case SLOT_RIGHT_FRONT_FENDER:
        g_bMirrorTargetPanel = SLOT_LEFT_FRONT_FENDER;
        break;
    case SLOT_LEFT_REAR_FENDER:
        g_bMirrorTargetPanel = SLOT_RIGHT_REAR_FENDER;
        break;
    case SLOT_RIGHT_REAR_FENDER:
        g_bMirrorTargetPanel = SLOT_LEFT_REAR_FENDER;
        break;
    }
    g_bPreviousDecalSlot = g_bDecalSlot;
    source = g_pCurrentVehicleModelData
                 ->textures[g_pCurrentVehicleModelData->materialTextures[g_bPreviousDecalSlot]]
                 .image;
    g_bDecalSlot = g_bMirrorTargetPanel;
    /* The size and the palette stay those of the source. Each pair of slots shares them. */
    g_pDecalTexels = g_pCurrentVehicleModelData
                         ->textures[g_pCurrentVehicleModelData->materialTextures[g_bDecalSlot]]
                         .image;
    g_pDecalMask = g_pCurrentVehicleModelData->decalMasks[g_bDecalSlot];
    g_pDecalSlotIcon = g_apDecalSlotIcons[g_bDecalSlot];
    s_pMirrorSource = source;
    SaveDecalUndo();
    /* IDO uses source locations to schedule independent global loads. */
#line 50
    y = 0;
    if (g_bDecalHeight > 0) {
#line 10000
        source = s_pMirrorSource;
#line 100
        do {
            for (x = 0; x < g_bDecalWidth; x++) {
                SetPixel4Bit(g_bDecalWidth - x - 1, y, ReadPixel4Bit(source, x, y));
            }
            y++;
        } while (y < g_bDecalHeight);
        s_pMirrorSource = source;
    }
#line 1585
    g_bDecalSlotChanged = TRUE;
    g_wScreenFrameCount = 0;
    g_bPaintTool = g_bPreviousPaintTool;
    s_pMirrorSource = NULL;
}

void FlipDecalPixels(void) {
    s32 x, y;

    /* The undo buffer is the copy the flip reads from. */
    SaveDecalUndo();
    if (g_texPaintToolMirror.style == MIRROR_OPTION_FLIP_HORIZONTAL) {
        for (y = 0; y < g_bDecalHeight; y++) {
            for (x = 0; x < g_bDecalWidth; x++) {
                SetPixel4Bit(g_bDecalWidth - x - 1, y, ReadPixel4Bit(g_pDecalUndoBuffer, x, y));
            }
        }
    } else if (g_texPaintToolMirror.style == MIRROR_OPTION_FLIP_VERTICAL) {
        for (y = 0; y < g_bDecalHeight; y++) {
            for (x = 0; x < g_bDecalWidth; x++) {
                SetPixel4Bit(x, g_bDecalHeight - y - 1, ReadPixel4Bit(g_pDecalUndoBuffer, x, y));
            }
        }
    }
    g_texPaintToolMirror.style = 0;
    g_bPaintTool = g_bPreviousPaintTool;
}

void AllocateEditorBuffers(void) {
#if TGR_REGION != TGR_REGION_JP
    osSyncPrintf("\nAllocating %d bytes for decal buffer...\n\n", DECAL_BUFFER_SIZE);
#endif
    g_pDecalUndoBuffer = AllocHeapAligned(DECAL_BUFFER_SIZE);
    LoadTextureFromRom(&g_texButtonA);
    LoadTextureFromRom(&g_texButtonB);
}

#ifdef __sgi
void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
    }
    if (width < 2) {
        width = 1;
    }
    if (height < 2) {
        height = 1;
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetFillColor(gDisplayListPtr++,
                    GPACK_RGBA5551(red, green, blue, 1) << 16 |
                        GPACK_RGBA5551(red, green, blue, 1));
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}
#endif

s32 ProcessDecalSaveLoad(VehicleModel *m, u8 player, u8 operation, u8 inCarSelect, u8 *done) {
#ifdef __sgi
    static u8 g_bPakDialogActive = 0;
    static u8 g_bPakFileNeedsCreate = 0;
    static s32 g_dwPakFileOffset = 0;
    static u8 g_bPakRumbleSwapped;
    static u8 g_bPakRetryNewPak;
    static s32 g_dwPakError;
    static s32 g_dwPakFileNo;
    static s32 g_dwPakChunkSize;
    static u8 g_bPakDialogState;
    static u8 g_bPakDecalIndex;
    static u8 g_bPakWaitFrames;
#endif
#ifdef __sgi
    ModelTexture *part;
#endif
    OSPfs *pfs;
    unsigned char *tex;
    int top;
    unsigned char *mask;
    int stride;
    int half;
    int ty;
    int row;
    int y;
    int col;
    int freeBytes;
    int w;
    int h;
    unsigned short *pal;
    int k;
    int flip;
    int n;
    int x2;
    int tx;
    unsigned char pakBits;
    unsigned char name[16] = {0x2D,
                              0x28,
                              0x29,
                              0x0F,
                              0x20,
                              0x1E,
                              0x1A,
                              0x2B,
                              0x0F,
                              0x1D,
                              0x1E,
                              0x1C,
                              0x1A,
                              0x25,
                              0x2C,
                              0x00};
    unsigned char ext[4] = {0, 0, 0, 0};

    g_wSerialInterfaceIdle = 0;
    if (g_bPakDialogActive == 0) {
        g_bPakDialogActive = 1;
        g_bPakRumbleSwapped = 0;
        g_bPakRetryNewPak = 0;
        g_dwPakError = 0;
        g_bPakDecalIndex = 0;
        g_dwPakFileOffset = 0;
        g_dwPakChunkSize = 0;
        g_bPakWaitFrames = 0;
        if (inCarSelect == 1) {
            g_bPakDialogState = 0;
        } else {
            g_bPakDialogState = 2;
        }
    }
    *done = 0;
    switch (g_bPakDialogState) {
    case 0:
        FadeSfxTo(0.0f, 0.2f);
        FadeMusicTo(0.0f, 0.2f);
        g_bPakDialogState = 1;
        break;
    case 1:
        if (HasSfxFadeFinished() != 0 && g_bPakWaitFrames++ == 3) {
            g_bPakDialogState = 3;
            g_bPakWaitFrames = 0;
        }
        break;
    case 2: /* load/save? */
        y = 0x110 - g_texButtonA.width;
#if (TGR_REGION == TGR_REGION_JP)
        DrawRectBorder(0xAA, 200, 300, 0x50, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextSize(14);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%j\xBB\x81(\xA8w\xC8(\xCB"
                     "B6R1%%",
                     94,
                     116);
        } else {
            DrawText("%j\xBB\x81(\xA8w\xB2(\xAC"
                     "B6R1%%",
                     94,
                     116);
        }
        SetTextSize(10);
        DrawText("%wwOK", (g_texButtonA.width + 0xC0U) >> 1, (y + 18) >> 1);
        DrawText("%wwCANCEL", (g_texButtonB.width + 0x168U) >> 1, (y + 18) >> 1);
        DrawTexturedSprite(&g_texButtonA, 0xBA, y);
        DrawTexturedSprite(&g_texButtonB, 0x162, y);
#else
        DrawRectBorder(0xDC, 200, 200, 0x50, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextSize(14);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("LOAD DECALS?", 119, 116);
        } else {
            DrawText("SAVE DECALS?", 119, 116);
        }
        SetTextSize(10);
        DrawText("%wwOK", (g_texButtonA.width + 0xF2U) >> 1, (y + 18) >> 1);
        DrawText("%wwCANCEL", (g_texButtonB.width + 0x136U) >> 1, (y + 18) >> 1);
        DrawTexturedSprite(&g_texButtonA, 0xEC, y);
        DrawTexturedSprite(&g_texButtonB, 0x130, y);
#endif
        if (g_aPlayerInputState[player].buttons & 0x10) {
            TransferFlags((&g_aPlayerInputState[player]), 0x10);
            g_bPakDialogState = 0;
        } else {
            if (g_aPlayerInputState[player].buttons & 0x20) {
                TransferFlags((&g_aPlayerInputState[player]), 0x20);
                *done = 1;
                g_bPaintTool = g_bPreviousPaintTool;
                return 0;
            }
        }
        break;
    case 3: /* probe the pak */
        osPfsIsPlug(&g_SiMesgQueue, &pakBits);
        if ((pakBits & (1 << player)) == 0) {
            g_dwPakError = 1;
            g_bPakDialogState = 12;
        } else {
#if !(TGR_REGION == TGR_REGION_JP)
            osSyncPrintf("\nInitializing controller pak...\n");
#endif
            g_dwPakError = osPfsInitPak(&g_SiMesgQueue, (&g_aPakPfs[player]), player);
            if (g_dwPakError == 0) {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = 0;
                if (g_abSavePakIdKnown[player] == 0) {
                    g_abSavePakIdKnown[player] = 1;
                    memcpy(g_aabSavePakIds[player], g_aPakPfs[player].id, 32);
                    g_bPakDialogState = 6;
                } else if (bcmp(g_aabSavePakIds[player], g_aPakPfs[player].id, 32) != 0) {
                    memcpy(g_aabSavePakIds[player], g_aPakPfs[player].id, 32);
                    g_bPakDialogState = 9;
                } else {
                    g_bPakDialogState = 6;
                }
            } else if (g_dwPakError == 10) {
                if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) == 0) {
                    osMotorStop(&g_aContPfs[player]);
                    g_dwPakError = 9999;
                    g_bPakRumbleSwapped = 1;
                    g_bPakDialogState = 10;
                } else {
                    g_abRumblePakPresent[g_dwMenuChoosingPlayer] = 0;
                    g_dwPakError = 0;
                    g_bPakDialogState = 4;
                }
            } else {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = 0;
                g_bPakDialogState = 12;
            }
        }
        break;
    case 4:
        g_dwPakError = osPfsRepairId((&g_aPakPfs[player]));
        if (g_dwPakError != 0) {
            g_bPakDialogState = 10;
        } else {
            g_bPakDialogState = 3;
        }
        break;
    case 6: /* find the file */
        g_dwPakFileOffset = 0;
        g_dwPakChunkSize = 0;
        g_bPakDecalIndex = 0;
        g_bPakFileNeedsCreate = 0;
        pfs = (&g_aPakPfs[player]);
        g_bPakWaitFrames = 0;
        g_dwPakError =
            osPfsFindFile(pfs, DECAL_FILE_COMPANY, DECAL_FILE_GAME, name, ext, &g_dwPakFileNo);
        if (g_dwPakError == 0) {
            if (operation == PAINT_TOOL_LOAD) {
#if !(TGR_REGION == TGR_REGION_EU) && !(TGR_REGION == TGR_REGION_JP)
                top = 0xC5;
                if (inCarSelect == 1) {
                    top = 0xDB;
                }
                DrawRectBorder(0xD4, top, 0xD8, 0x56, 3, 0, 0, 0x80, 0x80, 0x80);
#elif (TGR_REGION == TGR_REGION_EU)
                if (inCarSelect == 1) { /* the panel moved to state 7; the test stays */
                }
#endif
                SetTextAlignLeft();
                SetTextStyleNormal();
                SetTextSize(13);
                SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
#if !(TGR_REGION == TGR_REGION_EU) && !(TGR_REGION == TGR_REGION_JP)
                DrawText("LOADING DECALS...", 114, (top + 30) >> 1);
                DrawRectBorder(0xE6, top + 0x34, 0xB4, 0x14, 1, 1, 1, 0x80, 0x80, 0x80);
#endif
                g_bPakDialogState = 7;
            } else {
                g_bPakDialogState = 5;
            }
        } else if (g_dwPakError == 5) {
            if (operation == PAINT_TOOL_LOAD) {
                g_bPakDialogState = 12;
            } else if (osPfsFreeBlocks(pfs, &freeBytes) == 0) {
                if (freeBytes < 0x3E00) {
                    g_dwPakError = 7;
                    g_bPakDialogState = 12;
                } else {
#if !(TGR_REGION == TGR_REGION_EU) && !(TGR_REGION == TGR_REGION_JP)
                    DrawRectBorder(0xD4, 0xC5, 0xD8, 0x56, 3, 0, 0, 0x80, 0x80, 0x80);
#endif
                    SetTextAlignLeft();
                    SetTextStyleNormal();
                    SetTextSize(13);
                    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
#if !(TGR_REGION == TGR_REGION_EU) && !(TGR_REGION == TGR_REGION_JP)
                    DrawText("SAVING DECALS...", 115, 113);
                    DrawRectBorder(0xE6, 0xF9, 0xB4, 0x14, 1, 1, 1, 0x80, 0x80, 0x80);
#endif
                    g_bPakFileNeedsCreate = 1;
                    g_bPakDialogState = 7;
                    g_bPakWaitFrames = 0;
                }
            } else {
                g_dwPakError = 4;
                g_bPakDialogState = 12;
            }
        } else if (g_dwPakError == 3) {
            g_dwPakError = osPfsChecker(pfs);
            if (g_dwPakError == 0) {
                g_bPakDialogState = 6;
            } else {
                g_bPakDialogState = 12;
            }
        } else if (g_dwPakError == 2) {
            g_bPakDialogState = 3;
            g_dwPakError = 0;
        } else {
            g_bPakDialogState = 12;
        }
        break;
    case 5: /* overwrite? */
#if (TGR_REGION == TGR_REGION_JP)
        DrawRectBorder(0xD6, 0xBB, 0xD4, 0x6A, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        SetTextSize(14);
        DrawText("%j\xDA\xE4"
                 "AB6R1%%",
                 121,
                 113);
        y = 0x118 - g_texButtonA.width;
#elif (TGR_REGION == TGR_REGION_EU)
        DrawRectBorder(0xD6, 0xBB, 0xD4, 0x6A, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        SetTextSize(13);
        DrawText("OK TO OVERWRITE", 114, 108);
        DrawText("SAVED DECALS?", 114, 122);
        y = 0x11D - g_texButtonA.width;
#else
        y = 0x11D - g_texButtonA.width;
        DrawRectBorder(0xD6, 0xBB, 0xD4, 0x6A, 3, 0, 0, 0x80, 0x80, 0x80);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextSize(13);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        DrawText("OK TO OVERWRITE", 114, 108);
        DrawText("SAVED DECALS?", 114, 122);
#endif
        SetTextSize(10);
        DrawText("%wwOK", (g_texButtonA.width + 0xEEU) >> 1, (y + 18) >> 1);
        DrawText("%wwCANCEL", (g_texButtonB.height + 0x13AU) >> 1, (y + 18) >> 1);
        DrawTexturedSprite(&g_texButtonA, 0xE8, y);
        DrawTexturedSprite(&g_texButtonB, 0x134, y);
        if (g_aPlayerInputState[player].buttons & 0x10) {
            TransferFlags((&g_aPlayerInputState[player]), 0x10);
            g_bPakDialogState = 7;
        } else if (g_aPlayerInputState[player].buttons & 0x20) {
            TransferFlags((&g_aPlayerInputState[player]), 0x20);
            g_bPakDialogState = 12;
        }
        break;
    case 7: /* one decal a frame */
#if (TGR_REGION == TGR_REGION_JP)
        top = 0xC5;
        ty = 0x71;
        if (inCarSelect == 1) {
            top = 0xDB;
            ty = 0x7C;
        }
        if (operation == PAINT_TOOL_LOAD) {
            DrawRectBorder(0xC4, top, 0xF8, 0x56, 3, 0, 0, 0x80, 0x80, 0x80);
        } else {
            DrawRectBorder(0xD4, top, 0xD8, 0x56, 3, 0, 0, 0x80, 0x80, 0x80);
        }
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextSize(14);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("%j\xBB\x81(\xA8\xC8(\xCB\xE6", g_dwScreenWidth / 2, ty);
        } else {
            DrawText("%j\xBB\x81(\xA8\xB2(\xAC\xE6", g_dwScreenWidth / 2, ty);
        }
#else
        top = 0xC5;
        ty = 0x71;
        if (inCarSelect == 1) {
            /* The unused test preserves IDO's register schedule. */
#ifdef __sgi
            /* clang-format off */
            ty = 0x7C; if (ty); top = 0xDB;
            /* clang-format on */
#else
            ty = 0x7C;
            top = 0xDB;
#endif
        }
        DrawRectBorder(0xD4, top, 0xD8, 0x56, 3, 0, 0, 0x80, 0x80, 0x80);
#if (TGR_REGION == TGR_REGION_EU)
        SetTextStyleNormal();
        SetTextAlignLeft();
#else
        SetTextAlignLeft();
        SetTextStyleNormal();
#endif
        SetTextSize(13);
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        tx = 114;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("LOADING DECALS...", tx, ty);
        } else {
            tx++;
            DrawText("SAVING DECALS...", tx, ty);
        }
#endif
        DrawRectBorder(0xE6, top + 0x34, 0xB4, 0x14, 1, 1, 1, 0x80, 0x80, 0x80);
        if (g_bPakWaitFrames++ > 1) {
            if (g_bPakFileNeedsCreate != 0) {
                pfs = (&g_aPakPfs[player]);
                g_bPakFileNeedsCreate = 0;
                g_dwPakError = osPfsAllocateFile(
                    pfs, DECAL_FILE_COMPANY, DECAL_FILE_GAME, name, ext, 0x3E00, &g_dwPakFileNo);
                switch (g_dwPakError) {
                case 3:
                    g_dwPakError = osPfsChecker(pfs);
                    if (g_dwPakError == 0) {
                        g_bPakDialogState = 3;
                        return 0;
                    }
                    g_bPakDialogState = 2;
                    return g_dwPakError;
                case 5:
                case 9:
                    g_dwPakError = 3;
                    break;
                case 2:
                    g_bPakDialogState = 3;
                    break;
                case 0:
                    break;
                }
                if (g_dwPakError != 0) {
                    if (g_dwPakError == 2) {
                        g_dwPakError = 0;
                        g_bPakDialogState = 3;
                        *done = 0;
                        return 0;
                    }
                    g_bPakDialogState = 12;
                    break;
                }
            }
            if (g_bPakDecalIndex < 10) {
                n = g_bPakDecalIndex;
                w = m->textures[m->materialTextures[g_bPakDecalIndex]].width;
                h = m->textures[m->materialTextures[g_bPakDecalIndex]].height;
                pfs = (&g_aPakPfs[player]);
                if (n == 0 || n == 1) {
                    h++;
                }
                g_dwPakChunkSize = (int)(w * h) >> 1;
                if (operation == PAINT_TOOL_LOAD) {
                    g_dwPakError = osPfsReadWriteFile(pfs,
                                                      g_dwPakFileNo,
                                                      0,
                                                      g_dwPakFileOffset,
                                                      g_dwPakChunkSize,
                                                      g_pDecalUndoBuffer);
                } else {
                    g_dwPakError = osPfsReadWriteFile(
                        pfs,
                        g_dwPakFileNo,
                        1,
                        g_dwPakFileOffset,
                        g_dwPakChunkSize,
                        m->textures[m->materialTextures[g_bPakDecalIndex]].image);
                }
                switch (g_dwPakError) {
                case 0:
                    if (operation == PAINT_TOOL_LOAD) {
#if !(TGR_REGION == TGR_REGION_EU) && !(TGR_REGION == TGR_REGION_JP)
                        osSyncPrintf("Loading decal %d...\n", g_bPakDecalIndex);
#endif
                        tex = m->textures[m->materialTextures[g_bPakDecalIndex]].image;
                        mask = m->decalMasks[g_bPakDecalIndex];
                        if (mask == 0) {
                            memcpy(tex, g_pDecalUndoBuffer, g_dwPakChunkSize);
                        } else {
                            /* keep the pixels the mask marks, take the rest from the pak */
                            for (row = 0; row < h; row++) {
                                stride = (int)(w + 7) >> 3;
                                half = (int)w >> 1;
                                flip = (row & 1) << 3;
                                for (col = 0; col < w; col++) {
                                    x2 = ((int)col >> 3) + row * stride;
                                    if ((mask[x2] & (1 << ((col ^ 7) & 7))) == 0) {
                                        k = ((int)(flip ^ col) >> 1) + row * half;
                                        tex[k] = g_pDecalUndoBuffer[k];
                                    }
                                }
                            }
                        }
#if (TGR_REGION == TGR_REGION_EU)
                        {
                            /* dump the decal's bytes on the debug output, one a line */
                            int i;
                            int size;

                            size = (m->textures[m->materialTextures[g_bPakDecalIndex]].width *
                                    m->textures[m->materialTextures[g_bPakDecalIndex]].height) >>
                                   1;
                            if (g_bPakDecalIndex == 0) {
                                osSyncPrintf("# Format SB BitSize 8 Rows 1 Columns 1\n");
                            }
                            for (i = 0; i < size; i++) {
                                osSyncPrintf("%02x\n", tex[i]);
                            }
                        }
#endif
                    } else {
                        osSyncPrintf("Saving decal %d...\n", g_bPakDecalIndex);
                    }
                    g_dwPakFileOffset += g_dwPakChunkSize;
                    break;
                case 5:
                    g_dwPakError = 99999;
                    break;
                case 3:
                    g_dwPakError = osPfsChecker(pfs);
                    if (g_dwPakError == 1 || g_dwPakError == 4 || g_dwPakError == 3) {
                        g_bPakDecalIndex = 0;
                        g_dwPakFileOffset = 0;
                        g_dwPakChunkSize = 0;
                        g_bPakDialogState = 12;
                    } else {
                        g_bPakDecalIndex--;
                        g_dwPakFileOffset -= g_dwPakChunkSize;
                        g_dwPakError = 0;
                    }
                    break;
                }
                if (g_dwPakError != 0) {
                    if (g_dwPakError == 2) {
                        g_dwPakError = 0;
                        g_bPakDialogState = 9;
                        g_bPakRetryNewPak = 1;
                        *done = 0;
                        return 0;
                    }
                    g_bPakDialogState = 12;
                } else {
                    DrawFilledRectHiRes(
                        0xE7, top + 0x35, g_dwPakFileOffset * 0xB2 / 0x3D00, 0x12, 0, 0, 0xC0);
                    g_bPakDecalIndex++;
                }
            } else {
                g_bPakDecalIndex = 0;
                DrawFilledRectHiRes(0xE7, top + 0x35, 0xB2, 0x12, 0, 0, 0xC0);
                g_bPakDialogState = 8;
            }
        }
        break;
    case 8: /* the palette */
        if (operation == PAINT_TOOL_LOAD) {
#if !(TGR_REGION == TGR_REGION_EU) && !(TGR_REGION == TGR_REGION_JP)
            osSyncPrintf("Loading color palette...\n");
#endif
            pfs = (&g_aPakPfs[player]);
            g_dwPakError =
                osPfsReadWriteFile(pfs,
                                   g_dwPakFileNo,
                                   0,
                                   g_dwPakFileOffset,
                                   0x20,
                                   (unsigned char *)m->textures[m->materialTextures[2]].palette);
            if (g_dwPakError == 0) {
                pal = m->textures[m->materialTextures[2]].palette;
#if (TGR_REGION == TGR_REGION_EU)
                {
                    /* dump the palette on the debug output, a byte a line */
                    int i;

                    for (i = 0; i < 16; i++) {
                        osSyncPrintf("%02x\n%02x\n", pal[i] >> 8, pal[i] & 0xFF);
                    }
                }
#endif
                m->textures[m->materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]].palette[0] = pal[0];
                m->textures[m->materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]].palette[1] = pal[1];
                m->textures[m->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette[0] = pal[0];
                m->textures[m->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette[1] = pal[1];
                if (inCarSelect == 0) {
                    ConvertPaletteToRgb();
                    SaveDecalUndo();
                }
            }
        } else {
#if !(TGR_REGION == TGR_REGION_JP)
            osSyncPrintf("Saving color palette...\n");
#endif
            pfs = (&g_aPakPfs[player]);
            g_dwPakError =
                osPfsReadWriteFile(pfs,
                                   g_dwPakFileNo,
                                   1,
                                   g_dwPakFileOffset,
                                   0x20,
                                   (unsigned char *)m->textures[m->materialTextures[2]].palette);
        }
        switch (g_dwPakError) {
        case 0:
#if !(TGR_REGION == TGR_REGION_JP)
            if (operation == PAINT_TOOL_LOAD) {
                osSyncPrintf("Color palette loaded successfully!\n\n");
            } else {
                osSyncPrintf("Color palette saved successfully!\n\n");
            }
#endif
            g_bPakDialogState = 11;
            if (inCarSelect == 0) {
                g_bDecalEditCount = 0;
            }
            break;
        case 3:
            g_dwPakError = osPfsChecker(pfs);
            if (g_dwPakError == 1 || g_dwPakError == 4 || g_dwPakError == 3) {
                g_bPakDialogState = 12;
            } else {
                g_dwPakFileOffset -= g_dwPakChunkSize;
                g_dwPakError = 0;
                g_bPakDialogState = 8;
            }
            break;
        }
        if (g_dwPakError != 0) {
#if !(TGR_REGION == TGR_REGION_JP)
            osSyncPrintf("err_code = %d\n\n", g_dwPakError);
#endif
            if (g_dwPakError == 2) {
                g_bPakDialogState = 9;
                g_bPakRetryNewPak = 1;
                *done = 0;
                g_dwPakError = 0;
                return 0;
            }
            g_bPakDialogState = 12;
            *done = 0;
        }
        break;
    case 9: /* a different pak */
        top = 0xAB;
        if (inCarSelect == 1) {
            top = 0xD3;
        }
        SetTextStyleNormal();
        SetTextAlignLeft();
#if (TGR_REGION == TGR_REGION_JP)
        SetTextSize(14);
        DrawRectBorder(0x99, top, 0x14E, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
#else
        SetTextSize(12);
        DrawRectBorder(0xAF, top, 0x122, 0x89, 3, 0, 0, 0x80, 0x80, 0x80);
#endif
        if (inCarSelect == 1) {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        } else {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        }
#if (TGR_REGION == TGR_REGION_JP)
        DrawText("%j038B@\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1"
                 "9",
                 84,
                 (top + 32) >> 1);
        DrawText("%j2Bq6hc@6R&", 84, ((top + 32) >> 1) + 13);
        DrawText("%jqt\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w'", 84, ((top + 32) >> 1) + 32);
        DrawText("%j\xDE@6R&", 84, ((top + 32) >> 1) + 45);
#else
        DrawText("A NEW CONTROLLER PAK", 95, (top + 32) >> 1);
        DrawText("WAS INSERTED.", 95, ((top + 32) >> 1) + 13);
        DrawText("THIS CONTROLLER PAK", 95, ((top + 32) >> 1) + 32);
        DrawText("WILL BE USED.", 95, ((top + 32) >> 1) + 45);
#endif
        if (g_aPlayerInputState[player].buttons & 0x8030) {
            TransferFlags((&g_aPlayerInputState[player]), 0x8030);
            if (g_bPakRetryNewPak != 0) {
                g_bPakRetryNewPak = 0;
                g_bPakDialogState = 3;
            } else {
                g_bPakDialogState = 6;
            }
        }
        break;
    case 10: /* a Rumble Pak is in the way, or an error */
        SetTextStyleNormal();
        SetTextAlignLeft();
#if (TGR_REGION == TGR_REGION_JP)
        SetTextSize(14);
#else
        SetTextSize(12);
#endif
        if (inCarSelect == 1) {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        } else {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        }
        if (g_dwPakError == 9999) {
#if (TGR_REGION == TGR_REGION_JP)
            top = 0xAD;
            if (inCarSelect == 1) {
                top = 0xD5;
            }
            y = top - g_texButtonA.width + 0x7E;
            DrawRectBorder(200, top, 0xF0, 0x86, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("%j\xD2\xD3\x8D\x8F\xA1s", 108, (top + 32) >> 1);
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1w", 108, ((top + 32) >> 1) + 13);
            DrawText("%j\xD4\xD5"
                     "Bc\xDB"
                     "2@&",
                     108,
                     ((top + 32) >> 1) + 26);
#else
            top = 0xA0;
            if (inCarSelect == 1) {
                top = 200;
            }
            y = top - g_texButtonA.width + 0x98;
            DrawRectBorder(0x7A, top, 0x18C, 0xA0, 3, 0, 0, 0x80, 0x80, 0x80);
            DrawText("PLEASE REMOVE THE RUMBLE PAK", 69, (top + 32) >> 1);
            DrawText("AND INSERT THE CONTROLLER PAK", 69, ((top + 32) >> 1) + 13);
            DrawText("INTO CONTROLLER.  PRESS THE A", 69, ((top + 32) >> 1) + 26);
            DrawText("BUTTON WHEN READY.", 69, ((top + 32) >> 1) + 39);
#endif
            SetTextSize(10);
            DrawText("%wwOK", (g_texButtonA.width + 0xDEU) >> 1, (y + 18) >> 1);
            DrawText("%wwCANCEL", (g_texButtonB.width + 0x14AU) >> 1, (y + 18) >> 1);
            DrawTexturedSprite(&g_texButtonA, 0xD8, y);
            DrawTexturedSprite(&g_texButtonB, 0x144, y);
        }
        if (g_dwPakError != 9999) {
            if (g_aPlayerInputState[player].buttons & 0x8030) {
                TransferFlags((&g_aPlayerInputState[player]), 0x8030);
                g_bPakDialogState = 12;
            }
        } else if (g_aPlayerInputState[player].buttons & 0x10) {
            TransferFlags((&g_aPlayerInputState[player]), 0x10);
            g_dwPakError = 0;
            g_bPakDialogState = 3;
        } else if (g_aPlayerInputState[player].buttons & 0x20) {
            TransferFlags((&g_aPlayerInputState[player]), 0x20);
            g_dwPakError = 0;
            g_bPakDialogState = 12;
            if (inCarSelect == 1) {
                g_dwPakError = 999;
            }
        }
        break;
    case 11: /* put the Rumble Pak back */
        g_bPakDecalIndex = 0;
        g_dwPakFileOffset = 0;
        g_dwPakChunkSize = 0;
        g_bPakWaitFrames = 0;
        if (inCarSelect == 0) {
            g_bPakDialogState = 12;
        } else if (g_bPakRumbleSwapped != 0) {
#if (TGR_REGION == TGR_REGION_JP)
            DrawRectBorder(0xAA, 0xE6, 300, 100, 3, 0, 0, 0x80, 0x80, 0x80);
            SetTextStyleNormal();
            SetTextAlignLeft();
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
            SetTextSize(14);
            DrawText("%j\xD2\xD3\x8D\x8F\xA1w\xDEPsA5'", 93, 131);
            DrawText("%j\xC1\xCF\xC3\xC8(\x88\x8D\x8F\xA1s", 93, 144);
            DrawText("%j\xD2\xD3\x8D\x8F\xA1w\xD4\xD5"
                     "Bc\xDB"
                     "2@&",
                     93,
                     157);
#else
            DrawRectBorder(0x73, 0xCC, 0x19A, 0x98, 3, 0, 0, 0x80, 0x80, 0x80);
            SetTextStyleNormal();
            SetTextAlignLeft();
#if (TGR_REGION == TGR_REGION_EU)
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
            SetTextSize(12);
#else
            SetTextSize(12);
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
#endif
            DrawText("IF THE RUMBLE PAK IS TO BE USED,", 65, 118);
            DrawText("REMOVE THE CONTROLLER PAK AND", 65, 131);
            DrawText("INSERT THE RUMBLE PAK INTO THE", 65, 144);
            DrawText("CONTROLLER.  PRESS THE A BUTTON", 65, 157);
            DrawText("TO CONTINUE.", 65, 170);
#endif
            if (g_aPlayerInputState[player].buttons & 0x8010) {
                TransferFlags((&g_aPlayerInputState[player]), 0x8010);
                g_bPakRumbleSwapped = 0;
                g_dwPakError = 0;
                if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) == 0) {
                    g_abRumblePakPresent[player] = 1;
                }
                g_bPakDialogState = 12;
            }
        } else {
            g_bPakDialogState = 12;
        }
        break;
    case 12: /* done */
        g_bPakDialogActive = 0;
        FadeSfxTo(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        g_wSerialInterfaceIdle = 1;
        if (g_dwPakError == 0) {
            *done = 1;
            if (inCarSelect == 0) {
                g_bPaintTool = g_bPreviousPaintTool;
            }
            return 0;
        }
        if (inCarSelect == 0 && (g_aPlayerInputState[player].buttons & 0x8030) != 0) {
            TransferFlags((&g_aPlayerInputState[player]), 0x8030);
            g_bPaintTool = g_bPreviousPaintTool;
        }
        *done = 0;
        return g_dwPakError;
    }
    return 0;
}

void ProcessTextInputKeyboard(void) {
    s32 spare; /* IDO reserves the original stack frame for this unused local. */
    s32 x;
    s32 y;
    s32 i;
    s32 kern;
    PlayerInputState *input;

    g_bShowPreview3D = FALSE;
    g_bPaletteEnabled = FALSE;
    DrawSpriteTinted(&g_texKeyboardBackground, 0xE0, 0xE0, 0xE0);
    if (g_texKeyboardHighlight.style != 0) {
        g_texKeyboardHighlight.style = 0;
        if (g_wScreenFrameCount > 0) {
            g_wScreenFrameCount--;
        }
        if (g_wScreenFrameCount == 0) {
            DrawSpriteTinted(&g_texKeyboardHighlight, 0x20, 0xC8, 0xFF);
        } else {
            DrawSpriteTinted(&g_texKeyboardHighlight, 0xFF, 0xEB, 0);
        }
    }
    DrawSpriteAtOrigin(WITH_PALETTE(&g_texKeyboardPanel, NULL));
    x = g_texKeyboardPanel.originX;
    y = g_texKeyboardPanel.originY + g_texKeyboardPanel.height + 0x20;
    DrawSpriteRegion(&g_texKeyboardBackground, x, y, 0xC8, 0x30, 0xE0, 0xE0, 0xE0);
    DrawRectOutline(x, y, 0xC8, 0x30, 2, 0xFF, 0xFF, 0xFF);
    if (IsCursorInButton(&g_texKeyboardBackground)) {
        for (i = 0; i < DECAL_GLYPHS; i++) {
            if (IsCursorOnKeyboardKey(&g_aDecalGlyphs[i])) {
                g_bKeyboardHoverKey = i;
                g_texKeyboardHighlight.style = 1;
                g_texKeyboardHighlight.originX = g_aDecalGlyphs[i].x;
                g_texKeyboardHighlight.originY = g_aDecalGlyphs[i].y;
            }
            if (IsCursorInButton(&g_texKeyboardHighlight) &&
                ((input = &g_aPlayerInputState[g_bPaintShopPlayer])->buttons &
                 (INPUT_Z | INPUT_A))) {
                TransferFlags(input, INPUT_Z | INPUT_A);
                g_wScreenFrameCount = KEY_FLASH_FRAMES;
                if (g_bKeyboardHoverKey == KEY_ENTER) {
                    if (g_bDecalTextLength > 0) {
                        g_bPaintToolStarted = TRUE;
                        g_bTextShadowPickerOpen = TRUE;
                    } else {
                        g_bPaintToolStarted = FALSE;
                        g_bDecalDragActive = FALSE;
                    }
                    g_bShowPreview3D = TRUE;
                    g_bPaletteEnabled = TRUE;
                } else if (g_bKeyboardHoverKey == KEY_BACKSPACE) {
                    /* An empty text reads the word before the keys of the text. */
                    if (g_apDecalTextGlyphs[g_bDecalTextLength - 1] != NULL) {
                        g_bDecalTextLength--;
                        if (g_bDecalTextLength == 0) {
                            g_bDecalTextWidth -= g_apDecalTextGlyphs[g_bDecalTextLength]->width;
                        } else {
                            kern = g_apDecalTextGlyphs[g_bDecalTextLength - 1]->kernRight <
                                           g_apDecalTextGlyphs[g_bDecalTextLength]->kernLeft ?
                                       g_apDecalTextGlyphs[g_bDecalTextLength - 1]->kernRight :
                                       g_apDecalTextGlyphs[g_bDecalTextLength]->kernLeft;
                            g_bDecalTextWidth = g_bDecalTextWidth -
                                                g_apDecalTextGlyphs[g_bDecalTextLength]->width +
                                                kern - 2;
                        }
                    } else {
                        g_bDecalTextLength = 0;
                        g_bDecalTextWidth = 0;
                    }
                    g_szDecalText[g_bDecalTextLength] = 0;
                    g_apDecalTextGlyphs[g_bDecalTextLength] = NULL;
                } else {
                    if (g_aDecalGlyphs[g_bKeyboardHoverKey].width + g_bDecalTextWidth + 2 <
                        g_bDecalWidth) {
                        g_szDecalText[g_bDecalTextLength] =
                            g_aDecalGlyphs[g_bKeyboardHoverKey].character;
                        g_apDecalTextGlyphs[g_bDecalTextLength] =
                            &g_aDecalGlyphs[g_bKeyboardHoverKey];
                        if (g_bDecalTextLength == 0) {
                            g_bDecalTextWidth =
                                g_aDecalGlyphs[g_bKeyboardHoverKey].width + g_bDecalTextWidth;
                        } else {
                            kern = g_apDecalTextGlyphs[g_bDecalTextLength - 1]->kernRight <
                                           g_apDecalTextGlyphs[g_bDecalTextLength]->kernLeft ?
                                       g_apDecalTextGlyphs[g_bDecalTextLength - 1]->kernRight :
                                       g_apDecalTextGlyphs[g_bDecalTextLength]->kernLeft;
                            g_bDecalTextWidth = g_aDecalGlyphs[g_bKeyboardHoverKey].width +
                                                g_bDecalTextWidth - kern + 2;
                        }
                        g_szDecalText[++g_bDecalTextLength] = 0;
                        if (g_bDecalTextLength > DECAL_TEXT_LENGTH) {
                            g_bDecalTextLength = DECAL_TEXT_LENGTH;
                        }
                    }
                }
            }
        }
    }
    if (g_bDecalTextLength > 0) {
        SetTextCustomColors(g_aDecalSwatches[g_bDecalColor].color[0],
                            g_aDecalSwatches[g_bDecalColor].color[1],
                            g_aDecalSwatches[g_bDecalColor].color[2],
                            g_aDecalSwatches[g_bDecalColor].color[0],
                            g_aDecalSwatches[g_bDecalColor].color[1],
                            g_aDecalSwatches[g_bDecalColor].color[2]);
        SetTextSize(20);
        SetTextAlignCenter();
        SetTextStyleNormal();
        DrawText(g_szDecalText, (x + 0x64) >> 1, (y + 0x24) >> 1);
    }
}
