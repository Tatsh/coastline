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

/**
 * Device buffer reference and cached byte cursors for Windows DirectSound or the SDL mixer.
 * Buffer release is explicit; resetting or replacing the reference does not release a buffer.
 */
class [[gnu::packed]] EarDirectSoundPlaybackState {
public:
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /** Windows device buffer type. */
    using Buffer = IDirectSoundBuffer;
#else
    /** SDL mixer voice type. */
    using Buffer = EarSdlVoice;
#endif

    /**
     * Reset the buffer pointer and both cached byte cursors without releasing prior resources.
     * @ghidraAddress EAR: 0x10012e50
     */
    void Initialize();

    /** @return The borrowed device buffer, or null; releasing the buffer invalidates pointers. */
    Buffer *GetBuffer() const {
        return buffer;
    }
    /** Clear the buffer reference without stopping or releasing the buffer. */
    void ClearBuffer() {
        buffer = nullptr;
    }
    /** @return The cached playback position in buffer bytes, without querying the device. */
    DWORD GetPlayCursor() const {
        return playCursor;
    }
    /**
     * Replace the cached cursor without seeking the device.
     * @param cursor The byte offset to store without validation.
     */
    void SetPlayCursor(DWORD cursor) {
        playCursor = cursor;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /**
     * Create a Windows buffer and store the returned reference without releasing a prior buffer.
     * @param device The non-null borrowed DirectSound device.
     * @param description The non-null DirectSound buffer descriptor.
     * @return The HRESULT from CreateSoundBuffer(), with DS_OK indicating success.
     */
    HRESULT CreateBuffer(IDirectSound *device, _DSBUFFERDESC *description);
#else
    /**
     * Allocate an SDL mixer voice and store the reference without releasing a prior buffer.
     *
     * @param format The PCM sample format copied into the voice.
     * @param bytes The requested device-buffer capacity in bytes.
     * @return Whether creation succeeded; failure stores a null reference.
     */
    bool CreateBuffer(const EarWaveFormat &format, DWORD bytes);
#endif
    /**
     * Query both cached byte cursors from a non-null device buffer.
     * SDL uses the playback cursor for both positions. Windows query results are copied back
     * without checking the result code; output arguments initially have the cached values.
     */
    void UpdateCursors();

private:
    Buffer *buffer;
    DWORD writeCursor;
    DWORD playCursor;
};

#endif /* COASTLINE_EAR_DIRECTSOUNDSTATE_H */
