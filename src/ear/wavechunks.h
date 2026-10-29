#ifndef COASTLINE_EAR_WAVECHUNKS_H
#define COASTLINE_EAR_WAVECHUNKS_H

#include <stdint.h>

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmreg.h>
#include <mmsystem.h>
#else
#include <stdio.h>

#include "common/types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef MMCKINFO EarRiffChunk;                    /*!< A chunk of a RIFF file. */
typedef WAVEFORMATEX EarWaveFormat;               /*!< The format chunk of a wave file. */
typedef IMAADPCMWAVEFORMAT EarImaAdpcmWaveFormat; /*!< The format chunk of IMA ADPCM. */
typedef HMMIO EarWaveFile;                        /*!< An open wave file. */
#else
/** A chunk of a RIFF file, with the members of the MMCKINFO of Windows. */
typedef struct EarRiffChunk {
    DWORD ckid;         /*!< The identifier of the chunk. */
    DWORD cksize;       /*!< The bytes of the data of the chunk. */
    DWORD fccType;      /*!< The form type of a RIFF or LIST chunk. */
    DWORD dwDataOffset; /*!< The offset of the data of the chunk in the file. */
    DWORD dwFlags;      /*!< Unused. */
} EarRiffChunk;

/** The format chunk of a wave file, laid out as the WAVEFORMATEX of Windows. */
typedef struct [[gnu::packed]] EarWaveFormat {
    WORD wFormatTag;       /*!< The format of the samples. */
    WORD nChannels;        /*!< The channels. */
    DWORD nSamplesPerSec;  /*!< The frames a second. */
    DWORD nAvgBytesPerSec; /*!< The bytes a second. */
    WORD nBlockAlign;      /*!< The bytes of a frame, or of a block of a compressed format. */
    WORD wBitsPerSample;   /*!< The bits of a sample. */
    WORD cbSize;           /*!< The bytes of the format after this member. */
} EarWaveFormat;

/** The format chunk of a wave file of IMA ADPCM, laid out as the IMAADPCMWAVEFORMAT of Windows. */
typedef struct [[gnu::packed]] EarImaAdpcmWaveFormat {
    EarWaveFormat wfx;     /*!< The common members of the format. */
    WORD wSamplesPerBlock; /*!< The frames of a block. */
} EarImaAdpcmWaveFormat;

/** A wave file open for reading, from a path or from memory. */
typedef struct EarRiffFile EarRiffFile;
typedef EarRiffFile *EarWaveFile; /*!< An open wave file. */

/** The flag of EarRiffDescend() that searches the parent for the identifier of the chunk. */
#define EAR_RIFF_FIND_CHUNK 0x10

/**
 * Open a wave file to read, as mmioOpenA() opens a path.
 *
 * @param path The path.
 * @return The file, or a null pointer when it cannot be opened.
 */
EarRiffFile *EarRiffOpenPath(const char *path);

/**
 * Open a wave file in memory to read, as mmioOpenA() opens a buffer of FOURCC_MEM.
 *
 * @param data The file.
 * @param size The bytes that can be read.
 * @return The file.
 */
EarRiffFile *EarRiffOpenMemory(const void *data, LONG size);

/**
 * Close a file. A null pointer is ignored.
 *
 * @param file The file.
 */
void EarRiffClose(EarRiffFile *file);

/**
 * Read bytes, as mmioRead() does.
 *
 * @param file The file.
 * @param buffer The bytes read.
 * @param bytes The bytes to read.
 * @return The bytes read, 0 at the end of the file, or -1 on an error.
 */
LONG EarRiffRead(EarRiffFile *file, void *buffer, LONG bytes);

/**
 * Move the position, as mmioSeek() does.
 *
 * @param file The file.
 * @param offset The offset.
 * @param origin SEEK_SET, SEEK_CUR, or SEEK_END.
 * @return The new position, or -1 on an error.
 */
LONG EarRiffSeek(EarRiffFile *file, LONG offset, int origin);

/**
 * Read the header of the chunk at the position, or with EAR_RIFF_FIND_CHUNK find the chunk of the
 * identifier of @p chunk within @p parent, as mmioDescend() does.
 *
 * @param file The file.
 * @param chunk The chunk read.
 * @param parent The chunk searched, or a null pointer for the whole file.
 * @param flags 0 or EAR_RIFF_FIND_CHUNK.
 * @return 0, or nonzero when the chunk is not found.
 */
UINT EarRiffDescend(EarRiffFile *file, EarRiffChunk *chunk, const EarRiffChunk *parent, UINT flags);

/**
 * Move past the end of a chunk and its pad byte, as mmioAscend() does.
 *
 * @param file The file.
 * @param chunk The chunk.
 * @return 0, or nonzero on an error.
 */
UINT EarRiffAscend(EarRiffFile *file, const EarRiffChunk *chunk);
#endif

/** The identifier of a RIFF file. */
#define EAR_RIFF_ID 0x46464952
/** The form type of a wave file. */
#define EAR_WAVE_ID 0x45564157
/** The identifier of the format chunk. */
#define EAR_FORMAT_CHUNK_ID 0x20746d66
/** The identifier of the data chunk. */
#define EAR_DATA_CHUNK_ID 0x61746164
/** The format of 8-bit and 16-bit PCM samples. */
#define EAR_WAVE_FORMAT_PCM 1
/** The format of IMA ADPCM samples. */
#define EAR_WAVE_FORMAT_IMA_ADPCM 0x11

/** @ghidraAddress EAR: 0x1003b880 */
extern EarRiffChunk g_RiffWaveChunk;
/** @ghidraAddress EAR: 0x1003b898 */
extern EarRiffChunk g_WaveDataChunk;
/** @ghidraAddress EAR: 0x1003b8b0 */
extern EarRiffChunk g_WaveFormatChunk;
/** @ghidraAddress EAR: 0x10037268 */
extern uint16_t g_earMaximumStreamingBufferBytes;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_WAVECHUNKS_H */
