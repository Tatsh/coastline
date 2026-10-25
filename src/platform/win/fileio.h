#pragma once

/**
 * @file
 * The checked file and memory routines of the PC port. A failure is written to the debugger and
 * ends the process.
 */

#include <cstddef>
#include <cstdio>

/** A file opened by FileOpenForReading(). */
struct GameFile {
    FILE *pFile;   /*!< The stream of the C library. */
    char *pszPath; /*!< A copy of the path, for the messages. */
};

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
void *AllocateMemoryChecked(size_t nSize, const char *szCaller);

/**
 * Whether a file can be opened for reading.
 *
 * @param pszPath The path of the file.
 * @return Whether the file opened.
 * @ghidraAddress BRD3D: 0x10003320
 */
bool FileExists(const char *pszPath);

/**
 * Open a file for reading in binary mode. A file that does not open is reported in
 * `c:\RallyError.txt` and to the debugger, and ends the process.
 *
 * @param pszPath The path of the file.
 * @return The open file.
 * @ghidraAddress BRD3D: 0x10002fe0
 */
GameFile *FileOpenForReading(const char *pszPath);

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
void FileClose(GameFile *pFile);
