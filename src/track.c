#include "track.h"

#include <PR/os.h>
#include <PR/os_libc.h>

#include "racemenu.h"
#include "system.h"
#include "trackselect.h"
#include "util.h"

/* The room for the messages of the loading. */
#define LOAD_MESSAGE_SIZE 0x50

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
