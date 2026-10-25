#pragma once

/**
 * @file
 * The interpreter of the display lists of the game in the PC port. Each handler runs one command
 * and returns the command to run next.
 */

/** A command of a display list of the game. */
struct GbiCommand {
    unsigned w0; /*!< The first word, the opcode in its high byte. */
    union {
        unsigned w1;         /*!< The second word. */
        float f1;            /*!< The second word as a float. */
        GbiCommand *pTarget; /*!< The second word as the display list a branch or a call runs. */
    };
};

/** The number of entries of g_apGbiDisplayListReturnStack. */
#define GBI_DISPLAY_LIST_STACK_SIZE 10

/** The depth g_dwGbiMatrixStackDepth takes when a pop empties it. */
#define GBI_MATRIX_STACK_RESET_DEPTH 10

/**
 * The scale of the width of a texture rectangle.
 *
 * @ghidraAddress BRD3D: 0x100a79e8
 * @ghidraAddress BRGlide: 0x100a9a54
 */
extern float g_fTexRectScaleW;

/**
 * The depth of a texture rectangle.
 *
 * @ghidraAddress BRD3D: 0x104c5174
 * @ghidraAddress BRGlide: 0x105d17c4
 */
extern float g_fTexRectDepth;

/**
 * The first word of the combine mode set last.
 *
 * @ghidraAddress BRD3D: 0x104c5158
 * @ghidraAddress BRGlide: 0x105d17ac
 */
extern unsigned g_dwGbiCombineModeHi;

/**
 * The second word of the combine mode set last.
 *
 * @ghidraAddress BRD3D: 0x104c515c
 * @ghidraAddress BRGlide: 0x105d17b0
 */
extern unsigned g_dwGbiCombineModeLo;

/**
 * The geometry mode.
 *
 * @ghidraAddress BRD3D: 0x104c5178
 * @ghidraAddress BRGlide: 0x105d17c8
 */
extern unsigned g_dwGbiGeometryMode;

/**
 * The geometry mode before the last change.
 *
 * @ghidraAddress BRD3D: 0x104c517c
 * @ghidraAddress BRGlide: 0x105d17cc
 */
extern unsigned g_dwGbiPrevGeometryMode;

/**
 * The depth of the stack of matrices.
 *
 * @ghidraAddress BRD3D: 0x100a79dc
 * @ghidraAddress BRGlide: 0x100a9a50
 */
extern int g_dwGbiMatrixStackDepth;

/**
 * The number of entries of g_apGbiDisplayListReturnStack in use.
 *
 * @ghidraAddress BRD3D: 0x104c01a4
 * @ghidraAddress BRGlide: 0x105ccfe8
 */
extern int g_dwGbiDisplayListStackDepth;

/**
 * The commands that follow the calls of display lists, to return to.
 *
 * @ghidraAddress BRD3D: 0x104c16a8
 * @ghidraAddress BRGlide: 0x105ce2e8
 */
extern GbiCommand *g_apGbiDisplayListReturnStack[GBI_DISPLAY_LIST_STACK_SIZE];

/**
 * Choose the routine that submits vertices for a combine mode.
 *
 * @param dwCombineHi The first word of the combine mode.
 * @param dwCombineLo The second word of the combine mode.
 * @ghidraAddress BRD3D: 0x1001c820
 * @ghidraAddress BRGlide: 0x1001e7a0
 */
void SelectVertexSubmitterForCombineMode(unsigned dwCombineHi, unsigned dwCombineLo);

/**
 * Apply the geometry mode after a change.
 *
 * @ghidraAddress BRD3D: 0x1001e7c0
 * @ghidraAddress BRGlide: 0x1001fd70
 */
void ApplyGeometryModeChanges(void);

/**
 * Set the scale of the width of a texture rectangle.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cd60
 * @ghidraAddress BRGlide: 0x1001eb10
 */
GbiCommand *HandleGbiSetTexRectW(GbiCommand *pCommand);

/**
 * Set the depth of a texture rectangle.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cd80
 * @ghidraAddress BRGlide: 0x1001eb30
 */
GbiCommand *HandleGbiSetTexRectDepth(GbiCommand *pCommand);

/**
 * Set the combine mode and choose the routine that submits vertices for it.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001c7f0
 * @ghidraAddress BRGlide: 0x1001e770
 */
GbiCommand *HandleGbiSetCombine(GbiCommand *pCommand);

/**
 * Clear bits of the geometry mode.
 *
 * @param pCommand The command, the bits in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001e790
 * @ghidraAddress BRGlide: 0x1001fd40
 */
GbiCommand *HandleGbiClearGeometryMode(GbiCommand *pCommand);

/**
 * Set bits of the geometry mode.
 *
 * @param pCommand The command, the bits in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020f20
 * @ghidraAddress BRGlide: 0x100211e0
 */
GbiCommand *HandleGbiSetGeometryMode(GbiCommand *pCommand);

/**
 * Pop the stack of matrices. A pop that empties it sets its depth to GBI_MATRIX_STACK_RESET_DEPTH.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020ef0
 * @ghidraAddress BRGlide: 0x100211b0
 */
GbiCommand *HandleGbiPopMatrix(GbiCommand *pCommand);

/**
 * Branch to a display list, or call it when bits 16 to 23 of the first word are clear. A call that
 * fills the last entry of g_apGbiDisplayListReturnStack exits the game.
 *
 * @param pCommand The command.
 * @return The first command of the display list.
 * @ghidraAddress BRD3D: 0x10020d60
 * @ghidraAddress BRGlide: 0x10021020
 */
GbiCommand *HandleGbiDisplayList(GbiCommand *pCommand);

/**
 * Skip a command.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x100243d0
 * @ghidraAddress BRGlide: 0x10021240
 */
GbiCommand *HandleGbiNoOp(GbiCommand *pCommand);

/**
 * Set the render mode.
 *
 * @param pCommand The command.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020f80
 * @ghidraAddress BRGlide: 0x10021250
 */
GbiCommand *HandleGbiSetRenderMode(GbiCommand *pCommand);

/**
 * Set the low word of the other modes. Only the render mode is applied, and the comparison of
 * alpha and every other mode are skipped.
 *
 * @param pCommand The command, the shift of the mode in bits 8 to 15 of its first word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x10020f50
 * @ghidraAddress BRGlide: 0x10021210
 */
GbiCommand *HandleGbiSetOtherModeL(GbiCommand *pCommand);

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * Set the colour of the fog of the device from the RGBA colour of the command.
 *
 * @param pCommand The command, the colour in its second word.
 * @return The next command.
 * @ghidraAddress BRD3D: 0x1001cc70
 */
GbiCommand *HandleGbiSetFogColor(GbiCommand *pCommand);
#endif
