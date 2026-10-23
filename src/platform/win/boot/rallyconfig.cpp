#include "platform/win/boot/rallyconfig.h"

#include <stdio.h>
#include <string.h>

#include "platform/win/boot/app.h"

// The first bytes of BossRally.cfg and the version of its layout.
#define CONFIG_FILE_MAGIC "RCfg"
#define CONFIG_FILE_VERSION 2

// The defaults of the menu selections, in the order of adwMenuSelectionBackup.
static constexpr DWORD kDefaultMenuSelections[MENU_SELECTION_COUNT] = {
    2, 0, 0, 1, 1, 1, 0, 3, 0, 0, 4, 0};

RallyConfig g_GlobalRallyConfig;

RallyConfig::RallyConfig() {
    InitRallyConfigDefaults();
}

void RallyConfig::InitRallyConfigDefaults() {
    memcpy(aInputBindingsKeyboard, g_awDefaultKeyBindingsDevice0, sizeof(aInputBindingsKeyboard));
    memcpy(aInputBindingsDevice1, g_awDefaultKeyBindingsDevice1, sizeof(aInputBindingsDevice1));
    memcpy(aInputBindingsDevice2, g_awDefaultKeyBindingsDevice2, sizeof(aInputBindingsDevice2));
    memcpy(aInputBindingsDevice3, g_awDefaultKeyBindingsDevice3, sizeof(aInputBindingsDevice3));
    dwInputDeviceType = 0;
    pActiveInputBindings = aInputBindingsKeyboard;
    bCarShadowSelection = TRUE;
    bSpecularLightingSelection = TRUE;
    bForceFeedbackEnabled = TRUE;
    memset(szSavedModemPhoneNumber, 0, sizeof(szSavedModemPhoneNumber));
    memset(szPlayerName, 0, sizeof(szPlayerName));
    dwDisplayModeWidth = 640;
    dwDisplayModeHeight = 480;
    dwDisplayModeBitDepth = 16;
    dwDisplayModeExtra = 0;
    memset(&DisplayDriverGuid, 0, sizeof(DisplayDriverGuid));
    dwSfxVolume = 9;
    dwMusicVolume = 9;
    memcpy(adwMenuSelectionBackup, kDefaultMenuSelections, sizeof(adwMenuSelectionBackup));
    memset(szTcpIpPort, 0, sizeof(szTcpIpPort));
    memset(szTcpIpAddress, 0, sizeof(szTcpIpAddress));
    bSkidMarksSelection = TRUE;
}

RallyConfig &RallyConfig::CopyRallyConfig(const RallyConfig &other) {
    *this = other;
    // The pointer of the other configuration points into its own tables.
    SelectActiveInputBindings();
    return *this;
}

// Read one field of BossRally.cfg.
static inline bool ReadConfigField(FILE *pFile, void *pField, size_t nSize) {
    return fread(pField, nSize, 1, pFile) == 1;
}

BOOL RallyConfig::LoadRallyConfigFile(const char *pszPath) {
    FILE *pFile;
    char achMagic[4];
    DWORD dwVersion;
    bool bRead;

    pFile = fopen(pszPath, "rb");
    if (pFile == NULL) {
        return FALSE;
    }
    RallyConfig backup;
    backup.CopyRallyConfig(*this);
    bRead =
        ReadConfigField(pFile, achMagic, sizeof(achMagic)) &&
        strncmp(achMagic, CONFIG_FILE_MAGIC, strlen(CONFIG_FILE_MAGIC)) == 0 &&
        ReadConfigField(pFile, &dwVersion, sizeof(dwVersion)) && dwVersion == CONFIG_FILE_VERSION &&
        ReadConfigField(pFile, &bCarShadowSelection, sizeof(bCarShadowSelection)) &&
        ReadConfigField(pFile, &bSpecularLightingSelection, sizeof(bSpecularLightingSelection)) &&
        ReadConfigField(pFile, &bForceFeedbackEnabled, sizeof(bForceFeedbackEnabled)) &&
        ReadConfigField(pFile, szSavedModemPhoneNumber, sizeof(szSavedModemPhoneNumber)) &&
        ReadConfigField(pFile, szPlayerName, sizeof(szPlayerName)) &&
        ReadConfigField(pFile, &dwDisplayModeWidth, sizeof(dwDisplayModeWidth)) &&
        ReadConfigField(pFile, &dwDisplayModeHeight, sizeof(dwDisplayModeHeight)) &&
        ReadConfigField(pFile, &dwDisplayModeBitDepth, sizeof(dwDisplayModeBitDepth)) &&
        ReadConfigField(pFile, &dwDisplayModeExtra, sizeof(dwDisplayModeExtra)) &&
        ReadConfigField(pFile, &DisplayDriverGuid, sizeof(DisplayDriverGuid)) &&
        ReadConfigField(pFile, &dwSfxVolume, sizeof(dwSfxVolume)) &&
        ReadConfigField(pFile, &dwMusicVolume, sizeof(dwMusicVolume));
    // Each selection is read on its own.
    for (int i = 0; bRead && i < MENU_SELECTION_COUNT; i++) {
        bRead = ReadConfigField(pFile, &adwMenuSelectionBackup[i], sizeof(DWORD));
    }
    // The bindings come last, after the device that selects them.
    bRead = bRead && ReadConfigField(pFile, szTcpIpPort, sizeof(szTcpIpPort)) &&
            ReadConfigField(pFile, szTcpIpAddress, sizeof(szTcpIpAddress)) &&
            ReadConfigField(pFile, &bSkidMarksSelection, sizeof(bSkidMarksSelection)) &&
            ReadConfigField(pFile, &dwInputDeviceType, sizeof(dwInputDeviceType)) &&
            ReadConfigField(pFile, aInputBindingsKeyboard, sizeof(aInputBindingsKeyboard)) &&
            ReadConfigField(pFile, aInputBindingsDevice1, sizeof(aInputBindingsDevice1)) &&
            ReadConfigField(pFile, aInputBindingsDevice2, sizeof(aInputBindingsDevice2)) &&
            ReadConfigField(pFile, aInputBindingsDevice3, sizeof(aInputBindingsDevice3));
    if (bRead) {
        fclose(pFile);
        return TRUE;
    }
    // The settings before the read are restored and then replaced by the defaults.
    CopyRallyConfig(backup);
    fclose(pFile);
    InitRallyConfigDefaults();
    return FALSE;
}

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
