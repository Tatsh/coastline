#ifndef COASTLINE_SDL_DSOUNDCOMPAT_H
#define COASTLINE_SDL_DSOUNDCOMPAT_H

/**
 * @file
 * The DirectSound of the PC release over SDL audio. Every playing secondary buffer is mixed into
 * one stream of the default playback device, at its frequency, volume, and pan. The primary
 * buffer records its format and does not play.
 */

#include <cstdint>
#include <vector>

#include "sdl/win32types.h"

#define DS_OK S_OK                               /*!< The result of a success. */
#define DSERR_BADFORMAT ((HRESULT)0x88780064)    /*!< The result of a format not supported. */
#define DSERR_INVALIDPARAM ((HRESULT)0x80070057) /*!< The result of a parameter not valid. */
#define DSBCAPS_PRIMARYBUFFER 0x00000001         /*!< The primary buffer. */
#define DSBCAPS_STATIC 0x00000002                /*!< A buffer of a sample. */
#define DSBCAPS_LOCHARDWARE 0x00000004           /*!< A buffer the hardware mixes. */
#define DSBCAPS_LOCSOFTWARE 0x00000008           /*!< A buffer the software mixes. */
#define DSBCAPS_CTRLFREQUENCY 0x00000020         /*!< A buffer with a frequency. */
#define DSBCAPS_CTRLPAN 0x00000040               /*!< A buffer with a pan. */
#define DSBCAPS_CTRLVOLUME 0x00000080            /*!< A buffer with a volume. */
#define DSBCAPS_STICKYFOCUS 0x00004000           /*!< A buffer heard without focus. */
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000   /*!< A buffer with an exact play position. */
#define DSBVOLUME_MIN (-10000)                   /*!< The volume of silence. */
#define DSBVOLUME_MAX 0                          /*!< The full volume. */
#define DSBPAN_LEFT (-10000)                     /*!< The pan of the left side. */
#define DSBPAN_CENTER 0                          /*!< The pan of the centre. */
#define DSBPAN_RIGHT 10000                       /*!< The pan of the right side. */
#define DSBPLAY_LOOPING 0x00000001               /*!< Play a buffer in a loop. */
#define DSBSTATUS_PLAYING 0x00000001             /*!< A buffer that plays. */
#define DSBSTATUS_BUFFERLOST 0x00000002          /*!< A buffer that is lost. */
#define DSBSTATUS_LOOPING 0x00000004             /*!< A buffer that plays in a loop. */
#define DSBLOCK_ENTIREBUFFER 0x00000002          /*!< Lock the whole buffer. */
#define DSSCL_NORMAL 1                           /*!< The normal cooperative level. */
#define DSSCL_PRIORITY 2                         /*!< The priority cooperative level. */
#define DSSCL_EXCLUSIVE 3                        /*!< The exclusive cooperative level. */
#define CLSCTX_INPROC_SERVER 0x1                 /*!< A server in the process. */

/** The description of a buffer to create. */
struct DSBUFFERDESC {
    DWORD dwSize;              /*!< The size of the description. */
    DWORD dwFlags;             /*!< The DSBCAPS flags. */
    DWORD dwBufferBytes;       /*!< The bytes of the buffer, 0 for the primary buffer. */
    DWORD dwReserved;          /*!< 0. */
    WAVEFORMATEX *lpwfxFormat; /*!< The format, null for the primary buffer. */
};

/** The capabilities of a buffer. */
struct DSBCAPS {
    DWORD dwSize;               /*!< The size of the capabilities. */
    DWORD dwFlags;              /*!< The DSBCAPS flags. */
    DWORD dwBufferBytes;        /*!< The bytes of the buffer. */
    DWORD dwUnlockTransferRate; /*!< 0. */
    DWORD dwPlayCpuOverhead;    /*!< 0. */
};

/** A buffer of DirectSound. */
struct IDirectSoundBuffer {
    /**
     * @param offset The first byte.
     * @param bytes The bytes to lock.
     * @param audio1 Receives the first part.
     * @param bytes1 Receives the bytes of the first part.
     * @param audio2 Receives the part that wraps to the start, or null.
     * @param bytes2 Receives the bytes of the second part.
     * @param flags DSBLOCK_ENTIREBUFFER or 0.
     * @return DS_OK, or DSERR_INVALIDPARAM.
     */
    HRESULT Lock(DWORD offset,
                 DWORD bytes,
                 void **audio1,
                 DWORD *bytes1,
                 void **audio2,
                 DWORD *bytes2,
                 DWORD flags);

    /** @return DS_OK. The mixer reads the bytes as they are written. */
    HRESULT Unlock(void *audio1, DWORD bytes1, void *audio2, DWORD bytes2) {
        return DS_OK;
    }

    /**
     * @param reserved 0.
     * @param priority 0.
     * @param flags DSBPLAY_LOOPING or 0.
     * @return DS_OK.
     */
    HRESULT Play(DWORD reserved, DWORD priority, DWORD flags);

    /** @return DS_OK, with the buffer stopped at its position. */
    HRESULT Stop();

    /**
     * @param volume The attenuation in hundredths of a decibel, from DSBVOLUME_MIN to 0.
     * @return DS_OK.
     */
    HRESULT SetVolume(LONG volume);

    /**
     * @param pan The attenuation of the other side in hundredths of a decibel, negative for the
     * left.
     * @return DS_OK.
     */
    HRESULT SetPan(LONG pan);

    /**
     * @param frequency The frames a second, or 0 for those of the format.
     * @return DS_OK.
     */
    HRESULT SetFrequency(DWORD frequency);

    /**
     * @param status Receives the DSBSTATUS flags.
     * @return DS_OK.
     */
    HRESULT GetStatus(DWORD *status);

    /**
     * @param position The byte to play next.
     * @return DS_OK.
     */
    HRESULT SetCurrentPosition(DWORD position);

    /**
     * @param play Receives the byte that plays, or null.
     * @param write Receives the first byte that is safe to write, or null.
     * @return DS_OK.
     */
    HRESULT GetCurrentPosition(DWORD *play, DWORD *write);

    /**
     * @param caps Receives the capabilities, a buffer the software mixes.
     * @return DS_OK.
     */
    HRESULT GetCaps(DSBCAPS *caps);

    /**
     * @param format Receives the format, or null.
     * @param size The bytes of the format.
     * @param written Receives the bytes of the format, or null.
     * @return DS_OK.
     */
    HRESULT GetFormat(WAVEFORMATEX *format, DWORD size, DWORD *written);

    /**
     * @param format The format of the primary buffer.
     * @return DS_OK.
     */
    HRESULT SetFormat(const WAVEFORMATEX *format);

    /** @return DS_OK. The buffers of SDL are not lost. */
    HRESULT Restore() {
        return DS_OK;
    }

    /** @return 0, with the buffer destroyed. */
    ULONG Release();

    WAVEFORMATEX format{};           /*!< The format. */
    std::vector<std::uint8_t> bytes; /*!< The samples. */
    double position = 0.0;           /*!< The frame to play next, with its fraction. */
    DWORD frequency = 0;             /*!< The frames a second. */
    LONG volume = DSBVOLUME_MAX;     /*!< The volume. */
    LONG pan = DSBPAN_CENTER;        /*!< The pan. */
    bool primary = false;            /*!< Whether the buffer is the primary buffer. */
    bool playing = false;            /*!< Whether the buffer plays. */
    bool looping = false;            /*!< Whether the buffer plays in a loop. */
};

/** The DirectSound object. */
struct IDirectSound {
    /**
     * Start the audio of SDL.
     *
     * @param device Unused.
     * @return DS_OK, or E_FAIL when SDL has no playback device.
     */
    HRESULT Initialize(const GUID *device);

    /** @return DS_OK. */
    HRESULT SetCooperativeLevel(HWND window, DWORD level) {
        return DS_OK;
    }

    /**
     * @param description The buffer.
     * @param buffer Receives the buffer.
     * @param outer Unused.
     * @return DS_OK, DSERR_BADFORMAT for a format not 8-bit or 16-bit PCM, or DSERR_INVALIDPARAM.
     */
    HRESULT
    CreateSoundBuffer(const DSBUFFERDESC *description, IDirectSoundBuffer **buffer, void *outer);

    /** @return 0, with the audio of SDL stopped. */
    ULONG Release();
};

extern const GUID CLSID_DirectSound; /*!< The class of DirectSound. */
extern const GUID IID_IDirectSound;  /*!< The interface of DirectSound. */

/**
 * Create the DirectSound object, the only class the SDL game creates.
 *
 * @param classId CLSID_DirectSound.
 * @param outer Unused.
 * @param context CLSCTX_INPROC_SERVER.
 * @param interfaceId IID_IDirectSound.
 * @param object Receives the object.
 * @return S_OK.
 */
HRESULT CoCreateInstance(
    const GUID &classId, void *outer, DWORD context, const GUID &interfaceId, void **object);

/** @return S_OK. COM is not used on SDL. */
inline HRESULT CoInitialize(void *reserved) {
    return S_OK;
}

/** COM is not used on SDL. */
inline void CoUninitialize() {
}

#endif /* COASTLINE_SDL_DSOUNDCOMPAT_H */
