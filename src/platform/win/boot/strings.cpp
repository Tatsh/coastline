#include "platform/win/boot/strings.h"

#include <cstdlib>
#include <cstring>

#include <windows.h>

#include "platform/win/fileio.h"

// The language resource library, beside the game.
#define STRING_LIBRARY "BRString.dll"

const char *g_apszStringResourceTable[STRING_RESOURCE_COUNT + 1];
char *g_pStringTableBuffer;
size_t g_dwStringTableUsedBytes;
size_t g_dwStringTableBufferSize;

void LoadStringTableFromDll(void) {
    GameFile *pFile;
    HMODULE hLibrary;
    int nLength;

    memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(const char *));
    if (g_pStringTableBuffer != nullptr) {
        return;
    }
    // The strings take less room than the library that includes them.
    pFile = CHK_FReadOpen(STRING_LIBRARY);
    g_dwStringTableBufferSize = GetFileLength(pFile);
    CHK_FClose(pFile);
    hLibrary = LoadLibraryA(STRING_LIBRARY);
    if (hLibrary == nullptr) {
        return;
    }
    g_pStringTableBuffer = static_cast<char *>(malloc(g_dwStringTableBufferSize));
    if (g_pStringTableBuffer != nullptr) {
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

const char *GetStringResource(unsigned nResourceId) {
    if (nResourceId < 1 || nResourceId > STRING_RESOURCE_COUNT) {
        return nullptr;
    }
    return g_apszStringResourceTable[nResourceId];
}

void FreeStringTable(void) {
    if (g_pStringTableBuffer == nullptr) {
        return;
    }
    // The entries of the table still point into the freed buffer.
    free(g_pStringTableBuffer);
    g_pStringTableBuffer = nullptr;
    g_dwStringTableUsedBytes = 0;
    g_dwStringTableBufferSize = 0;
}
