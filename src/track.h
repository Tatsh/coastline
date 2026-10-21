#ifndef COASTLINE_TRACK_H
#define COASTLINE_TRACK_H

/**
 * @file
 * The track data a race loads below the program.
 */

#include <PR/ultratypes.h>

/** A node of a model tree. */
typedef struct ModelNode {
    struct ModelNode *child;
    struct ModelNode *next;
    u8 unreconstructed8[0x11 - 0x8];
    /** What the node draws. */
    u8 kind;
    u8 unreconstructed12[0x16 - 0x12];
    u16 flags;
} ModelNode;

/** The flag a node can carry that stops a walk from entering it. */
#define MODEL_NODE_SKIP 0x0001

/** The flag a walk sets on the nodes it has visited. */
#define MODEL_NODE_VISITED 0x8000

/** The header of the loaded track. Its fields join this structure as they are reconstructed. */
typedef struct {
    u8 unreconstructed0[0x60];
    /** The placed models, 0x54 bytes each. */
    void *instances;
    s32 instanceCount;
    u8 unreconstructed68[0x70 - 0x68];
    ModelNode *modelRoot;
} TrackHeader;

/**
 * The loaded track. The menus use the same memory for their music. The link script places it.
 *
 * @ghidraAddress USA: 0x80025c00
 */
extern TrackHeader g_LoadedTrack;

/** The placed models drawn this frame, by index. @ghidraAddress USA: 0x80352580 */
extern u16 g_awVisibleInstanceList[];

/** The opaque models at the start of the list. @ghidraAddress USA: 0x8028c744 */
extern s32 g_nOpaqueVisibleInstanceCount;

/** The placed model left undrawn while debugging. @ghidraAddress USA: 0x8028c754 */
extern s32 g_nDebugHiddenInstanceIndex;

#endif
