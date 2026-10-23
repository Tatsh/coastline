#include "platform/win/boot/rallyconfig.h"

#include <string.h>

#include "platform/win/boot/app.h"

void ReadInstallDirectoryFromRegistry(void) {
    HKEY hKey;
    DWORD dwSize;
    LONG nResult;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\SouthPeak Interactive\\Boss Rally",
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS) {
        dwSize = MAX_PATH;
        nResult = RegQueryValueExA(
            hKey, "Directory", NULL, NULL, reinterpret_cast<BYTE *>(g_szInstallDirectory), &dwSize);
        RegCloseKey(hKey);
        if (nResult == ERROR_SUCCESS) {
            if (g_szInstallDirectory[strlen(g_szInstallDirectory) - 1] != '\\') {
                strcat(g_szInstallDirectory, "\\");
            }
            return;
        }
    }
    strcpy(g_szInstallDirectory, "c:\\");
}
