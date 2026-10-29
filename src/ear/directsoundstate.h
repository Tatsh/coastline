#ifndef COASTLINE_EAR_DIRECTSOUNDSTATE_H
#define COASTLINE_EAR_DIRECTSOUNDSTATE_H

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>

struct IDirectSoundBuffer;
struct IDirectSound;
struct _DSBUFFERDESC;
#else
#include "common/types.h"
#include "ear/wavechunks.h"

class EarSdlVoice;
#endif

class [[gnu::packed]] EarDirectSoundPlaybackState {
public:
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /** The buffer of the playback. */
    using Buffer = IDirectSoundBuffer;
#else
    /** The buffer of the playback. */
    using Buffer = EarSdlVoice;
#endif

    /** @ghidraAddress EAR: 0x10012e50 */
    void Initialize();

    Buffer *GetBuffer() const {
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    HRESULT CreateBuffer(IDirectSound *device, _DSBUFFERDESC *description);
#else
    /**
     * Create the buffer of the playback.
     *
     * @param format The format of the samples.
     * @param bytes The bytes of the buffer.
     * @return Whether the buffer was created.
     */
    bool CreateBuffer(const EarWaveFormat &format, DWORD bytes);
#endif
    void UpdateCursors();

private:
    Buffer *buffer;
    DWORD writeCursor;
    DWORD playCursor;
};

#endif /* COASTLINE_EAR_DIRECTSOUNDSTATE_H */
