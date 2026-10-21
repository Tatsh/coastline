#include "track.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "racemenu.h"
#include "system.h"
#include "trackselect.h"
#include "util.h"

/* The grid of the track: 64 cells of 32 units each way. */
#define GRID_SIZE 2048.0f
#define GRID_CELL_SCALE 0.03125f
#define GRID_CELLS 64
#define GRID_ROW_SHIFT 6

/* The room for the messages of the loading. */
#define LOAD_MESSAGE_SIZE 0x50

/* Pack the triangles or models of the cell at a column and row of a grid: the count in the upper
 * half and the start in the lower. */
static u32 PackGridCell(u16 *starts, u8 column, u8 row) {
    u16 cell = column + (row << GRID_ROW_SHIFT);
    u16 start = starts[cell];

    return ((u32)(starts[(u16)(cell + 1)] - start) << 16) | start;
}

/* Whether a point is on the grid. */
static s32 IsOnGrid(f32 x, f32 y) {
    return !(x < 0.0f || GRID_SIZE <= x || y < 0.0f || GRID_SIZE <= y);
}

/* Whether a column and row are on the grid. */
static s32 IsGridCell(s32 column, s32 row) {
    return column >= 0 && column < GRID_CELLS && row >= 0 && row < GRID_CELLS;
}

u32 LookupTrackGridCell(f32 x, f32 y) {
    if (!IsOnGrid(x, y)) {
        return 0;
    }
    return PackGridCell(g_LoadedTrack.gridCellStarts,
                        (u8)(u32)(x * GRID_CELL_SCALE),
                        (u8)(u32)(y * GRID_CELL_SCALE));
}

u32 LookupInstanceCellAtPoint(f32 x, f32 y) {
    if (!IsOnGrid(x, y)) {
        return 0;
    }
    return PackGridCell(g_LoadedTrack.gridInstanceStarts,
                        (u8)(u32)(x * GRID_CELL_SCALE),
                        (u8)(u32)(y * GRID_CELL_SCALE));
}

u32 FindTrackGridCellByIndex(s32 column, s32 row) {
    if (!IsGridCell(column, row)) {
        return 0;
    }
    return PackGridCell(g_LoadedTrack.gridCellStarts, column, row);
}

u32 LookupInstanceGridCell(s32 column, s32 row) {
    if (!IsGridCell(column, row)) {
        return 0;
    }
    return PackGridCell(g_LoadedTrack.gridInstanceStarts, column, row);
}

u16 GetTrackGridCellCount(TrackGridCursor *cursor) {
    return cursor->count;
}

u16 GetInstanceGridCellCount(TrackGridCursor *cursor) {
    return cursor->count;
}

/* Advance a cursor of a cell, and return where it was. */
static u16 AdvanceGridCursor(TrackGridCursor *cursor) {
    u16 start = cursor->start;
    u32 packed = (start + 1) | ((u32)(cursor->count - 1) << 16);

    cursor->count = packed >> 16;
    cursor->start = packed;
    return start;
}

u16 NextGridCellTriangle(TrackGridCursor *cursor) {
    if (cursor->count == 0) {
        return 0;
    }
    return g_LoadedTrack.gridTriangles[AdvanceGridCursor(cursor)];
}

u16 NextGridCellInstance(TrackGridCursor *cursor) {
    if (cursor->count == 0) {
        return 0;
    }
    return g_LoadedTrack.gridInstances[AdvanceGridCursor(cursor)];
}

s32 IsTrackIndexValid(s32 track) {
    return track < g_dwNumTracks && g_aTrackDescriptors[track].trackRomEnd != 0;
}

void LoadTrackGeometryFromRom(s32 track) {
    TrackMenuItem *descriptor = &g_aTrackDescriptors[track];
    /* The messages stay where the debug display points until it shows them. */
    char tooBig[LOAD_MESSAGE_SIZE];
    char tooManyInstances[LOAD_MESSAGE_SIZE];
    char headerMismatch[LOAD_MESSAGE_SIZE];
    Vec3f axis;
    f32 length;
    f32 scale;
    s32 unrotated;
    s32 i;

    g_LoadedTrack.size = DmaReadAssetSize(descriptor->trackRom);
    osInvalDCache(&g_LoadedTrack, sizeof(g_LoadedTrack.size));
    if (g_LoadedTrack.size > TRACK_MAX_SIZE) {
        sprintf(tooBig, "Track %d too big (%d vs. %d)", track, g_LoadedTrack.size, TRACK_MAX_SIZE);
        SetDebugMessage(tooBig);
    } else {
        osSyncPrintf("Loading track %d (%d / %d)\n", track, g_LoadedTrack.size, TRACK_MAX_SIZE);
    }
    DecompressFromRom(&g_LoadedTrack, descriptor->trackRom, NULL);
    osSyncPrintf("hmm = %04x %04x\n", g_awTrackDebugWords[0], g_awTrackDebugWords[4]);
    osInvalDCache(&g_LoadedTrack, g_LoadedTrack.size);
    DmaRead(g_abTrackSection1Header, descriptor->sectionARom, TRACK_SECTION_HEADER_SIZE);
    DecompressFromRom(
        g_abTrackSection1Data, descriptor->sectionARom + TRACK_SECTION_HEADER_SIZE, NULL);
    DmaRead(g_abTrackSection2Header, descriptor->sectionBRom, TRACK_SECTION_HEADER_SIZE);
    DecompressFromRom(
        g_abTrackSection2Data, descriptor->sectionBRom + TRACK_SECTION_HEADER_SIZE, NULL);
    g_dwCurrentTrackRomOffset = descriptor->trackRom;
    g_dwTrackTextureRomOffset = DmaReadWord(descriptor->trackRom) + descriptor->trackRom;
    g_dwStreamDmaPrevSegment = -1;
    g_dwStreamedSnowMode = -1;

    /* Mark the placed models whose matrix holds only a position and a scale, and store the inverse
     * of the scale of each. */
    unrotated = 0;
    for (i = 0; i < g_LoadedTrack.instanceCount; i++) {
        axis.x = 1.0f;
        axis.y = 0.0f;
        axis.z = 0.0f;
        Mtx3x3TransformVec(&axis, &axis, g_LoadedTrack.instances[i].matrix);
        length = Vec3fLength(&axis);
        if (length == 0.0f) {
            continue;
        }
        scale = 1.0f / length;
        if (g_LoadedTrack.instances[i].matrix[0][0] * scale == 1.0f &&
            g_LoadedTrack.instances[i].matrix[1][1] * scale == 1.0f &&
            g_LoadedTrack.instances[i].matrix[2][2] * scale == 1.0f) {
            g_LoadedTrack.instances[i].flags |= TRACK_INSTANCE_UNROTATED;
            unrotated++;
        }
        g_LoadedTrack.instances[i].invScale = scale;
    }
    /* The format asks for two numbers and is given one. */
    osSyncPrintf("Scalars: %d/%d\n", unrotated);
    if (g_LoadedTrack.instanceCount > TRACK_MAX_INSTANCES) {
        sprintf(tooManyInstances,
                "ERROR: instances (%d) > MAX_INSTANCES (%d)",
                g_LoadedTrack.instanceCount,
                TRACK_MAX_INSTANCES);
        SetDebugMessage(tooManyInstances);
    }
    if (g_LoadedTrack.headerSize != TRACK_HEADER_SIZE) {
        sprintf(headerMismatch,
                "ERROR: Track header size mismatch! (%d!=%d)",
                g_LoadedTrack.headerSize,
                TRACK_HEADER_SIZE);
        SetDebugMessage(headerMismatch);
    }
    PrepareTrackSections();
}
