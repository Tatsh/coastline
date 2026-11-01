#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#include "common/fileio.h"

// The size of the buffer of a message to the debugger.
#define MESSAGE_SIZE 1024

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
