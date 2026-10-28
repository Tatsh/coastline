#include "registry.h"

#include <string.h>

#include <windows.h>

char g_szInstallDir[260];

void ReadInstallDirectoryFromRegistry(void) {
    HKEY key;
    if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE, "SOFTWARE\\SouthPeak Interactive\\Boss Rally", 0, KEY_READ, &key) ==
        ERROR_SUCCESS) {
        DWORD bytes = sizeof(g_szInstallDir);
        LSTATUS result =
            RegQueryValueExA(key, "Directory", nullptr, nullptr, (BYTE *)g_szInstallDir, &bytes);
        RegCloseKey(key);
        if (result == ERROR_SUCCESS) {
#ifdef ENABLE_PATCHES
            // An empty or unterminated directory takes the default below.
            size_t length = strnlen(g_szInstallDir, sizeof(g_szInstallDir));
            if (length != 0 && length < sizeof(g_szInstallDir)) {
                if (g_szInstallDir[length - 1] != '\\' && length + 1 < sizeof(g_szInstallDir)) {
                    strcat(g_szInstallDir, "\\");
                }
                return;
            }
#else
            if (g_szInstallDir[strlen(g_szInstallDir) - 1] != '\\') {
                strcat(g_szInstallDir, "\\");
            }
            return;
#endif
        }
    }
    strcpy(g_szInstallDir, "c:\\");
}
