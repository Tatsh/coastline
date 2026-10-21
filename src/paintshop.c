#include "paintshop.h"

#include <PR/gbi.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "decaldraw.h"
#include "fade.h"
#include "game.h"
#include "heap.h"
#include "input.h"
#include "options.h"
#include "pakmenu.h"
#include "racemenu.h"
#include "render.h"
#include "rumble.h"
#include "system.h"
#include "text.h"
#include "timing.h"
#include "vehiclerender.h"
#include "visgrid.h"
#include "weather.h"

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
#define DECAL_FILE_COMPANY 0x3544
#define DECAL_FILE_GAME 0x4E475245
#define DECAL_FILE_SIZE 0x3E00

/* The bytes of the decals at which the progress bar is full, and the width of the bar. */
#define DECAL_PROGRESS_FULL 0x3D00
#define DECAL_PROGRESS_WIDTH 0xB2

/* The bytes of the palette of the paint saved after the decals. */
#define DECAL_PALETTE_BYTES 0x20

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
        /* The format has five values and is given four, without the computed size. */
        osSyncPrintf(
            "ERROR: AllocPaintShopGfxMem given image %dx%dx%d = %d bytes but uncompressed rom "
            "image is %d bytes\n",
            sprite->width,
            sprite->height,
            sprite->bitsPerTexel,
            DmaReadAssetSize(sprite->romOffset));
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

/* Put the car of the player in the paint shop, facing the camera by the track. */
static inline void SetUpPaintShopCar(void) {
    CameraFrame *camera;
    Vehicle *vehicle;

    g_dwVehicleViewModeIndex = 0;
    g_bPaintShopPlayer = g_dwMenuChoosingPlayer;
    g_dwNumActiveVehicles = 1;
    InitVehicleColor(&g_aVehicles[g_bPaintShopPlayer]);
    g_aVehicles[g_bPaintShopPlayer].driver = UpdatePhysicsStep;
    g_aVehicles[g_bPaintShopPlayer].carModel = g_aVehicles[0].carModel;
    InstallVehicleModel(
        &g_aVehicles[g_bPaintShopPlayer], g_bPaintShopPlayer, g_aVehicles[0].carModel);
    vehicle = &g_aVehicles[g_bPaintShopPlayer];
    g_pCurrentVehicleModelData = vehicle->model;
    vehicle->frame.position.z = 0.0f;
    vehicle->frame.position.y = 0.0f;
    vehicle->frame.position.x = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.up.z = 1.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.up.y = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.up.x = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.forward.y = 1.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.forward.x = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.forward.z = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.side.z = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.side.y = 0.0f;
    g_aVehicles[g_bPaintShopPlayer].frame.side.x = -1.0f;
    vehicle = &g_aVehicles[g_bPaintShopPlayer];
    camera = &vehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    if (vehicle->carModel == CAR_LOWERED_IN_PREVIEW) {
        camera->position.x = 7.3f;
        camera->position.y = 7.3f;
        camera->position.z = 2.0f;
    } else {
        camera->position.x = 6.5f;
        camera->position.y = 6.5f;
        camera->position.z = 2.0f;
    }
    camera = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE];
    Vec3fNegate(&camera->forward, &camera->position);
    Vec3fNormalize(&g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE].forward);
    camera = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE];
    camera->up.y = 0.0f;
    camera->up.x = 0.0f;
    camera->up.z = 1.0f;
    camera = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE];
    Vec3fCross(&camera->side, &camera->up, &camera->forward);
    camera = &g_aVehicles[g_bPaintShopPlayer].cameras[VEHICLE_CAMERA_TRACKSIDE];
    Vec3fCross(&camera->up, &camera->forward, &camera->side);
    vehicle = &g_aVehicles[g_bPaintShopPlayer];
    vehicle->groundInstanceCount = 0;
    vehicle->nearbyInstanceCount = 0;
}

/* Load the buttons of the decal slots and work out the rectangles that pick them. */
static inline void LoadDecalSlotIcons(void) {
    Sprite *icon;
    ScreenRect *rect;
    s32 i;

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
    for (i = 0; i < DECAL_SLOTS; i++) {
        icon = g_apDecalSlotIcons[i];
        rect = &g_aDecalSlotRects[i];
        switch (i) {
        case SLOT_LEFT_DOOR:
        case SLOT_RIGHT_DOOR:
            SetScreenRect(
                rect, icon->originX + 5, icon->originY + 2, icon->width - 10, icon->height - 4);
            break;
        case SLOT_LEFT_FRONT_FENDER:
        case SLOT_RIGHT_FRONT_FENDER:
            SetScreenRect(
                rect, icon->originX + 4, icon->originY + 1, icon->width - 9, icon->height - 4);
            break;
        case 4:
            SetScreenRect(
                rect, icon->originX + 6, icon->originY + 4, icon->width - 12, icon->height - 8);
            break;
        case SLOT_ROOF:
            SetScreenRect(
                rect, icon->originX + 4, icon->originY + 2, icon->width - 8, icon->height - 4);
            break;
        case SLOT_LEFT_REAR_FENDER:
        case SLOT_RIGHT_REAR_FENDER:
            SetScreenRect(
                rect, icon->originX + 2, icon->originY + 2, icon->width - 6, icon->height - 4);
            break;
        case 8:
            SetScreenRect(
                rect, icon->originX + 2, icon->originY + 2, icon->width - 4, icon->height - 4);
            break;
        case 9:
            SetScreenRect(
                rect, icon->originX + 4, icon->originY + 2, icon->width - 8, icon->height - 4);
            break;
        }
        g_apDecalSlotIcons[i]->style = SPRITE_STYLE_TINTED;
    }
}

/* Place the box of brush sizes, the palette, and the tool buttons under the decal. */
static inline void LayOutPaintShopButtons(void) {
    ScreenRect *rects = g_aPaintToolButtonRects;
    s32 row, column, x, y;

    rects[0].x = g_rcDecalCanvasFrame.x;
    rects[0].y = g_rcDecalCanvasFrame.y + g_rcDecalCanvasFrame.height + 6;
    rects[0].width = 32;
    rects[0].height = 94;
    g_aPaletteFrameRects[0].x = rects[0].x + rects[0].width + 2;
    g_aPaletteFrameRects[0].y = rects[0].y;
    g_aPaletteFrameRects[1].x = g_aPaletteFrameRects[0].x + g_aPaletteFrameRects[0].width + 14;
    g_aPaletteFrameRects[1].y = rects[0].y;
    y = rects[0].y + g_aPaletteFrameRects[0].height + 2;
    for (row = 0; row < 3; row++, y += 24) {
        for (column = 0, x = g_aPaletteFrameRects[0].x; column < 4; column++, x += 74) {
            SetScreenRect(&rects[1 + row * 4 + column], x, y, 72, 22);
        }
    }
}

/* Load the pictures of the tool buttons into their rectangles. */
static inline void LoadPaintToolIcons(void) {
    Sprite **icon;
    ScreenRect *rect;

    g_texBrushSizeRound.originY = g_aPaintToolButtonRects[0].y + 4;
    g_texBrushSizeSquare.originY = g_aPaintToolButtonRects[0].y + 4;
    g_texBrushSizeRound.originX = g_aPaintToolButtonRects[0].x;
    g_texBrushSizeSquare.originX = g_aPaintToolButtonRects[0].x;
    LoadTextureFromRom(&g_texBrushSizeRound);
    LoadTextureFromRom(&g_texBrushSizeSquare);
    for (icon = &g_apPaintToolIcons[1], rect = &g_aPaintToolButtonRects[1];
         rect < &g_aPaintToolButtonRects[PAINT_TOOLS];
         icon++, rect++) {
        (*icon)->width = TOOL_ICON_WIDTH;
        (*icon)->height = TOOL_ICON_HEIGHT;
        (*icon)->stripHeight = (*icon)->height;
        (*icon)->drawWidth = (*icon)->width;
        (*icon)->drawHeight = (*icon)->height;
        (*icon)->originX = rect->x + 4;
        (*icon)->originY = rect->y + 4;
        (*icon)->style = 0;
        LoadTextureFromRom(*icon);
    }
}

/* Load the pictures of the styles of the rectangle and the oval, all of one size. */
static inline void LoadStyleIcons(void) {
    Sprite **rect, **oval;

    for (rect = g_apRectStyleIcons, oval = g_apOvalStyleIcons; rect < &g_apOvalStyleIcons[0];
         rect++, oval++) {
        (*oval)->bitsPerTexel = 4;
        (*rect)->bitsPerTexel = (*oval)->bitsPerTexel;
        (*oval)->width = STYLE_ICON_WIDTH;
        (*rect)->width = (*oval)->width;
        (*oval)->height = STYLE_ICON_HEIGHT;
        (*rect)->height = (*oval)->height;
        (*oval)->stripHeight = (*rect)->height;
        (*rect)->stripHeight = (*rect)->height;
        (*oval)->drawWidth = (*rect)->width;
        (*rect)->drawWidth = (*rect)->width;
        (*oval)->drawHeight = (*rect)->height;
        (*rect)->drawHeight = (*rect)->height;
        LoadTextureFromRom(*rect);
        LoadTextureFromRom(*oval);
    }
}

/* Place the rectangles of the brush sizes, the swatches, and the keys of the keyboard. */
static inline void LayOutPaintShopControls(void) {
    s32 i, row, column, pitchX, pitchY;

    g_aBrushSizeButtonRects[0].x = g_texBrushSizeRound.originX;
    g_aBrushSizeButtonRects[0].y = g_texBrushSizeRound.originY;
    for (i = 1; i < BRUSH_SIZES; i++) {
        g_aBrushSizeButtonRects[i].x = g_aBrushSizeButtonRects[0].x;
        g_aBrushSizeButtonRects[i].y =
            g_aBrushSizeButtonRects[i - 1].y + g_aBrushSizeButtonRects[i - 1].height;
    }
    for (i = 0; i < 2; i++) {
        SetScreenRect(&g_aDecalSwatches[i].rect,
                      g_aPaletteFrameRects[0].x + 4 + 20 * i,
                      g_aPaletteFrameRects[0].y + 4,
                      20,
                      14);
    }
    for (i = 2; i < DECAL_SWATCHES; i++) {
        SetScreenRect(&g_aDecalSwatches[i].rect,
                      g_aPaletteFrameRects[1].x + 4 + 16 * (i - 2),
                      g_aPaletteFrameRects[1].y + 4,
                      16,
                      14);
    }
    ConvertPaletteToRgb();
    pitchX = g_texKeyboardHighlight.drawWidth + 4;
    for (row = 0; row < DECAL_GLYPHS / KEYBOARD_COLUMNS; row++) {
        pitchY = g_texKeyboardHighlight.drawHeight + 4;
        for (column = 0; column < KEYBOARD_COLUMNS; column++) {
            g_aDecalGlyphs[row * KEYBOARD_COLUMNS + column].x =
                g_texKeyboardBackground.originX + column * pitchX;
            g_aDecalGlyphs[row * KEYBOARD_COLUMNS + column].y =
                g_texKeyboardBackground.originY + row * pitchY;
        }
    }
}

/* Load and place everything of the paint shop on entering it. */
static void InitPaintShop(void) {
    VehicleModel *model;

    SetUpPaintShopCar();
    ResetHeap();
    osSyncPrintf("\nAllocating %d bytes (x2) for decal buffers...\n\n", DECAL_BUFFER_SIZE);
    g_pDecalUndoBuffer = AllocHeapAligned(DECAL_BUFFER_SIZE);
    /* The second buffer is not read again. */
    g_pDecalScratchTexels = AllocHeapAligned(DECAL_BUFFER_SIZE);
    LoadTextureFromRom(&g_texBackground);
    LoadTextureFromRom(&g_texLogo);
    LoadTextureFromRom(&g_texPaintWholeCar);
    g_texPaintWholeCar.style = SPRITE_STYLE_TINTED;
    LoadDecalSlotIcons();
    LayOutPaintShopButtons();
    LoadPaintToolIcons();
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
    LoadStyleIcons();
    model = g_pCurrentVehicleModelData;
    SelectDecalTexels(model, g_bDecalSlot);
    g_pDecalPalette = model->textures[model->materialTextures[g_bDecalSlot]].palette;
    g_pDecalSlotIcon = g_apDecalSlotIcons[g_bDecalSlot];
    LayOutPaintShopControls();
    g_rcDecalCanvas.width = g_bDecalWidth * 4;
    g_rcDecalCanvas.height = g_bDecalHeight * 4;
    g_rcDecalCanvas.x =
        g_rcDecalCanvasFrame.x + ((g_rcDecalCanvasFrame.width - g_rcDecalCanvas.width) >> 1);
    g_rcDecalCanvas.y =
        g_rcDecalCanvasFrame.y + ((g_rcDecalCanvasFrame.height - g_rcDecalCanvas.height) >> 1);
    g_bPaintShopInitialized = TRUE;
    gControllerPakState = 0;
    SetFadeTarget(1.0f, 0.2f);
}

/* Fill a rectangle of the screen. */
static inline void FillScreenRect(ScreenRect *rect, u8 red, u8 green, u8 blue) {
    DrawFilledRectHiRes(rect->x, rect->y, rect->width, rect->height, red, green, blue);
}

/* Draw a button as a frame in a color around a yellow face. */
static inline void DrawPaintShopButton(ScreenRect *rect, u8 red, u8 green, u8 blue) {
    FillScreenRect(rect, red, green, blue);
    DrawFilledRectHiRes(rect->x + 4, rect->y + 4, rect->width - 8, rect->height - 8, 0xFF, 0xCA, 0);
}

/* Draw the brush sizes of a brush with the size chosen in yellow. */
static inline void DrawBrushSizes(Sprite *sizes, ScreenRect *chosen) {
    /* The rows of the picture run from the bottom up. */
    DrawScaledTexturedRect(sizes,
                           0,
                           sizes->originY + sizes->height - chosen->y - chosen->height,
                           sizes->width,
                           chosen->height,
                           sizes->originX - 1,
                           chosen->y,
                           sizes->width,
                           chosen->height,
                           0xFF,
                           0xCA,
                           0);
}

/* Draw the background, the decal slots, the decal, the palette, and the tool buttons. */
static void DrawPaintShop(void) {
    VehicleModel *model;
    ScreenRect *chosen;
    s32 i;

    DrawSpriteTinted(&g_texBackground, 0x71, 0x0B, 0x40);
    DrawSpriteAtOrigin(&g_texLogo);
    SetTextSize(34);
    SetTextAlignCenter();
    SetTextStyleNormal();
    DrawText("%ryPAINT SHOP", 160, 38);
    for (i = 0; i < DECAL_SLOTS; i++) {
        if (i == g_bDecalSlot) {
            DrawSpriteTinted(g_apDecalSlotIcons[i], 0x20, 0xC8, 0xFF);
        } else {
            DrawSpriteTinted(g_apDecalSlotIcons[i], 0x20, 0x10, 0xFF);
        }
    }
    DrawSpriteTinted(&g_texPanel8Overlay, 0xFF, 0xFF, 0xFF);
    DrawSpriteTinted(&g_texPanel9Overlay, 0xC0, 0, 0);
    FillScreenRect(&g_rcDecalCanvasFrame, 0, 0, 0);
    model = g_pCurrentVehicleModelData;
    DrawTexturedTile(model->materialTextures[g_bDecalSlot], model->textures);
    EmitScaledTextureRect(g_bDecalWidth,
                          g_bDecalHeight,
                          g_rcDecalCanvas.x,
                          g_rcDecalCanvas.y,
                          g_rcDecalCanvas.width,
                          g_rcDecalCanvas.height);
    DrawDecalColorPalette();
    DrawPaintShopButton(&g_aPaintToolButtonRects[0], 0, 0, 0);
    if (g_texPaintToolBrush.style == 0) {
        DrawSpriteAtOrigin(&g_texBrushSizeRound);
    } else {
        DrawSpriteAtOrigin(&g_texBrushSizeSquare);
    }
    FillScreenRect(&g_aBrushSizeButtonRects[g_bBrushSize], 0, 0, 0);
    chosen = &g_aBrushSizeButtonRects[g_bBrushSize];
    if (g_texPaintToolBrush.style == 0) {
        DrawBrushSizes(&g_texBrushSizeRound, chosen);
    } else {
        DrawBrushSizes(&g_texBrushSizeSquare, chosen);
    }
    for (i = PAINT_TOOL_BRUSH; i < PAINT_TOOLS; i++) {
        if (i == g_bPaintTool) {
            DrawPaintShopButton(&g_aPaintToolButtonRects[i], 0x20, 0xC8, 0xFF);
        } else {
            DrawPaintShopButton(&g_aPaintToolButtonRects[i], 0, 0, 0);
        }
        DrawSpriteAtOrigin(g_apPaintToolIcons[i]);
    }
}

/* Draw the cursor of the tool in use over the decal. */
static inline void DrawToolCursor(void) {
    switch (g_bPaintTool) {
    case PAINT_TOOL_BRUSH:
        if (g_bEyedropperActive) {
            DrawTexturedSprite(&g_texPaintEyedropperCursor,
                               g_texPaintCursor.originX + 1,
                               g_texPaintCursor.originY - 24);
        } else {
            DrawTexturedSprite(&g_texPaintBrushCursor,
                               g_texPaintCursor.originX + 1,
                               g_texPaintCursor.originY - 24);
        }
        break;
    case PAINT_TOOL_FILL:
        if (g_bEyedropperActive) {
            DrawTexturedSprite(&g_texPaintEyedropperCursor,
                               g_texPaintCursor.originX,
                               g_texPaintCursor.originY - 24);
        } else {
            DrawTexturedSprite(
                &g_texPaintFillCursor, g_texPaintCursor.originX - 6, g_texPaintCursor.originY - 23);
        }
        break;
    case PAINT_TOOL_LINE:
    case PAINT_TOOL_RECTANGLE:
    case PAINT_TOOL_OVAL:
        if (g_bEyedropperActive) {
            DrawTexturedSprite(&g_texPaintEyedropperCursor,
                               g_texPaintCursor.originX,
                               g_texPaintCursor.originY - 24);
        } else {
            DrawTexturedSprite(&g_texPaintCrosshairCursor,
                               g_texPaintCursor.originX - 8,
                               g_texPaintCursor.originY - 8);
        }
        break;
    case PAINT_TOOL_TEXT:
        if (g_bDecalDragActive) {
            DrawSpriteAtOrigin(&g_texPaintCursor);
        } else if (g_bEyedropperActive) {
            DrawTexturedSprite(&g_texPaintEyedropperCursor,
                               g_texPaintCursor.originX,
                               g_texPaintCursor.originY - 24);
        } else {
            DrawTexturedSprite(&g_texPaintTextCursor,
                               g_texPaintCursor.originX - (g_texPaintTextCursor.drawWidth >> 1),
                               g_texPaintCursor.originY - g_texPaintTextCursor.drawHeight);
        }
        break;
    default:
        DrawSpriteAtOrigin(&g_texPaintCursor);
        break;
    }
}

/* Run the dialog of the controller pak for a load or a save, or show its error until dismissed. */
static inline void RunPaintShopPakDialog(u8 operation, u8 *done) {
    PlayerInputState *input;

    if (gControllerPakState == 0) {
        gControllerPakState = ProcessDecalSaveLoad(
            g_pCurrentVehicleModelData, g_bPaintShopPlayer, operation, FALSE, done);
        return;
    }
    DrawPakErrorMessage(gControllerPakState, operation, FALSE);
    input = &g_aPlayerInputState[g_bPaintShopPlayer];
    if (input->buttons & (INPUT_A | INPUT_B)) {
        TransferFlags(input, INPUT_A | INPUT_B);
        gControllerPakState = 0;
        g_bPaintTool = g_bPreviousPaintTool;
    }
}

/* Run the menu or the dialog of the tool in use while the cursor is off the decal. */
static inline void RunPaintShopMenus(u8 *done) {
    switch (g_bPaintTool) {
    case PAINT_TOOL_MIRROR:
        ProcessMirrorFlipMenu();
        break;
    case PAINT_TOOL_CLEAR:
        ProcessFillClearMenu();
        break;
    case PAINT_TOOL_LOAD:
        RunPaintShopPakDialog(PAINT_TOOL_LOAD, done);
        break;
    case PAINT_TOOL_SAVE:
        RunPaintShopPakDialog(PAINT_TOOL_SAVE, done);
        break;
    case PAINT_TOOL_EXIT:
        ProcessPaintShopExitConfirm();
        break;
    default:
        if (g_bStyleMenuOpen) {
            if (g_bPaintTool == PAINT_TOOL_BRUSH) {
                ProcessBrushStyleMenu();
            } else if (g_bPaintTool == PAINT_TOOL_TEXT) {
                ProcessTextStyleMenu();
            } else if (g_bPaintTool == PAINT_TOOL_RECTANGLE) {
                ProcessRectStyleMenu();
            } else if (g_bPaintTool == PAINT_TOOL_OVAL) {
                ProcessOvalStyleMenu();
            }
        } else if (g_bColorEditorOpen) {
            ProcessColorEditorDialog();
        } else {
            ProcessDecalEditorInput();
            UpdateCursorFromInput();
            DrawSpriteAtOrigin(&g_texPaintCursor);
        }
        break;
    }
}

/* Pick the color under the cursor with B, or close the keyboard and the started tool. */
static inline void HandlePaintShopB(void) {
    PlayerInputState *input = &g_aPlayerInputState[g_bPaintShopPlayer];

    if (!(input->buttons & INPUT_B)) {
        g_bEyedropperActive = FALSE;
        return;
    }
    if (IsCursorInRect(&g_rcDecalCanvas) && !g_bPaintToolStarted && !g_bDecalDragActive) {
        /* The rows of the decal run from the bottom up. */
        g_bDecalColor = GetPixel4Bit(
            (g_texPaintCursor.originX - g_rcDecalCanvas.x) >> 2,
            ((g_rcDecalCanvas.y + g_rcDecalCanvas.height) - g_texPaintCursor.originY) >> 2);
        g_bEyedropperActive = TRUE;
    } else if (g_bPaintToolStarted || g_bDecalDragActive) {
        TransferFlags(&g_aPlayerInputState[g_bPaintShopPlayer], INPUT_B);
        g_bDecalDragActive = FALSE;
        g_bPaintToolStarted = FALSE;
        g_bShowPreview3D = TRUE;
        g_bPaletteEnabled = TRUE;
        g_bDecalTextLength = 0;
        g_bDecalTextWidth = 0;
    }
}

void ProcessPaintShopScreen(void) {
    u8 done = FALSE;

    g_wSerialInterfaceIdle = FALSE;
    if (!g_bPaintShopInitialized) {
        InitPaintShop();
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix(g_dwScreenWidth, g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawPaintShop();
    if (g_bShowPreview3D) {
        g_dwNightMode = 0;
        SetupFog();
        SetupWeatherLights();
        RenderDecalPreview3D();
        SetupProjectionMatrix(g_dwScreenWidth, g_dwScreenHeight);
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    }
    if (IsCursorInRect(&g_rcDecalCanvas)) {
        ProcessDecalEditorInput();
        UpdateCursorFromInput();
        DrawToolCursor();
    } else {
        RunPaintShopMenus(&done);
    }
    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        DrawTimingBars();
    }
    FlushDisplayList();
    HandlePaintShopB();
    if (g_bPaintShopExiting && IsFadedToBlack()) {
        g_bPaintShopExiting = FALSE;
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        StartHighResFrame();
        ClearScreen(0, 0, 0);
        FlushDisplayList();
        ResetPaintShopState();
        g_bPaintShopInitialized = FALSE;
        gControllerPakState = 0;
        g_bPaintShopPreviewActive = FALSE;
        g_wSerialInterfaceIdle = TRUE;
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
    const char *scopeTexts[2] = {"CLEAR CURRENT DECAL", "CLEAR ALL DECALS"};
    const char *colorTexts[2] = {"TO SELECTED COLOR", "TO BODY COLOR"};
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
    SetTextSize(11);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    DrawText(scopeTexts[g_texPaintToolClear.style >> 1], 0x9E, 0x8E);
    DrawText(colorTexts[g_texPaintToolClear.style & CLEAR_TO_BODY_COLOR], 0x9E, 0x9B);
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
    const char *flipTexts[2] = {"FLIP CURRENT DECAL", "FLIP CURRENT DECAL"};
    const char *axisTexts[2] = {"HORIZONTALLY", "VERTICALLY"};
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
        DrawText(flipTexts[g_bStyleMenuCursor - 1], 0x9F, 0x87);
        DrawText(axisTexts[g_bStyleMenuCursor - 1], 0x9F, 0x94);
    } else if (!g_bDecalSlotHasMirror) {
        DrawText("MIRROR IS NOT APPLICABLE", 0x9F, 0x87);
        DrawText("TO THE CURRENT DECAL", 0x9F, 0x94);
    } else {
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
    VehicleModel *model;
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
    model = g_pCurrentVehicleModelData;
    s_pMirrorSource = model->textures[model->materialTextures[g_bPreviousDecalSlot]].image;
    g_bDecalSlot = g_bMirrorTargetPanel;
    /* The size and the palette stay those of the source. Each pair of slots shares them. */
    g_pDecalTexels = model->textures[model->materialTextures[g_bDecalSlot]].image;
    g_pDecalMask = model->decalMasks[g_bDecalSlot];
    g_pDecalSlotIcon = g_apDecalSlotIcons[g_bDecalSlot];
    SaveDecalUndo();
    for (y = 0; y < g_bDecalHeight; y++) {
        for (x = 0; x < g_bDecalWidth; x++) {
            SetPixel4Bit(g_bDecalWidth - x - 1, y, ReadPixel4Bit(s_pMirrorSource, x, y));
        }
    }
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
    osSyncPrintf("\nAllocating %d bytes for decal buffer...\n\n", DECAL_BUFFER_SIZE);
    g_pDecalUndoBuffer = AllocHeapAligned(DECAL_BUFFER_SIZE);
    LoadTextureFromRom(&g_texButtonA);
    LoadTextureFromRom(&g_texButtonB);
}

/* Draw the box of the progress of a transfer of the decals, with its empty bar. */
static inline void DrawPakProgressBox(s32 y, const char *text, s32 textX, s32 textY) {
    DrawDialogBox(0xD4, y, 0xD8, 0x56);
    SetTextAlignLeft();
    SetTextStyleNormal();
    SetTextSize(13);
    SetPakTextColors(FALSE);
    DrawText(text, textX, textY);
    DrawSunkenFrame(0xE6, y + 0x34, 0xB4, 0x14);
}

/* Copy the texels of a decal loaded into the undo buffer that its mask does not protect. A texel
 * copies the whole byte it shares with its neighbour. */
static inline void CopyUnmaskedTexels(u8 *texels, u8 *mask, s32 width, s32 height) {
    s32 x, y, offset;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (!(mask[y * ((width + 7) >> 3) + (x >> 3)] & (1 << ((x ^ 7) & 7)))) {
                /* Odd rows swap the halves of each 64-bit word, as the texture memory reads them.
                 */
                offset = ((((y & 1) << 3) ^ x) >> 1) + y * (width >> 1);
                texels[offset] = g_pDecalUndoBuffer[offset];
            }
        }
    }
}

/* Move the next decal between the model and the file on the controller pak, through the undo
 * buffer when it loads. */
static inline void TransferDecal(VehicleModel *model, OSPfs *pfs, u8 operation) {
    ModelTexture *texture = &model->textures[model->materialTextures[g_bPakDecalIndex]];
    s32 width = texture->width;
    s32 height = texture->height;
    u8 *mask;

    /* The doors move one row more than they have. */
    if (g_bPakDecalIndex == SLOT_LEFT_DOOR || g_bPakDecalIndex == SLOT_RIGHT_DOOR) {
        height++;
    }
    if (operation == PAINT_TOOL_LOAD) {
        g_dwPakChunkSize = (width * height) >> 1;
        g_dwPakError = osPfsReadWriteFile(pfs,
                                          g_dwPakFileNo,
                                          PFS_READ,
                                          g_dwPakFileOffset,
                                          (width * height) >> 1,
                                          g_pDecalUndoBuffer);
    } else {
        g_dwPakChunkSize = (width * height) >> 1;
        g_dwPakError = osPfsReadWriteFile(pfs,
                                          g_dwPakFileNo,
                                          PFS_WRITE,
                                          g_dwPakFileOffset,
                                          (width * height) >> 1,
                                          texture->image);
    }
    switch (g_dwPakError) {
    case 0:
        if (operation == PAINT_TOOL_LOAD) {
            osSyncPrintf("Loading decal %d...\n", g_bPakDecalIndex);
            texture = &model->textures[model->materialTextures[g_bPakDecalIndex]];
            mask = model->decalMasks[g_bPakDecalIndex];
            if (mask == NULL) {
                memcpy(texture->image, g_pDecalUndoBuffer, g_dwPakChunkSize);
            } else {
                CopyUnmaskedTexels(texture->image, mask, width, height);
            }
        } else {
            osSyncPrintf("Saving decal %d...\n", g_bPakDecalIndex);
        }
        g_dwPakFileOffset += g_dwPakChunkSize;
        break;
    case PFS_ERR_INCONSISTENT:
        g_dwPakError = osPfsChecker(pfs);
        if (g_dwPakError == PFS_ERR_NOPACK || g_dwPakError == PFS_ERR_CONTRFAIL ||
            g_dwPakError == PFS_ERR_INCONSISTENT) {
            g_bPakDecalIndex = 0;
            g_dwPakFileOffset = 0;
            g_dwPakChunkSize = 0;
            g_bPakDialogState = PAK_STATE_FINISH;
        } else {
            /* The offset of the failed chunk was not advanced, so the retry starts a chunk back. */
            g_bPakDecalIndex--;
            g_dwPakFileOffset -= g_dwPakChunkSize;
            g_dwPakError = 0;
        }
        break;
    case PFS_ERR_INVALID:
        g_dwPakError = PAK_ERROR_TRANSFER;
        break;
    }
}

/* Draw the labels and the pictures of the A and B buttons of a dialog of the controller pak. */
static inline void DrawPakButtons(s32 buttonY, u32 okTextX, u32 cancelTextX, s32 okX, s32 cancelX) {
    s32 textY;

    SetTextSize(10);
    textY = (buttonY + 0x12) >> 1;
    DrawText("%wwOK", okTextX >> 1, textY);
    DrawText("%wwCANCEL", cancelTextX >> 1, textY);
    DrawTexturedSprite(&g_texButtonA, okX, buttonY);
    DrawTexturedSprite(&g_texButtonB, cancelX, buttonY);
}

s32 ProcessDecalSaveLoad(VehicleModel *model, u8 player, u8 operation, u8 inCarSelect, u8 *done) {
    /* TOP GEAR DECALS in the font of the controller pak. */
    u8 noteName[16] = {0x2D,
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
    u8 extName[4] = {0};
    u8 pattern;
    s32 freeBytes, buttonY, textY, y;
    OSPfs *pfs;
    PlayerInputState *input;
    u16 *palette;

    g_wSerialInterfaceIdle = FALSE;
    if (!g_bPakDialogActive) {
        g_bPakDialogActive = TRUE;
        g_bPakRumbleSwapped = FALSE;
        g_bPakRetryNewPak = FALSE;
        g_dwPakError = 0;
        g_bPakDecalIndex = 0;
        g_dwPakFileOffset = 0;
        g_dwPakChunkSize = 0;
        g_bPakWaitFrames = 0;
        g_bPakDialogState = inCarSelect == 1 ? PAK_STATE_FADE_OUT : PAK_STATE_CONFIRM;
    }
    *done = FALSE;
    switch (g_bPakDialogState) {
    case PAK_STATE_FADE_OUT:
        FadeSfxTo(0.0f, PAK_FADE_TIME);
        FadeMusicTo(0.0f, PAK_FADE_TIME);
        g_bPakDialogState = PAK_STATE_WAIT_FADE;
        break;
    case PAK_STATE_WAIT_FADE:
        if (HasSfxFadeFinished() && g_bPakWaitFrames++ == PAK_FADE_WAIT_FRAMES) {
            g_bPakDialogState = PAK_STATE_INIT_PAK;
            g_bPakWaitFrames = 0;
        }
        break;
    case PAK_STATE_CONFIRM:
        /* The row of the buttons is placed by the width of the A button. */
        buttonY = 0x110 - g_texButtonA.width;
        DrawDialogBox(0xDC, 0xC8, 0xC8, 0x50);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextSize(14);
        SetPakTextColors(FALSE);
        DrawText(operation == PAINT_TOOL_LOAD ? "LOAD DECALS?" : "SAVE DECALS?", 0x77, 0x74);
        DrawPakButtons(buttonY, g_texButtonA.width + 0xF2, g_texButtonB.width + 0x136, 0xEC, 0x130);
        input = &g_aPlayerInputState[player];
        if (input->buttons & INPUT_A) {
            TransferFlags(input, INPUT_A);
            g_bPakDialogState = PAK_STATE_FADE_OUT;
        } else if (input->buttons & INPUT_B) {
            TransferFlags(input, INPUT_B);
            /* The dialog stays active and opens here again when it is next run. */
            *done = TRUE;
            g_bPaintTool = g_bPreviousPaintTool;
            return 0;
        }
        break;
    case PAK_STATE_INIT_PAK:
        osPfsIsPlug(&g_SiMesgQueue, &pattern);
        if (!(pattern & (1 << player))) {
            g_dwPakError = PFS_ERR_NOPACK;
            g_bPakDialogState = PAK_STATE_FINISH;
            break;
        }
        osSyncPrintf("\nInitializing controller pak...\n");
        pfs = &g_aPakPfs[player];
        g_dwPakError = osPfsInitPak(&g_SiMesgQueue, pfs, player);
        if (g_dwPakError == 0) {
            /* The rumble pak is cleared for the player of the menu rather than of the dialog. */
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            if (!g_abSavePakIdKnown[player]) {
                g_abSavePakIdKnown[player] = TRUE;
                memcpy(g_aabSavePakIds[player], pfs->id, sizeof(pfs->id));
                g_bPakDialogState = PAK_STATE_FIND_FILE;
            } else if (bcmp(g_aabSavePakIds[player], pfs->id, sizeof(pfs->id))) {
                memcpy(g_aabSavePakIds[player], pfs->id, sizeof(pfs->id));
                g_bPakDialogState = PAK_STATE_NEW_PAK;
            } else {
                g_bPakDialogState = PAK_STATE_FIND_FILE;
            }
        } else if (g_dwPakError == PFS_ERR_ID_FATAL) {
            /* A pak whose identity cannot be read may be a rumble pak. */
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) == 0) {
                osMotorStop(&g_aContPfs[player]);
                g_dwPakError = PAK_ERROR_RUMBLE_PAK;
                g_bPakRumbleSwapped = TRUE;
                g_bPakDialogState = PAK_STATE_REMOVE_RUMBLE_PAK;
            } else {
                g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
                g_dwPakError = 0;
                g_bPakDialogState = PAK_STATE_REPAIR_ID;
            }
        } else {
            g_abRumblePakPresent[g_dwMenuChoosingPlayer] = FALSE;
            g_bPakDialogState = PAK_STATE_FINISH;
        }
        break;
    case PAK_STATE_REPAIR_ID:
        g_dwPakError = osPfsRepairId(&g_aPakPfs[player]);
        if (g_dwPakError) {
            g_bPakDialogState = PAK_STATE_REMOVE_RUMBLE_PAK;
        } else {
            g_bPakDialogState = PAK_STATE_INIT_PAK;
        }
        break;
    case PAK_STATE_CONFIRM_OVERWRITE:
        buttonY = 0x11D - g_texButtonA.width;
        DrawDialogBox(0xD6, 0xBB, 0xD4, 0x6A);
        SetTextAlignLeft();
        SetTextStyleNormal();
        SetTextSize(13);
        SetPakTextColors(FALSE);
        DrawText("OK TO OVERWRITE", 0x72, 0x6C);
        DrawText("SAVED DECALS?", 0x72, 0x7A);
        /* The label of the B button is placed by its height. */
        DrawPakButtons(
            buttonY, g_texButtonA.width + 0xEE, g_texButtonB.height + 0x13A, 0xE8, 0x134);
        input = &g_aPlayerInputState[player];
        if (input->buttons & INPUT_A) {
            TransferFlags(input, INPUT_A);
            g_bPakDialogState = PAK_STATE_TRANSFER_DECALS;
        } else if (input->buttons & INPUT_B) {
            TransferFlags(input, INPUT_B);
            /* The error is clear, so the dialog finishes as if the decals were saved. */
            g_bPakDialogState = PAK_STATE_FINISH;
        }
        break;
    case PAK_STATE_FIND_FILE:
        g_dwPakFileOffset = 0;
        g_dwPakChunkSize = 0;
        g_bPakDecalIndex = 0;
        g_bPakFileNeedsCreate = FALSE;
        pfs = &g_aPakPfs[player];
        g_bPakWaitFrames = 0;
        g_dwPakError = osPfsFindFile(
            pfs, DECAL_FILE_COMPANY, DECAL_FILE_GAME, noteName, extName, &g_dwPakFileNo);
        if (g_dwPakError == 0) {
            if (operation == PAINT_TOOL_LOAD) {
                y = inCarSelect == 1 ? 0xDB : 0xC5;
                DrawPakProgressBox(y, "LOADING DECALS...", 0x72, (y + 30) >> 1);
                g_bPakDialogState = PAK_STATE_TRANSFER_DECALS;
            } else {
                g_bPakDialogState = PAK_STATE_CONFIRM_OVERWRITE;
            }
        } else if (g_dwPakError == PFS_ERR_INVALID) {
            if (operation == PAINT_TOOL_LOAD) {
                g_bPakDialogState = PAK_STATE_FINISH;
            } else if (osPfsFreeBlocks(pfs, &freeBytes)) {
                /* Any error of the count of the free pages is reported as a failed controller. */
                g_dwPakError = PFS_ERR_CONTRFAIL;
                g_bPakDialogState = PAK_STATE_FINISH;
            } else if (freeBytes < DECAL_FILE_SIZE) {
                g_dwPakError = PFS_DATA_FULL;
                g_bPakDialogState = PAK_STATE_FINISH;
            } else {
                DrawPakProgressBox(0xC5, "SAVING DECALS...", 0x73, 0x71);
                g_bPakFileNeedsCreate = TRUE;
                g_bPakDialogState = PAK_STATE_TRANSFER_DECALS;
                g_bPakWaitFrames = 0;
            }
        } else if (g_dwPakError == PFS_ERR_INCONSISTENT) {
            g_dwPakError = osPfsChecker(pfs);
            if (g_dwPakError) {
                g_bPakDialogState = PAK_STATE_FINISH;
            } else {
                g_bPakDialogState = PAK_STATE_FIND_FILE;
            }
        } else if (g_dwPakError == PFS_ERR_NEW_PACK) {
            g_bPakDialogState = PAK_STATE_INIT_PAK;
            g_dwPakError = 0;
        } else {
            g_bPakDialogState = PAK_STATE_FINISH;
        }
        break;
    case PAK_STATE_TRANSFER_DECALS:
        if (inCarSelect == 1) {
            y = 0xDB;
            textY = 0x7C;
        } else {
            y = 0xC5;
            textY = 0x71;
        }
        if (operation == PAINT_TOOL_LOAD) {
            DrawPakProgressBox(y, "LOADING DECALS...", 0x72, textY);
        } else {
            DrawPakProgressBox(y, "SAVING DECALS...", 0x73, textY);
        }
        if (g_bPakWaitFrames++ < PAK_TRANSFER_DELAY_FRAMES) {
            break;
        }
        if (g_bPakFileNeedsCreate) {
            g_bPakFileNeedsCreate = FALSE;
            pfs = &g_aPakPfs[player];
            g_dwPakError = osPfsAllocateFile(pfs,
                                             DECAL_FILE_COMPANY,
                                             DECAL_FILE_GAME,
                                             noteName,
                                             extName,
                                             DECAL_FILE_SIZE,
                                             &g_dwPakFileNo);
            switch (g_dwPakError) {
            case PFS_ERR_INCONSISTENT:
                g_dwPakError = osPfsChecker(pfs);
                if (!g_dwPakError) {
                    g_bPakDialogState = PAK_STATE_INIT_PAK;
                    return 0;
                }
                /* The dialog goes back to its confirmation with the error. */
                g_bPakDialogState = PAK_STATE_CONFIRM;
                return g_dwPakError;
            case PFS_ERR_INVALID:
            case PFS_ERR_EXIST:
                g_dwPakError = PFS_ERR_INCONSISTENT;
                break;
            case PFS_ERR_NEW_PACK:
                g_bPakDialogState = PAK_STATE_INIT_PAK;
                break;
            }
            if (g_dwPakError) {
                if (g_dwPakError == PFS_ERR_NEW_PACK) {
                    g_dwPakError = 0;
                    g_bPakDialogState = PAK_STATE_INIT_PAK;
                    *done = FALSE;
                    return 0;
                }
                g_bPakDialogState = PAK_STATE_FINISH;
                break;
            }
        }
        if (g_bPakDecalIndex < DECAL_SLOTS) {
            TransferDecal(model, &g_aPakPfs[player], operation);
            if (g_dwPakError) {
                if (g_dwPakError == PFS_ERR_NEW_PACK) {
                    g_dwPakError = 0;
                    g_bPakDialogState = PAK_STATE_NEW_PAK;
                    g_bPakRetryNewPak = TRUE;
                    *done = FALSE;
                    return 0;
                }
                g_bPakDialogState = PAK_STATE_FINISH;
            } else {
                DrawFilledRectHiRes(0xE7,
                                    y + 0x35,
                                    g_dwPakFileOffset * DECAL_PROGRESS_WIDTH / DECAL_PROGRESS_FULL,
                                    0x12,
                                    0,
                                    0,
                                    0xC0);
                g_bPakDecalIndex++;
            }
        } else {
            g_bPakDecalIndex = 0;
            DrawFilledRectHiRes(0xE7, y + 0x35, DECAL_PROGRESS_WIDTH, 0x12, 0, 0, 0xC0);
            g_bPakDialogState = PAK_STATE_TRANSFER_PALETTE;
        }
        break;
    case PAK_STATE_TRANSFER_PALETTE:
        pfs = &g_aPakPfs[player];
        if (operation == PAINT_TOOL_LOAD) {
            osSyncPrintf("Loading color palette...\n");
            g_dwPakError = osPfsReadWriteFile(
                pfs,
                g_dwPakFileNo,
                PFS_READ,
                g_dwPakFileOffset,
                DECAL_PALETTE_BYTES,
                (u8 *)model->textures[model->materialTextures[VEHICLE_MATERIAL_PAINT]].palette);
            if (!g_dwPakError) {
                /* The lights take the first two colors of the paint. */
                palette = model->textures[model->materialTextures[VEHICLE_MATERIAL_PAINT]].palette;
                model->textures[model->materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]].palette[0] =
                    palette[0];
                model->textures[model->materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]].palette[1] =
                    palette[1];
                model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette[0] =
                    palette[0];
                model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]].palette[1] =
                    palette[1];
                if (!inCarSelect) {
                    ConvertPaletteToRgb();
                    SaveDecalUndo();
                }
            }
        } else {
            osSyncPrintf("Saving color palette...\n");
            g_dwPakError = osPfsReadWriteFile(
                pfs,
                g_dwPakFileNo,
                PFS_WRITE,
                g_dwPakFileOffset,
                DECAL_PALETTE_BYTES,
                (u8 *)model->textures[model->materialTextures[VEHICLE_MATERIAL_PAINT]].palette);
        }
        if (g_dwPakError == 0) {
            if (operation == PAINT_TOOL_LOAD) {
                osSyncPrintf("Color palette loaded successfully!\n\n");
            } else {
                osSyncPrintf("Color palette saved successfully!\n\n");
            }
            g_bPakDialogState = PAK_STATE_DONE;
            if (!inCarSelect) {
                g_bDecalEditCount = 0;
            }
        } else if (g_dwPakError == PFS_ERR_INCONSISTENT) {
            g_dwPakError = osPfsChecker(pfs);
            if (g_dwPakError == PFS_ERR_NOPACK || g_dwPakError == PFS_ERR_CONTRFAIL ||
                g_dwPakError == PFS_ERR_INCONSISTENT) {
                g_bPakDialogState = PAK_STATE_FINISH;
            } else {
                /* The size of the last chunk is that of the last decal, not of the palette. */
                g_dwPakFileOffset -= g_dwPakChunkSize;
                g_dwPakError = 0;
                g_bPakDialogState = PAK_STATE_TRANSFER_PALETTE;
            }
        }
        if (!g_dwPakError) {
            return 0;
        }
        osSyncPrintf("err_code = %d\n\n", g_dwPakError);
        if (g_dwPakError == PFS_ERR_NEW_PACK) {
            g_bPakDialogState = PAK_STATE_NEW_PAK;
            g_bPakRetryNewPak = TRUE;
            *done = FALSE;
            g_dwPakError = 0;
            return 0;
        }
        g_bPakDialogState = PAK_STATE_FINISH;
        *done = FALSE;
        break;
    case PAK_STATE_NEW_PAK:
        y = inCarSelect == 1 ? 0xD3 : 0xAB;
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(12);
        DrawDialogBox(0xAF, y, 0x122, 0x89);
        SetPakTextColors(inCarSelect);
        textY = (y + 0x20) >> 1;
        DrawText("A NEW CONTROLLER PAK", 0x5F, textY);
        DrawText("WAS INSERTED.", 0x5F, textY + 13);
        DrawText("THIS CONTROLLER PAK", 0x5F, textY + 32);
        DrawText("WILL BE USED.", 0x5F, textY + 45);
        input = &g_aPlayerInputState[player];
        if (input->buttons & PAK_DISMISS_BUTTONS) {
            TransferFlags(input, PAK_DISMISS_BUTTONS);
            if (g_bPakRetryNewPak) {
                g_bPakRetryNewPak = FALSE;
                g_bPakDialogState = PAK_STATE_INIT_PAK;
            } else {
                g_bPakDialogState = PAK_STATE_FIND_FILE;
            }
        }
        break;
    case PAK_STATE_REMOVE_RUMBLE_PAK:
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(12);
        SetPakTextColors(inCarSelect);
        if (g_dwPakError == PAK_ERROR_RUMBLE_PAK) {
            y = inCarSelect == 1 ? 0xC8 : 0xA0;
            buttonY = y - g_texButtonA.width + 0x98;
            DrawDialogBox(0x7A, y, 0x18C, 0xA0);
            textY = (y + 0x20) >> 1;
            DrawText("PLEASE REMOVE THE RUMBLE PAK", 0x45, textY);
            DrawText("AND INSERT THE CONTROLLER PAK", 0x45, textY + 13);
            DrawText("INTO CONTROLLER.  PRESS THE A", 0x45, textY + 26);
            DrawText("BUTTON WHEN READY.", 0x45, textY + 39);
            DrawPakButtons(
                buttonY, g_texButtonA.width + 0xDE, g_texButtonB.width + 0x14A, 0xD8, 0x144);
        }
        input = &g_aPlayerInputState[player];
        if (g_dwPakError != PAK_ERROR_RUMBLE_PAK) {
            /* A pak that cannot be repaired shows nothing and waits for a button. */
            if (input->buttons & PAK_DISMISS_BUTTONS) {
                TransferFlags(input, PAK_DISMISS_BUTTONS);
                g_bPakDialogState = PAK_STATE_FINISH;
            }
        } else if (input->buttons & INPUT_A) {
            TransferFlags(input, INPUT_A);
            g_dwPakError = 0;
            g_bPakDialogState = PAK_STATE_INIT_PAK;
        } else if (input->buttons & INPUT_B) {
            TransferFlags(input, INPUT_B);
            g_dwPakError = 0;
            g_bPakDialogState = PAK_STATE_FINISH;
            if (inCarSelect == 1) {
                g_dwPakError = PAK_DIALOG_CANCELLED;
            }
        }
        break;
    case PAK_STATE_DONE:
        g_bPakDecalIndex = 0;
        g_dwPakFileOffset = 0;
        g_dwPakChunkSize = 0;
        g_bPakWaitFrames = 0;
        if (!inCarSelect) {
            g_bPakDialogState = PAK_STATE_FINISH;
            break;
        }
        if (!g_bPakRumbleSwapped) {
            g_bPakDialogState = PAK_STATE_FINISH;
            break;
        }
        DrawDialogBox(0x73, 0xCC, 0x19A, 0x98);
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(12);
        SetPakTextColors(TRUE);
        DrawText("IF THE RUMBLE PAK IS TO BE USED,", 0x41, 0x76);
        DrawText("REMOVE THE CONTROLLER PAK AND", 0x41, 0x83);
        DrawText("INSERT THE RUMBLE PAK INTO THE", 0x41, 0x90);
        DrawText("CONTROLLER.  PRESS THE A BUTTON", 0x41, 0x9D);
        DrawText("TO CONTINUE.", 0x41, 0xAA);
        input = &g_aPlayerInputState[player];
        if (input->buttons & (INPUT_Z | INPUT_A)) {
            TransferFlags(input, INPUT_Z | INPUT_A);
            g_bPakRumbleSwapped = FALSE;
            g_dwPakError = 0;
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[player], player) == 0) {
                g_abRumblePakPresent[player] = TRUE;
            }
            g_bPakDialogState = PAK_STATE_FINISH;
        }
        break;
    case PAK_STATE_FINISH:
        g_bPakDialogActive = FALSE;
        FadeSfxTo(1.0f, PAK_FADE_TIME);
        FadeMusicTo(1.0f, PAK_FADE_TIME);
        g_wSerialInterfaceIdle = TRUE;
        if (!g_dwPakError) {
            *done = TRUE;
            if (!inCarSelect) {
                g_bPaintTool = g_bPreviousPaintTool;
            }
            return 0;
        }
        if (!inCarSelect) {
            input = &g_aPlayerInputState[player];
            if (input->buttons & PAK_DISMISS_BUTTONS) {
                TransferFlags(input, PAK_DISMISS_BUTTONS);
                g_bPaintTool = g_bPreviousPaintTool;
            }
        }
        *done = FALSE;
        return g_dwPakError;
    }
    return 0;
}

/* The columns two glyphs side by side overlap, the lesser of how far each may move to the other. */
static inline s32 GlyphOverlap(DecalGlyph *previous, DecalGlyph *next) {
    return previous->kernRight < next->kernLeft ? previous->kernRight : next->kernLeft;
}

void ProcessTextInputKeyboard(void) {
    Sprite *highlight = &g_texKeyboardHighlight;
    PlayerInputState *input;
    DecalGlyph *key;
    s32 i, x, y, width;
    u8 length;

    g_bShowPreview3D = FALSE;
    g_bPaletteEnabled = FALSE;
    DrawSpriteTinted(&g_texKeyboardBackground, 0xE0, 0xE0, 0xE0);
    if (highlight->style) {
        highlight->style = FALSE;
        if (g_wScreenFrameCount > 0) {
            g_wScreenFrameCount--;
        }
        if (g_wScreenFrameCount == 0) {
            DrawSpriteTinted(highlight, 0x20, 0xC8, 0xFF);
        } else {
            DrawSpriteTinted(highlight, 0xFF, 0xEB, 0);
        }
    }
    DrawSpriteAtOrigin(&g_texKeyboardPanel);
    x = g_texKeyboardPanel.originX;
    y = g_texKeyboardPanel.originY + g_texKeyboardPanel.height + 0x20;
    DrawSpriteRegion(&g_texKeyboardBackground, x, y, 0xC8, 0x30, 0xE0, 0xE0, 0xE0);
    DrawRectOutline(x, y, 0xC8, 0x30, 2, 0xFF, 0xFF, 0xFF);
    if (IsCursorInButton(&g_texKeyboardBackground)) {
        for (i = 0; i < DECAL_GLYPHS; i++) {
            key = &g_aDecalGlyphs[i];
            if (IsCursorOnKeyboardKey(key)) {
                g_bKeyboardHoverKey = i;
                highlight->style = TRUE;
                highlight->originX = key->x;
                highlight->originY = key->y;
            }
            if (!IsCursorInButton(highlight)) {
                continue;
            }
            input = &g_aPlayerInputState[g_bPaintShopPlayer];
            if (!(input->buttons & (INPUT_Z | INPUT_A))) {
                continue;
            }
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
                if (g_apDecalTextGlyphs[g_bDecalTextLength - 1] == NULL) {
                    length = 0;
                    g_bDecalTextLength = 0;
                    g_bDecalTextWidth = 0;
                } else {
                    length = --g_bDecalTextLength;
                    if (length == 0) {
                        g_bDecalTextWidth -= g_apDecalTextGlyphs[0]->width;
                    } else {
                        g_bDecalTextWidth = g_bDecalTextWidth - g_apDecalTextGlyphs[length]->width +
                                            GlyphOverlap(g_apDecalTextGlyphs[length - 1],
                                                         g_apDecalTextGlyphs[length]) -
                                            2;
                    }
                }
                g_szDecalText[length] = '\0';
                g_apDecalTextGlyphs[length] = NULL;
            } else {
                key = &g_aDecalGlyphs[g_bKeyboardHoverKey];
                width = key->width + g_bDecalTextWidth;
                if (width + 2 < g_bDecalWidth) {
                    length = g_bDecalTextLength;
                    g_szDecalText[length] = key->character;
                    g_apDecalTextGlyphs[length] = key;
                    if (length == 0) {
                        g_bDecalTextWidth = width;
                    } else {
                        g_bDecalTextWidth = width -
                                            GlyphOverlap(g_apDecalTextGlyphs[length - 1],
                                                         g_apDecalTextGlyphs[length]) +
                                            2;
                    }
                    length++;
                    /* At the most characters, the terminator and the key go past the text. */
                    g_szDecalText[length] = '\0';
                    g_bDecalTextLength = length;
                    if (length > DECAL_TEXT_LENGTH) {
                        g_bDecalTextLength = DECAL_TEXT_LENGTH;
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
