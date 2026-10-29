#ifndef COASTLINE_SDL_MMSYSTEMCOMPAT_H
#define COASTLINE_SDL_MMSYSTEMCOMPAT_H

/**
 * @file
 * The calls of the multimedia API, the global memory of the Windows API, and the audio
 * compression manager that the sound of the PC release makes, on SDL. The multimedia I/O reads a
 * whole file into memory. The media control interface has no CD audio device.
 */

#include <stdio.h>

#include "sdl/win32types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD FOURCC; /*!< A code of four characters. */

/** Make a code of four characters. */
#define mmioFOURCC(a, b, c, d)                                                                     \
    ((FOURCC)(BYTE)(a) | ((FOURCC)(BYTE)(b) << 8) | ((FOURCC)(BYTE)(c) << 16) |                    \
     ((FOURCC)(BYTE)(d) << 24))
#define FOURCC_RIFF mmioFOURCC('R', 'I', 'F', 'F') /*!< The code of a RIFF chunk. */
#define FOURCC_LIST mmioFOURCC('L', 'I', 'S', 'T') /*!< The code of a list chunk. */

#define MMIO_READ 0x00000000      /*!< Open a file to read. */
#define MMIO_ALLOCBUF 0x00010000  /*!< Buffer the file. */
#define MMIO_FINDCHUNK 0x0010     /*!< Find a chunk by its code. */
#define MMIOERR_CHUNKNOTFOUND 265 /*!< The chunk is not in the file. */
#define MMIOERR_CANNOTSEEK 263    /*!< The position is outside the file. */
#define WAVE_FORMAT_PCM 1         /*!< The format tag of PCM. */

/** The state of the buffer of a file of the multimedia I/O. */
typedef struct _MMIOINFO {
    DWORD dwFlags;    /*!< The flags of the open file. */
    char *pchBuffer;  /*!< The buffer. */
    char *pchNext;    /*!< The next byte to read. */
    char *pchEndRead; /*!< The end of the bytes read into the buffer. */
    LONG lDiskOffset; /*!< The offset of the end of the buffer in the file. */
} MMIOINFO;

#pragma pack(push, 1)
/** The format of a wave, without the size of its extra data. */
typedef struct waveformat_tag {
    WORD wFormatTag;       /*!< The format. */
    WORD nChannels;        /*!< The channels. */
    DWORD nSamplesPerSec;  /*!< The frames a second. */
    DWORD nAvgBytesPerSec; /*!< The bytes a second. */
    WORD nBlockAlign;      /*!< The bytes of a frame. */
} WAVEFORMAT;

/** The format of a PCM wave. */
typedef struct pcmwaveformat_tag {
    WAVEFORMAT wf;       /*!< The format. */
    WORD wBitsPerSample; /*!< The bits of a sample. */
} PCMWAVEFORMAT;
#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(PCMWAVEFORMAT) == 16, "PCMWAVEFORMAT has the 16 bytes of a fmt chunk.");
static_assert(sizeof(WAVEFORMATEX) == 18, "WAVEFORMATEX has its 18 bytes on Windows.");
#endif

/**
 * @param path The file.
 * @param info Unused.
 * @param flags MMIO_READ and MMIO_ALLOCBUF.
 * @return The file, or null when it cannot be read.
 */
HMMIO mmioOpenA(LPSTR path, MMIOINFO *info, DWORD flags);

/**
 * Enter a chunk at the position of the file, or find one by its code.
 *
 * @param file The file.
 * @param chunk Receives the chunk, and with MMIO_FINDCHUNK specifies its code.
 * @param parent The chunk that the chunk is in, or null.
 * @param flags 0 or MMIO_FINDCHUNK.
 * @return 0, or MMIOERR_CHUNKNOTFOUND.
 */
UINT mmioDescend(HMMIO file, MMCKINFO *chunk, const MMCKINFO *parent, UINT flags);

/**
 * Move to the end of a chunk.
 *
 * @param file The file.
 * @param chunk The chunk.
 * @param flags 0.
 * @return 0.
 */
UINT mmioAscend(HMMIO file, MMCKINFO *chunk, UINT flags);

/**
 * @param file The file.
 * @param buffer Receives the bytes.
 * @param size The bytes to read.
 * @return The bytes read.
 */
LONG mmioRead(HMMIO file, HPSTR buffer, LONG size);

/**
 * @param file The file.
 * @param offset The offset.
 * @param origin SEEK_SET, SEEK_CUR, or SEEK_END.
 * @return The new position, or -1 outside the file.
 */
LONG mmioSeek(HMMIO file, LONG offset, int origin);

/**
 * @param file The file.
 * @param info Receives the buffer, the whole file.
 * @param flags 0.
 * @return 0.
 */
UINT mmioGetInfo(HMMIO file, MMIOINFO *info, UINT flags);

/**
 * Fill the buffer again. The buffer is the whole file, and the end of the file remains.
 *
 * @param file The file.
 * @param info The buffer.
 * @param flags MMIO_READ.
 * @return 0.
 */
UINT mmioAdvance(HMMIO file, MMIOINFO *info, UINT flags);

/**
 * @param file The file.
 * @param info The buffer, the next byte of which becomes the position of the file.
 * @param flags 0.
 * @return 0.
 */
UINT mmioSetInfo(HMMIO file, const MMIOINFO *info, UINT flags);

/**
 * @param file The file.
 * @param flags 0.
 * @return 0.
 */
UINT mmioClose(HMMIO file, UINT flags);

#define GMEM_FIXED 0x0000    /*!< Memory that does not move. */
#define GMEM_ZEROINIT 0x0040 /*!< Memory that is cleared. */

typedef void *HGLOBAL; /*!< A block of global memory, its address on SDL. */

/**
 * @param flags GMEM_FIXED or GMEM_ZEROINIT.
 * @param size The bytes.
 * @return The memory, cleared, or null.
 */
HGLOBAL GlobalAlloc(UINT flags, size_t size);

/**
 * @param memory The memory, or null.
 * @return Null.
 */
HGLOBAL GlobalFree(HGLOBAL memory);

/**
 * @param memory The memory.
 * @return The memory.
 */
static inline void *GlobalLock(HGLOBAL memory) {
    return memory;
}

/**
 * @param memory The memory.
 * @return TRUE.
 */
static inline BOOL GlobalUnlock(HGLOBAL memory) {
    return TRUE;
}

/**
 * @param memory The memory.
 * @return The memory.
 */
static inline HGLOBAL GlobalHandle(const void *memory) {
    return (HGLOBAL)memory;
}

#define ACM_METRIC_MAX_SIZE_FORMAT 50 /*!< The largest size of a wave format. */

/**
 * @param object Unused.
 * @param metric ACM_METRIC_MAX_SIZE_FORMAT.
 * @param result Receives the size of a wave format with 2 bytes of extra data.
 * @return 0.
 */
static inline UINT acmMetrics(void *object, UINT metric, void *result) {
    *(DWORD *)result = (DWORD)sizeof(WAVEFORMATEX) + 2;
    return 0;
}

#define MCI_OPEN 0x0803                        /*!< Open a device. */
#define MCI_CLOSE 0x0804                       /*!< Close a device. */
#define MCI_PLAY 0x0806                        /*!< Play. */
#define MCI_STOP 0x0808                        /*!< Stop. */
#define MCI_PAUSE 0x0809                       /*!< Pause. */
#define MCI_SET 0x080D                         /*!< Set a parameter. */
#define MCI_STATUS 0x0814                      /*!< Query a status. */
#define MCI_NOTIFY 0x00000001                  /*!< Notify the window. */
#define MCI_WAIT 0x00000002                    /*!< Wait for the command. */
#define MCI_FROM 0x00000004                    /*!< A start is given. */
#define MCI_TO 0x00000008                      /*!< An end is given. */
#define MCI_OPEN_TYPE 0x00002000               /*!< A device type is given. */
#define MCI_SET_TIME_FORMAT 0x00000400         /*!< A time format is given. */
#define MCI_STATUS_ITEM 0x00000100             /*!< An item is given. */
#define MCI_STATUS_NUMBER_OF_TRACKS 0x00000003 /*!< The tracks of the CD. */
#define MCI_STATUS_CURRENT_TRACK 0x00000008    /*!< The track that plays. */
#define MCI_FORMAT_TMSF 10                     /*!< Track, minute, second, and frame. */
#define MCI_NOTIFY_SUCCESSFUL 0x0001           /*!< A command that completed. */
#define MM_MCINOTIFY 0x3B9                     /*!< The message of a notification. */
#define MCIERR_DEVICE_NOT_READY 276            /*!< The error of a device not ready. */

/** The parameters of MCI_OPEN. */
typedef struct tagMCI_OPEN_PARMSA {
    DWORD_PTR dwCallback;    /*!< The window to notify. */
    MCIDEVICEID wDeviceID;   /*!< Receives the device. */
    LPCSTR lpstrDeviceType;  /*!< The type of the device. */
    LPCSTR lpstrElementName; /*!< The element. */
    LPCSTR lpstrAlias;       /*!< The alias. */
} MCI_OPEN_PARMSA;

/** The parameters of MCI_SET. */
typedef struct tagMCI_SET_PARMS {
    DWORD_PTR dwCallback; /*!< The window to notify. */
    DWORD dwTimeFormat;   /*!< The time format. */
    DWORD dwAudio;        /*!< The audio channel. */
} MCI_SET_PARMS;

/** The parameters of MCI_STATUS. */
typedef struct tagMCI_STATUS_PARMS {
    DWORD_PTR dwCallback; /*!< The window to notify. */
    DWORD_PTR dwReturn;   /*!< Receives the status. */
    DWORD dwItem;         /*!< The item. */
    DWORD dwTrack;        /*!< The track. */
} MCI_STATUS_PARMS;

/** The parameters of MCI_PLAY. */
typedef struct tagMCI_PLAY_PARMS {
    DWORD_PTR dwCallback; /*!< The window to notify. */
    DWORD dwFrom;         /*!< The start. */
    DWORD dwTo;           /*!< The end. */
} MCI_PLAY_PARMS;

/**
 * Send a command to the CD audio device of the media control interface. The audio tracks of the
 * CD are the files music/default/trackNN.flac of the game data, from track 2. MCI_PLAY plays the
 * tracks from its first to its last, and with MCI_NOTIFY posts MM_MCINOTIFY to the window when they
 * end.
 *
 * @param device The device of MCI_OPEN.
 * @param message MCI_OPEN, MCI_CLOSE, MCI_SET, MCI_STATUS, MCI_PLAY, MCI_PAUSE, or MCI_STOP.
 * @param flags The flags of the command.
 * @param parameters The parameters of the command.
 * @return 0, or MCIERR_DEVICE_NOT_READY when the game data has no audio track or SDL has no
 * playback device.
 */
MCIERROR mciSendCommandA(MCIDEVICEID device, UINT message, DWORD_PTR flags, DWORD_PTR parameters);

/** Format a string, as sprintf() does. */
#define wsprintfA sprintf

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_MMSYSTEMCOMPAT_H */
