#include "platform/win/boot/strings.h"

#include <stdlib.h>
#include <string.h>

#include "platform/win/fileio.h"

// The language resource library, beside the game.
#define STRING_LIBRARY "BRString.dll"

LPCSTR g_apszStringResourceTable[STRING_RESOURCE_COUNT + 1];
char *g_pStringTableBuffer;
DWORD g_dwStringTableUsedBytes;
DWORD g_dwStringTableBufferSize;

void LoadStringTableFromDll(void) {
    GameFile *pFile;
    HMODULE hLibrary;
    UINT nId;
    int nLength;

    memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(LPCSTR));
    if (g_pStringTableBuffer != NULL) {
        return;
    }
    // The strings take less room than the library that includes them.
    pFile = FileOpenForReading(STRING_LIBRARY);
    g_dwStringTableBufferSize = GetFileLength(pFile);
    FileClose(pFile);
    hLibrary = LoadLibraryA(STRING_LIBRARY);
    if (hLibrary == NULL) {
        return;
    }
    g_pStringTableBuffer = static_cast<char *>(malloc(g_dwStringTableBufferSize));
    if (g_pStringTableBuffer != NULL) {
        for (nId = 1; nId <= STRING_RESOURCE_COUNT; nId++) {
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

LPCSTR GetStringResource(UINT nResourceId) {
    if (nResourceId < 1 || nResourceId > STRING_RESOURCE_COUNT) {
        return NULL;
    }
    return g_apszStringResourceTable[nResourceId];
}
