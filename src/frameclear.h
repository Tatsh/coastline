#ifndef COASTLINE_FRAMECLEAR_H
#define COASTLINE_FRAMECLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A block of variables cleared at the start of each frame. */
typedef struct {
    u8 *start; /*!< The first byte of the block, or NULL to end the list. */
    u32 size;  /*!< The size of the block, in bytes. */
} PerFrameVariable;

/**
 * The variables of the searches for straight lines, cleared each frame, ending in a NULL start.
 *
 * @ghidraAddress USA: 0x8028b834
 * @ghidraAddress EUR: 0x8028dae4
 * @ghidraAddress JPN: 0x8028d514
 * @ghidraAddress BRD3D: 0x100b3700
 */
extern PerFrameVariable g_aPerFrameVariables[];

/**
 * Clear the variables g_aPerFrameVariables lists.
 *
 * @ghidraAddress USA: 0x80225f30
 * @ghidraAddress EUR: 0x80227270
 * @ghidraAddress JPN: 0x80226a60
 * @ghidraAddress BRD3D: 0x100633e0
 * @ghidraAddress BRGlide: 0x1005c450
 */
void ClearPerFrameVariables(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_FRAMECLEAR_H */
