/* Test of the grid lookups of src/n64/track.c and the check of a track index. */
#include <assert.h>
#include <stdio.h>

#include "track.c"

TrackHeader g_LoadedTrack;
TrackMenuItem g_aTrackDescriptors[2];
s32 g_dwNumTracks = 2;
u8 g_abTrackSection1Header[TRACK_SECTION_HEADER_SIZE],
    g_abTrackSection1Data[TRACK_SECTION_DATA_SIZE];
u8 g_abTrackSection2Header[TRACK_SECTION_HEADER_SIZE],
    g_abTrackSection2Data[TRACK_SECTION_DATA_SIZE];
u32 g_dwCurrentTrackRomOffset, g_dwTrackTextureRomOffset;
s32 g_dwStreamDmaPrevSegment, g_dwStreamedSnowMode;
u16 g_awTrackDebugWords[5];

u32 DmaReadAssetSize(u32 romOffset) {
    return 0;
}
u32 DmaReadWord(u32 romOffset) {
    return 0;
}
void DmaRead(void *dst, u32 romOffset, s32 size) {
}
u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
    return 0;
}
void osInvalDCache(void *vaddr, s32 nbytes) {
}
void osSyncPrintf(const char *format, ...) {
}
void PrepareTrackSections(void) {
}

int main(void) {
    static u16 starts[64 * 64 + 1];
    static u16 triangles[64 * 64 * 2];
    TrackGridCursor cursor;
    u32 cell;
    s32 i;

    /* Each cell holds two triangles, numbered from 1 by their place in the list. */
    for (i = 0; i <= 64 * 64; i++) {
        starts[i] = i * 2;
    }
    for (i = 0; i < 64 * 64 * 2; i++) {
        triangles[i] = i + 1;
    }
    g_LoadedTrack.gridCellStarts = starts;
    g_LoadedTrack.gridTriangles = triangles;
    g_LoadedTrack.gridInstanceStarts = starts;
    g_LoadedTrack.gridInstances = triangles;

    /* The point (40, 70) is in column 1 and row 2, cell 129. */
    cell = LookupTrackGridCell(40.0f, 70.0f);
    assert(cell == ((2u << 16) | 258));
    assert(FindTrackGridCellByIndex(1, 2) == cell && LookupInstanceGridCell(1, 2) == cell);
    assert(LookupInstanceCellAtPoint(40.0f, 70.0f) == cell);
    assert(LookupTrackGridCell(-1.0f, 5.0f) == 0 && LookupTrackGridCell(5.0f, 2048.0f) == 0);
    assert(FindTrackGridCellByIndex(64, 0) == 0 && LookupInstanceGridCell(0, -1) == 0);

    cursor.start = cell;
    cursor.count = cell >> 16;
    assert(GetTrackGridCellCount(&cursor) == 2);
    assert(NextGridCellTriangle(&cursor) == 259 && NextGridCellTriangle(&cursor) == 260);
    assert(NextGridCellTriangle(&cursor) == 0 && GetInstanceGridCellCount(&cursor) == 0);
    cursor.start = cell;
    cursor.count = 1;
    assert(NextGridCellInstance(&cursor) == 259 && NextGridCellInstance(&cursor) == 0);

    /* A track exists when it has geometry and is among the tracks. */
    g_aTrackDescriptors[1].trackRomEnd = 0x1000;
    assert(!IsTrackIndexValid(0) && IsTrackIndexValid(1) && !IsTrackIndexValid(2));

    puts("track ok");
    return 0;
}
