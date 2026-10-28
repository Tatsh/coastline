#include "win32/fileio.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

// The size of the buffer of a message to the debugger.
#define MESSAGE_SIZE 1024

int g_bVerboseFileTrace;

long GetFileLength(GameFile *pFile) {
    long nPosition = ftell(pFile->pFile);
    long nLength;

    fseek(pFile->pFile, 0, SEEK_END);
    nLength = ftell(pFile->pFile);
    fseek(pFile->pFile, nPosition, SEEK_SET);
    return nLength;
}

GameFile *CHK_FReadOpen(const char *pszPath) {
    char szMessage[MESSAGE_SIZE];
    GameFile *pFile;
    FILE *pErrorFile;

    pFile = CHK_AllocateMemory(sizeof(GameFile), "CHK_FReadOpen():pfil");
    pFile->pszPath = CHK_AllocateMemory(strlen(pszPath) + 1, "CHK_FReadOpen():szName");
    strcpy(pFile->pszPath, pszPath);
    if (g_bVerboseFileTrace) {
        sprintf(szMessage, "CHK_FReadOpen(%s)\n", pFile->pszPath);
        OutputDebugStringA(szMessage);
    }
    pFile->pFile = fopen(pFile->pszPath, "rb");
    if (pFile->pFile == nullptr) {
        pErrorFile = fopen("c:\\RallyError.txt", "w");
        sprintf(szMessage, "CHK_FReadOpen(): error opening file %s.\n", pFile->pszPath);
#ifdef ENABLE_PATCHES
        fprintf(pErrorFile, "%s", szMessage);
#else
        // The original passes the message to fprintf() as the format.
        fprintf(pErrorFile, szMessage);
#endif /* ENABLE_PATCHES */
        OutputDebugStringA(szMessage);
        fclose(pErrorFile);
        exit(1);
    }
    return pFile;
}

GameFile *CHK_FWriteOpen(const char *path, const char *mode) {
    char message[MESSAGE_SIZE];
    GameFile *file = CHK_AllocateMemory(sizeof(*file), "CHK_FWriteOpen():pfil");
    file->pszPath = CHK_AllocateMemory(strlen(path) + 1, "CHK_FReadOpen():szName");
    strcpy(file->pszPath, path);
    if (g_bVerboseFileTrace) {
        sprintf(message, "CHK_FWriteOpen(%s)\n", file->pszPath);
        OutputDebugStringA(message);
    }
    file->pFile = fopen(file->pszPath, mode);
    if (file->pFile == nullptr) {
        sprintf(message, "CHK_FWriteOpen(): error creating file %s.\n", file->pszPath);
        OutputDebugStringA(message);
        exit(1);
    }
    return file;
}

void CHK_FPutS(const char *text, GameFile *file) {
    char message[MESSAGE_SIZE];
    if (fputs(text, file->pFile) == EOF) {
        sprintf(message, "CHK_FPutS(): error writing file %s.\n", file->pszPath);
        OutputDebugStringA(message);
        exit(1);
    }
}

bool FCHK_FRead(void *pBuffer, size_t nSize, size_t nCount, GameFile *pFile) {
    char szMessage[MESSAGE_SIZE];
    size_t nRead;

    if (nSize * nCount == 0) {
        return true;
    }
    nRead = fread(pBuffer, nSize, nCount, pFile->pFile);
    if (nRead == 0) {
        return false;
    }
    if (nRead != nCount) {
        sprintf(szMessage,
                "FCHK_FRead(): trying to read %d bytes, but got only %d bytes.\n",
                (int)(nSize * nCount),
                (int)(nRead * nSize));
        OutputDebugStringA(szMessage);
        exit(1);
    }
    return true;
}

void *CHK_FRead(void *pBuffer, size_t nSize, size_t nCount, GameFile *pFile) {
    char szMessage[MESSAGE_SIZE];

    if (!FCHK_FRead(pBuffer, nSize, nCount, pFile)) {
        sprintf(szMessage,
                "CHK_FRead(): trying to read %u bytes, but got EOF.\n",
                (unsigned)(nCount * nSize));
        OutputDebugStringA(szMessage);
        exit(1);
    }
    return pBuffer;
}

char *FileReadLine(char *pszLine, int nSize, GameFile *pFile) {
    int c;

    for (int nCount = 0; nCount < nSize; nCount++) {
        c = fgetc(pFile->pFile);
        if (c == EOF) {
            if (nCount == 0) {
                return nullptr;
            }
            *pszLine = '\0';
            return pszLine + 1;
        }
        if (c == '\r') {
            *pszLine++ = '\n';
            *pszLine++ = '\0';
            // A line feed after the carriage return belongs to the same line.
            c = fgetc(pFile->pFile);
            if (c != EOF && c != '\n') {
                ungetc(c, pFile->pFile);
            }
            return pszLine;
        }
        if (c == '\n') {
            *pszLine++ = '\n';
            *pszLine = '\0';
            return pszLine + 1;
        }
        *pszLine++ = (char)c;
    }
    return pszLine;
}

void CHK_FClose(GameFile *pFile) {
    char szMessage[MESSAGE_SIZE];

    if (g_bVerboseFileTrace) {
        sprintf(szMessage, "CHK_FClose(%s)\n", pFile->pszPath);
        OutputDebugStringA(szMessage);
    }
    if (fclose(pFile->pFile) == EOF) {
        sprintf(szMessage, "CHK_FClose(): error closing file %s.\n", pFile->pszPath);
        OutputDebugStringA(szMessage);
        exit(1);
    }
    free(pFile->pszPath);
    free(pFile);
}

int CHK_FileExists(const char *pszPath) {
    char szMessage[MESSAGE_SIZE];
    FILE *pFile;

    if (g_bVerboseFileTrace) {
        sprintf(szMessage, "CHK_FileExists(%s)\n", pszPath);
        OutputDebugStringA(szMessage);
    }
    pFile = fopen(pszPath, "rb");
    if (pFile == nullptr) {
        return false;
    }
    fclose(pFile);
    return true;
}

void *CHK_AllocateMemory(size_t nSize, const char *szCaller) {
    char szMessage[MESSAGE_SIZE];
    void *pMemory;

    if (nSize == 0) {
        return nullptr;
    }
    pMemory = malloc(nSize);
    if (pMemory == nullptr) {
        sprintf(szMessage, "CHK_AllocateMemory(): Out of memory: couldn't allocate %s\n", szCaller);
        OutputDebugStringA(szMessage);
        exit(1);
    }
    return pMemory;
}

void *CHK_ReAllocateMemory(void *pMemory, size_t nSize, const char *szCaller) {
    char szMessage[MESSAGE_SIZE];
    void *pResizedMemory = realloc(pMemory, nSize);

    // Zero-size requests still call realloc().
    if (nSize == 0) {
        return nullptr;
    }
    if (pResizedMemory == nullptr) {
        sprintf(
            szMessage, "CHK_ReAllocateMemory(): Out of memory: couldn't reallocate %s\n", szCaller);
        OutputDebugStringA(szMessage);
        exit(1);
    }
    return pResizedMemory;
}
