#include "win32/rallyconfig.h"

#include <cstdio>
#include <cstring>

#include "win32/app.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include <string>

#include "sdl/gamefiles.h"
#endif

// The first bytes of BossRally.cfg and the version of its layout.
#define CONFIG_FILE_MAGIC "RCfg"
#define CONFIG_FILE_VERSION 2

// The defaults of the menu selections, in the order of adwMenuSelectionBackup.
static constexpr int kDefaultMenuSelections[MENU_SELECTION_COUNT] = {
    2, 0, 0, 1, 1, 1, 0, 3, 0, 0, 4, 0};

RallyConfig g_GlobalRallyConfig;

// The default bindings of a device, 1 to 3 for a game controller and any other for the keyboard.
static inline InputBinding *GetDefaultBindings(int nDevice) {
    switch (nDevice) {
    case 1:
        return g_awDefaultKeyBindingsDevice1;
    case 2:
        return g_awDefaultKeyBindingsDevice2;
    case 3:
        return g_awDefaultKeyBindingsDevice3;
    default:
        return g_awDefaultKeyBindingsDevice0;
    }
}

RallyConfig::RallyConfig() {
    InitRallyConfigDefaults();
}

void RallyConfig::LoadDeviceDefaultBindings(int nDevice) {
    memcpy(GetDeviceBindings(nDevice),
           GetDefaultBindings(nDevice),
           sizeof(InputBinding[INPUT_BINDING_COUNT]));
}

void RallyConfig::SetInputBinding(int nDevice, int nControl, int nKind, int nKeyCode) {
    InputBinding *pBindings = GetDeviceBindings(nDevice);
    InputBinding *pBinding = &pBindings[nControl];
    const InputBinding *pDefault = &GetDefaultBindings(nDevice)[nControl];

    pBinding->wControl = (nKind & 0xff00) | (nKeyCode & 0xff);
    for (int i = 0; i < 2; i++) {
        // The original repeats the same work for each control of the table.
        for (int nPass = 0; nPass < INPUT_BINDING_COUNT; nPass++) {
            pBinding->awAlternateControls[i] = pDefault->awAlternateControls[i];
            for (int j = 0; j < INPUT_BINDING_COUNT; j++) {
                if (pBindings[j].wControl == pBinding->awAlternateControls[i]) {
                    pBinding->awAlternateControls[i] = 0;
                    break;
                }
            }
        }
    }
}

int RallyConfig::GetInputBindingFlags(int nDevice, int nControl) {
    return GetDeviceBindings(nDevice)[nControl].wControl & 0xff00;
}

int RallyConfig::GetInputBindingKeyCode(int nDevice, int nControl) {
    int wControl = GetDeviceBindings(nDevice)[nControl].wControl;

    if (nDevice >= 1 && nDevice <= 3 && wControl >= 0x8000) {
        return wControl >> 8;
    }
    return wControl & 0xff;
}

void RallyConfig::InitRallyConfigDefaults() {
    memcpy(aInputBindingsKeyboard, g_awDefaultKeyBindingsDevice0, sizeof(aInputBindingsKeyboard));
    memcpy(aInputBindingsDevice1, g_awDefaultKeyBindingsDevice1, sizeof(aInputBindingsDevice1));
    memcpy(aInputBindingsDevice2, g_awDefaultKeyBindingsDevice2, sizeof(aInputBindingsDevice2));
    memcpy(aInputBindingsDevice3, g_awDefaultKeyBindingsDevice3, sizeof(aInputBindingsDevice3));
    dwInputDeviceType = 0;
    pActiveInputBindings = aInputBindingsKeyboard;
    bCarShadowSelection = true;
    bSpecularLightingSelection = true;
    bForceFeedbackEnabled = true;
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
    bSkidMarksSelection = true;
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

bool RallyConfig::LoadRallyConfigFile(const char *pszPath) {
    FILE *pFile;
    char achMagic[4];
    int dwVersion;
    bool bRead;

    pFile = fopen(pszPath, "rb");
    if (pFile == nullptr) {
        return false;
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
        bRead =
            ReadConfigField(pFile, &adwMenuSelectionBackup[i], sizeof(adwMenuSelectionBackup[i]));
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
        return true;
    }
    // The settings before the read are restored and then replaced by the defaults.
    CopyRallyConfig(backup);
    fclose(pFile);
    InitRallyConfigDefaults();
    return false;
}

// Write one field of BossRally.cfg.
static inline bool WriteConfigField(FILE *pFile, const void *pField, size_t nSize) {
    return fwrite(pField, nSize, 1, pFile) == 1;
}

bool RallyConfig::SaveConfigToFile(const char *pszPath) {
    FILE *pFile;
    int dwVersion = CONFIG_FILE_VERSION;
    bool bWritten;

    pFile = fopen(pszPath, "wb");
    if (pFile == nullptr) {
        return false;
    }
    bWritten =
        WriteConfigField(pFile, CONFIG_FILE_MAGIC, strlen(CONFIG_FILE_MAGIC)) &&
        WriteConfigField(pFile, &dwVersion, sizeof(dwVersion)) &&
        WriteConfigField(pFile, &bCarShadowSelection, sizeof(bCarShadowSelection)) &&
        WriteConfigField(pFile, &bSpecularLightingSelection, sizeof(bSpecularLightingSelection)) &&
        WriteConfigField(pFile, &bForceFeedbackEnabled, sizeof(bForceFeedbackEnabled)) &&
        WriteConfigField(pFile, szSavedModemPhoneNumber, sizeof(szSavedModemPhoneNumber)) &&
        WriteConfigField(pFile, szPlayerName, sizeof(szPlayerName)) &&
        WriteConfigField(pFile, &dwDisplayModeWidth, sizeof(dwDisplayModeWidth)) &&
        WriteConfigField(pFile, &dwDisplayModeHeight, sizeof(dwDisplayModeHeight)) &&
        WriteConfigField(pFile, &dwDisplayModeBitDepth, sizeof(dwDisplayModeBitDepth)) &&
        WriteConfigField(pFile, &dwDisplayModeExtra, sizeof(dwDisplayModeExtra)) &&
        WriteConfigField(pFile, &DisplayDriverGuid, sizeof(DisplayDriverGuid)) &&
        WriteConfigField(pFile, &dwSfxVolume, sizeof(dwSfxVolume)) &&
        WriteConfigField(pFile, &dwMusicVolume, sizeof(dwMusicVolume));
    for (int i = 0; bWritten && i < MENU_SELECTION_COUNT; i++) {
        bWritten =
            WriteConfigField(pFile, &adwMenuSelectionBackup[i], sizeof(adwMenuSelectionBackup[i]));
    }
    bWritten = bWritten && WriteConfigField(pFile, szTcpIpPort, sizeof(szTcpIpPort)) &&
               WriteConfigField(pFile, szTcpIpAddress, sizeof(szTcpIpAddress)) &&
               WriteConfigField(pFile, &bSkidMarksSelection, sizeof(bSkidMarksSelection)) &&
               WriteConfigField(pFile, &dwInputDeviceType, sizeof(dwInputDeviceType)) &&
               WriteConfigField(pFile, aInputBindingsKeyboard, sizeof(aInputBindingsKeyboard)) &&
               WriteConfigField(pFile, aInputBindingsDevice1, sizeof(aInputBindingsDevice1)) &&
               WriteConfigField(pFile, aInputBindingsDevice2, sizeof(aInputBindingsDevice2)) &&
               WriteConfigField(pFile, aInputBindingsDevice3, sizeof(aInputBindingsDevice3));
    fclose(pFile);
    return bWritten;
}

#if TGR_PLATFORM == TGR_PLATFORM_SDL
void ReadInstallDirectoryFromRegistry(void) {
    // The SDL game writes what the PC release writes to its installation to the state directory.
    std::string directory = coastline::WritableGameFile("").string();
    if (directory.empty() || directory.back() != '/') {
        directory += '/';
    }
    strncpy(g_szInstallDirectory, directory.c_str(), MAX_PATH - 1);
    g_szInstallDirectory[MAX_PATH - 1] = '\0';
}
#else
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
        nResult = RegQueryValueExA(hKey,
                                   "Directory",
                                   nullptr,
                                   nullptr,
                                   reinterpret_cast<BYTE *>(g_szInstallDirectory),
                                   &dwSize);
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
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */
