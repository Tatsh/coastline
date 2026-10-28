#include "win32/boot/strings.h"

#include <stdlib.h>
#include <string.h>

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/stringresources.h"
#include "sdl/win32types.h"
#else
#include <windows.h>
#endif

#include "win32/fileio.h"

// The language resource library, beside the game.
#define STRING_LIBRARY "BRString.dll"

char *g_apszStringResourceTable[STRING_RESOURCE_COUNT + 1];
char *g_pStringTableBuffer;
size_t g_dwStringTableUsedBytes;
size_t g_dwStringTableBufferSize;

#if TGR_PLATFORM == TGR_PLATFORM_SDL
void LoadStringTableFromDll(void) {
    GameFile *pFile;
    unsigned char *pImage;
    int nLength;

    memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(char *));
    if (g_pStringTableBuffer != NULL) {
        return;
    }
    // The resources are read from the file of the library, which the SDL game does not load.
    pFile = CHK_FReadOpen(STRING_LIBRARY);
    g_dwStringTableBufferSize = GetFileLength(pFile);
    pImage = (unsigned char *)CHK_AllocateMemory(g_dwStringTableBufferSize, "BRString.dll");
    CHK_FRead(pImage, 1, g_dwStringTableBufferSize, pFile);
    CHK_FClose(pFile);
    g_pStringTableBuffer = (char *)malloc(g_dwStringTableBufferSize);
    if (g_pStringTableBuffer != NULL) {
        for (unsigned nId = 1; nId <= STRING_RESOURCE_COUNT; nId++) {
            nLength =
                LoadLibraryString(pImage,
                                  g_dwStringTableBufferSize,
                                  nId,
                                  g_pStringTableBuffer + g_dwStringTableUsedBytes,
                                  (int)(g_dwStringTableBufferSize - g_dwStringTableUsedBytes));
            if (nLength != 0) {
                g_apszStringResourceTable[nId] = g_pStringTableBuffer + g_dwStringTableUsedBytes;
                g_dwStringTableUsedBytes += nLength + 1;
            }
        }
    }
    free(pImage);
}
#else
void LoadStringTableFromDll(void) {
    GameFile *pFile;
    HMODULE hLibrary;
    int nLength;

    memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(char *));
    if (g_pStringTableBuffer != NULL) {
        return;
    }
    // The strings take less room than the library that includes them.
    pFile = CHK_FReadOpen(STRING_LIBRARY);
    g_dwStringTableBufferSize = GetFileLength(pFile);
    CHK_FClose(pFile);
    hLibrary = LoadLibraryA(STRING_LIBRARY);
    if (hLibrary == NULL) {
        return;
    }
    g_pStringTableBuffer = malloc(g_dwStringTableBufferSize);
    if (g_pStringTableBuffer != NULL) {
        for (unsigned nId = 1; nId <= STRING_RESOURCE_COUNT; nId++) {
            nLength = LoadStringA(hLibrary,
                                  nId,
                                  g_pStringTableBuffer + g_dwStringTableUsedBytes,
                                  g_dwStringTableBufferSize - g_dwStringTableUsedBytes);
            if (nLength != 0) {
                g_apszStringResourceTable[nId] = g_pStringTableBuffer + g_dwStringTableUsedBytes;
                g_dwStringTableUsedBytes += nLength + 1;
            }
        }
    }
    FreeLibrary(hLibrary);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

char *GetStringResource(unsigned nResourceId) {
    if (nResourceId < 1 || nResourceId > STRING_RESOURCE_COUNT) {
        return NULL;
    }
    return g_apszStringResourceTable[nResourceId];
}

void FreeStringTable(void) {
    if (g_pStringTableBuffer == NULL) {
        return;
    }
    // The entries of the table still point into the freed buffer.
    free(g_pStringTableBuffer);
    g_pStringTableBuffer = NULL;
    g_dwStringTableUsedBytes = 0;
    g_dwStringTableBufferSize = 0;
}
