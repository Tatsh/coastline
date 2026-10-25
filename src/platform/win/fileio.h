#pragma once

/**
 * @file
 * The checked file and memory routines of the PC port. A failure is written to the debugger and
 * ends the process.
 */

#include <stddef.h>
#include <stdio.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** A file opened by CHK_FReadOpen(). */
typedef struct GameFile {
    FILE *pFile;   /*!< The stream of the C library. */
    char *pszPath; /*!< A copy of the path, for the messages. */
} GameFile;

/**
 * Whether the routines write each file they open or close to the debugger.
 *
 * @ghidraAddress BRD3D: 0x10220ce0
 */
extern bool g_bVerboseFileTrace;

/**
 * Allocate memory, ending the process when none is left.
 *
 * @param nSize The size in bytes.
 * @param szCaller What the memory is for, written to the debugger when the allocation fails.
 * @return The memory, or `NULL` for a size of 0.
 * @ghidraAddress BRD3D: 0x10003390
 */
void *CHK_AllocateMemory(size_t nSize, const char *szCaller);

/**
 * Whether a file can be opened for reading.
 *
 * @param pszPath The path of the file.
 * @return Whether the file opened.
 * @ghidraAddress BRD3D: 0x10003320
 */
bool CHK_FileExists(const char *pszPath);

/**
 * Open a file for reading in binary mode. A file that does not open is reported in
 * `c:\RallyError.txt` and to the debugger, and ends the process.
 *
 * @param pszPath The path of the file.
 * @return The open file.
 * @ghidraAddress BRD3D: 0x10002fe0
 */
GameFile *CHK_FReadOpen(const char *pszPath);

/**
 * Read items of a file. A read that ends part way through ends the process.
 *
 * @param pBuffer The buffer that receives the items.
 * @param nSize The size of an item.
 * @param nCount The number of items.
 * @param pFile The open file.
 * @return false when nothing was read at the end of the file, and true otherwise.
 * @ghidraAddress BRD3D: 0x100030e0
 * @ghidraAddress BRGlide: 0x10003430
 */
bool FCHK_FRead(void *pBuffer, size_t nSize, size_t nCount, GameFile *pFile);

/**
 * Read items of a file. A read at the end of the file, or one that ends part way through, ends the
 * process.
 *
 * @param pBuffer The buffer that receives the items.
 * @param nSize The size of an item.
 * @param nCount The number of items.
 * @param pFile The open file.
 * @return The buffer.
 * @ghidraAddress BRD3D: 0x10003170
 * @ghidraAddress BRGlide: 0x100034c0
 */
void *CHK_FRead(void *pBuffer, size_t nSize, size_t nCount, GameFile *pFile);

/**
 * The length of a file, leaving its position unchanged.
 *
 * @param pFile The open file.
 * @return The length in bytes.
 * @ghidraAddress BRD3D: 0x10002f90
 */
long GetFileLength(GameFile *pFile);

/**
 * Read a line of a file. A carriage return, a line feed, or both end the line, and the line is
 * stored with a line feed. A line that fills the buffer is stored without a terminator.
 *
 * @param pszLine The buffer that receives the line.
 * @param nSize The size of the buffer.
 * @param pFile The open file.
 * @return The byte after the terminator of the line, the end of a full buffer, or `NULL` at the
 * end of the file.
 * @ghidraAddress BRD3D: 0x100031e0
 */
char *FileReadLine(char *pszLine, size_t nSize, GameFile *pFile);

/**
 * Close a file and free it. A file that does not close ends the process.
 *
 * @param pFile The open file.
 * @ghidraAddress BRD3D: 0x10003290
 */
void CHK_FClose(GameFile *pFile);

#ifdef __cplusplus
}
#endif
