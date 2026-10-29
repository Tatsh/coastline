#include "common/trackselect.h"

#if !TGR_PC
#include "racemenu.h"
#endif

s32 IsTrackIndexValid(s32 track) {
#if TGR_PC
    return 1;
#else
    return track < g_dwNumTracks && g_aTrackDescriptors[track].trackRomEnd != 0;
#endif
}
