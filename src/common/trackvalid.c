#include "common/trackselect.h"

#if !TGR_BOSS_RALLY
#include "common/trackmenuitem.h"
#endif

s32 IsTrackIndexValid(s32 track) {
#if TGR_BOSS_RALLY
    return 1;
#else
    return track < g_dwNumTracks && g_aTrackDescriptors[track].trackRomEnd != 0;
#endif
}
