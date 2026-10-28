#ifndef COASTLINE_PLATFORM_WIN_WAVREAD_H
#define COASTLINE_PLATFORM_WIN_WAVREAD_H

/**
 * @file
 * The reading of wave files through the multimedia I/O of Windows.
 */

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/mmsystemcompat.h"
#else
#include <windows.h>
// The headers below need the types of windows.h.
#include <mmreg.h>
#include <mmsystem.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Global memory was not allocated. */
#define WAVE_ERROR_MEMORY 0xe000
/** The file was not opened. */
#define WAVE_ERROR_CANNOT_OPEN 0xe100
/** The file is not a wave file. */
#define WAVE_ERROR_NOT_WAVE_FILE 0xe101
/** The file was not read. */
#define WAVE_ERROR_CANNOT_READ 0xe102
/** The data of the file ends early. */
#define WAVE_ERROR_CORRUPT_WAVE_FILE 0xe103

/**
 * Open a wave file and read its format into global memory, leaving the file at the end of the
 * chunk of the format.
 *
 * @param pszPath The path of the file.
 * @param phmmio Receives the open file, or null.
 * @param ppFormat Receives the format, or null.
 * @param pckRiff Receives the chunk of the RIFF.
 * @return 0, a WAVE_ERROR_ value, or the error of the multimedia I/O.
 * @ghidraAddress BRD3D: 0x10076ce0
 * @ghidraAddress BRGlide: 0x1006ffc0
 */
int WaveOpenFile(const char *pszPath, HMMIO *phmmio, WAVEFORMATEX **ppFormat, MMCKINFO *pckRiff);

/**
 * Seek to the start of the RIFF of an open wave file and descend into its chunk of data.
 *
 * @param phmmio The open file.
 * @param pckData Receives the chunk of data.
 * @param pckRiff The chunk of the RIFF.
 * @return The result of mmioDescend().
 * @ghidraAddress BRD3D: 0x10076e90
 * @ghidraAddress BRGlide: 0x10070170
 */
int WaveStartDataRead(HMMIO *phmmio, MMCKINFO *pckData, MMCKINFO *pckRiff);

/**
 * Read data from the chunk of data of an open wave file.
 *
 * @param hmmio The open file.
 * @param dwRead The bytes to read, limited to the bytes left in the chunk.
 * @param pDest Receives the data.
 * @param pckData The chunk of data, whose size is reduced by the bytes read.
 * @param pdwActualRead Receives the bytes read, or 0 on an error.
 * @return 0, 1 when the information of the file is not read, #WAVE_ERROR_CORRUPT_WAVE_FILE, or the
 * error of the multimedia I/O.
 * @ghidraAddress BRD3D: 0x10076ed0
 * @ghidraAddress BRGlide: 0x100701b0
 */
int WaveReadFile(HMMIO hmmio, DWORD dwRead, BYTE *pDest, MMCKINFO *pckData, DWORD *pdwActualRead);

/**
 * Read a wave file into global memory.
 *
 * @param pszPath The path of the file.
 * @param pdwSize Receives the size of the data.
 * @param pdwSamples Not written.
 * @param ppFormat Receives the format.
 * @param ppData Receives the data.
 * @return 0, a WAVE_ERROR_ value, or the error of the multimedia I/O.
 * @ghidraAddress BRD3D: 0x10076fa0
 * @ghidraAddress BRGlide: 0x10070280
 */
int WaveLoadFile(
    const char *pszPath, DWORD *pdwSize, DWORD *pdwSamples, WAVEFORMATEX **ppFormat, void **ppData);

#ifdef __cplusplus
}
#endif

#endif // COASTLINE_PLATFORM_WIN_WAVREAD_H
