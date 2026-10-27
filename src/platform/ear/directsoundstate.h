#ifndef COASTLINE_EAR_DIRECTSOUNDSTATE_H
#define COASTLINE_EAR_DIRECTSOUNDSTATE_H

#include <windows.h>

struct IDirectSoundBuffer;
struct IDirectSound;
struct _DSBUFFERDESC;

class [[gnu::packed]] EarDirectSoundPlaybackState {
public:
    /** @ghidraAddress EAR: 0x10012e50 */
    void Initialize();

    IDirectSoundBuffer *GetBuffer() const {
        return buffer;
    }
    void ClearBuffer() {
        buffer = nullptr;
    }
    DWORD GetPlayCursor() const {
        return playCursor;
    }
    void SetPlayCursor(DWORD cursor) {
        playCursor = cursor;
    }
    HRESULT CreateBuffer(IDirectSound *device, _DSBUFFERDESC *description);
    void UpdateCursors();

private:
    IDirectSoundBuffer *buffer;
    DWORD writeCursor;
    DWORD playCursor;
};

#endif
