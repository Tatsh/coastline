#include "controllerscreen.h"

#include "heap.h"
#include "options.h"
#include "system.h"
#include "text.h"
#include "vehicle.h"

/* The size of the labels. */
#define LABEL_TEXT_SIZE 8

/* The rectangle of the pictures of the first four layouts. The fifth sits a little higher. */
#define PICTURE_X 0x6F
#define PICTURE_Y 0x55
#define PICTURE_WIDTH 0x68
#define PICTURE_HEIGHT 0x30

/* The colours of the top and the bottom of the background of the screen. */
#define BACKGROUND_TOP 0x08, 0x00, 0x00
#define BACKGROUND_BOTTOM 0xA0, 0x80, 0x20

/*
 * Draw a layout picture twice in black, as a shadow, and once in white over it, all in the same
 * place.
 */
#define DRAW_LAYOUT_PICTURE(texture, x, y)                                                         \
    {                                                                                              \
        DrawTexturedRect(texture,                                                                  \
                         x,                                                                        \
                         y,                                                                        \
                         PICTURE_WIDTH,                                                            \
                         PICTURE_HEIGHT,                                                           \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0xFF,                                                                     \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0xFF,                                                                     \
                         TEXRECT_BLEND_COLOR_THROUGH_TEXTURE);                                     \
        DrawTexturedRect(texture,                                                                  \
                         x,                                                                        \
                         y,                                                                        \
                         PICTURE_WIDTH,                                                            \
                         PICTURE_HEIGHT,                                                           \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0xFF,                                                                     \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0x00,                                                                     \
                         0xFF,                                                                     \
                         TEXRECT_BLEND_COLOR_THROUGH_TEXTURE);                                     \
        DrawTexturedRect(texture,                                                                  \
                         x,                                                                        \
                         y,                                                                        \
                         PICTURE_WIDTH,                                                            \
                         PICTURE_HEIGHT,                                                           \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         0xFF,                                                                     \
                         TEXRECT_BLEND_COLOR_THROUGH_TEXTURE);                                     \
    }

void DrawControllerLayoutDiagram(s32 layout) {
    SetTextSize(LABEL_TEXT_SIZE);
    SetTextStyleNormal();
    switch (layout) {
    case 0:
        DRAW_LAYOUT_PICTURE(&g_texControllerLayout0, PICTURE_X, PICTURE_Y);
        SetTextAlignLeft();
        DrawText("%wwACCELERATE", 0xC9, 0x7F);
        DrawText("%wwPLUS BRAKE = E-BRAKE", 0xC1, 0x87);
        DrawText("%wwCHANGE VIEW", 0xD7, 0x68);
        DrawText("%wwGEAR UP", 0xC7, 0x56);
        SetTextAlignRight();
        DrawText("%wwSTEERING / REVERSE", 0x6B, 0x63);
        DrawText("%wwGEAR DOWN", 0x75, 0x83);
        SetTextAlignCenter();
        DrawText("%wwBRAKE", 0x9C, 0x56);
        break;
    case 1:
        DRAW_LAYOUT_PICTURE(&g_texControllerLayout1, PICTURE_X, PICTURE_Y);
        SetTextAlignLeft();
        DrawText("%wwACCELERATE", 0xC9, 0x80);
        DrawText("%wwCHANGE VIEW", 0xD7, 0x68);
        DrawText("%wwGEAR UP", 0xC7, 0x56);
        DrawText("%wwE-BRAKE", 0xD3, 0x76);
        SetTextAlignRight();
        DrawText("%wwSTEERING / REVERSE", 0x6B, 0x63);
        DrawText("%wwGEAR DOWN", 0x75, 0x83);
        SetTextAlignCenter();
        DrawText("%wwBRAKE", 0x9C, 0x56);
        break;
    case 2:
        DRAW_LAYOUT_PICTURE(&g_texControllerLayout2, PICTURE_X, PICTURE_Y);
        SetTextAlignLeft();
        DrawText("%wwACCELERATE", 0xC9, 0x7F);
        DrawText("%wwPLUS BRAKE = E-BRAKE", 0xC1, 0x87);
        DrawText("%wwCHANGE VIEW", 0xD7, 0x68);
        DrawText("%wwGEAR UP", 0xC7, 0x56);
        SetTextAlignRight();
        DrawText("%wwSTEERING, REVERSE", 0x6B, 0x7D);
        DrawText("%wwGEAR DOWN", 0x74, 0x58);
        SetTextAlignCenter();
        DrawText("%wwBRAKE", 0x9C, 0x56);
        break;
    case 3:
        DRAW_LAYOUT_PICTURE(&g_texControllerLayout3, PICTURE_X, PICTURE_Y);
        SetTextAlignLeft();
        DrawText("%wwGEAR DOWN", 0xC9, 0x80);
        DrawText("%wwCHANGE VIEW", 0xD8, 0x67);
        SetTextAlignRight();
        DrawText("%wwACCELERATE / REVERSE,", 0x7C, 0x59);
        DrawText("%wwSTEERING ", 0x7C, 0x61);
        DrawText("%wwGEAR UP/DOWN", 0x6E, 0x72);
        DrawText("%wwBRAKE", 0x77, 0x81);
        DrawText("%wwPLUS ACCELERATE = E-BRAKE ", 0x77, 0x89);
        SetTextAlignCenter();
        DrawText("%wwGEAR UP", 0x9C, 0x56);
        break;
    case 4:
        DRAW_LAYOUT_PICTURE(&g_texControllerLayout4, PICTURE_X - 1, PICTURE_Y - 4);
        SetTextAlignLeft();
        DrawText("%wwBRAKE", 0xC7, 0x59);
        DrawText("%wwACCELERATE", 0xD0, 0x6E);
        DrawText("%wwPLUS BRAKE = E-BRAKE", 0xC8, 0x76);
        SetTextAlignCenter();
        DrawText("%wwCHANGE VIEW", 0xA7, 0x55);
        SetTextAlignRight();
        DrawText("%wwSTEERING", 0x72, 0x7C);
        DrawText("%wwREVERSE", 0x7F, 0x57);
        DrawText("%wwGEAR UP/DOWN", 0x74, 0x65);
        SetTextAlignCenter();
        break;
    }
}

void ProcessControllerConfigScreen(void) {
    /* The state of the screen: 0 before it starts, 1 while it runs. */
    /**
     * @ghidraAddress USA: 0x80272670
     * @ghidraAddress JPN: 0x80274190
     * @ghidraAddress EUR: 0x802746c8
     */
    static s32 g_dwControllerScreenInitState = 0;
    /**
     * @ghidraAddress USA: 0x80316440
     * @ghidraAddress JPN: 0x80316d50
     * @ghidraAddress EUR: 0x80318120
     */
    static s32 g_dwControllerScreenItemCount;
    /* The layout under the cursor. */
    /**
     * @ghidraAddress USA: 0x80316444
     * @ghidraAddress JPN: 0x80316d54
     * @ghidraAddress EUR: 0x80318124
     */
    static s32 g_eControllerScreenSelection;

    if (g_dwControllerScreenInitState <= 0) {
        ResetAndInitDisplay(g_apControllerScreenItems, &g_dwControllerScreenItemCount);
        LoadAndDecompressAsset(&g_texControllerLayout0, AllocHeapAligned);
        LoadAndDecompressAsset(&g_texControllerLayout1, AllocHeapAligned);
        LoadAndDecompressAsset(&g_texControllerLayout2, AllocHeapAligned);
        LoadAndDecompressAsset(&g_texControllerLayout3, AllocHeapAligned);
        LoadAndDecompressAsset(&g_texControllerLayout4, AllocHeapAligned);

        g_pMenuTitleImage = &g_TitleBarTextureDesc;
        g_dwMenuTitleImageX = 22;
        g_dwMenuTitleImageY = 9;
        g_dwMenuTitleImageWidth = 280;
        g_dwMenuTitleImageHeight = 42;
        g_pfnScreenPostDrawCallback = DrawControllerLayoutDiagram;

        /* Open on the layout the player already uses. */
        if (g_dwControllerScreenInitState == 0) {
            g_eControllerScreenSelection =
                g_aVehicles[g_dwScreenPlayerIdx].profile->controllerLayout;
        }
        g_dwControllerScreenInitState = 1;
    }

    switch (ProcessScreenState(g_dwScreenPlayerIdx != 0 ? "Controller 2" : "Controller 1",
                               g_dwControllerScreenItemCount,
                               g_apControllerScreenItems,
                               &g_eControllerScreenSelection,
                               NULL,
                               BACKGROUND_TOP,
                               BACKGROUND_BOTTOM)) {
    case MENU_RESULT_CHOSEN:
        g_aVehicles[g_dwScreenPlayerIdx].profile->controllerLayout = g_eControllerScreenSelection;
        /* Either way, go back to the options. */
        /* fall through */
    case MENU_RESULT_CANCELLED:
        SetCurrentScreenCallback(ProcessOptionsMenu);
        g_dwControllerScreenInitState = 0;
        break;
    }
}
