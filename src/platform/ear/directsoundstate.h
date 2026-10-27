#ifndef COASTLINE_EAR_DIRECTSOUNDSTATE_H
#define COASTLINE_EAR_DIRECTSOUNDSTATE_H

#include <windows.h>

struct IDirectSoundBuffer;
class EarPlayback;

class [[gnu::packed]] EarDirectSoundPlaybackState {
    friend class EarPlayback;

    /** @ghidraAddress EAR: 0x10012e50 */
    void Initialize();

    IDirectSoundBuffer *buffer;
    DWORD writeCursor;
    DWORD playCursor;
};

#endif
