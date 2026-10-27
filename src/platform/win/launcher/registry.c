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
            if (g_szInstallDir[strlen(g_szInstallDir) - 1] != '\\') {
                strcat(g_szInstallDir, "\\");
            }
            return;
        }
    }
    strcpy(g_szInstallDir, "c:\\");
}
