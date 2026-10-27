#include "trackselect.h"

#if TGR_PLATFORM != TGR_PLATFORM_WIN
#include "racemenu.h"
#endif

s32 IsTrackIndexValid(s32 track) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    return 1;
#else
    return track < g_dwNumTracks && g_aTrackDescriptors[track].trackRomEnd != 0;
#endif
}
