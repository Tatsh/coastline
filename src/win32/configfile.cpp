#include "win32/configfile.h"

#include <cstdio>
#include <cstring>

#include "win32/app.h"
#include "win32/ghost.h"
#include "win32/season.h"

int g_dwConfigValue0 = 9;
int g_dwConfigValue1 = 9;
int g_bSpeedUnitsMph = 1;
const char g_szCRallyConfigDat[] = "c:\\RallyConfig.dat";

static inline void ApplyEquipmentConfig(void) {
    int anEquipment[5];
    memcpy(anEquipment, g_abSaveFileReadBuffer, sizeof(anEquipment));
    PlayerProfile *pProfile =
        g_dwSaveSlotPlayerIndex == 0 ? g_pPrimarySeasonSave : g_pSecondarySeasonSave;
    pProfile->handling = anEquipment[0];
    pProfile->transmission = anEquipment[1];
    pProfile->tires = anEquipment[2];
    pProfile->suspension = anEquipment[3];
    pProfile->controllerLayout = anEquipment[4];
}

bool LoadConfigData(int nConfigType, ConfigLoadArgument argument) {
    bool bAllowFailure = (argument.value & 0xff) != 0;
    if (nConfigType == CONFIG_LOAD_CURRENT_SEASON || nConfigType == CONFIG_LOAD_SEASON_FILE) {
        return LoadSeasonSaveFile(nConfigType, bAllowFailure);
    }
    if (nConfigType == CONFIG_LOAD_GHOST) {
        return LoadGhostFile(bAllowFailure);
    }
    const char *pszPath;
    unsigned nSize;
    if (nConfigType == CONFIG_LOAD_EQUIPMENT || nConfigType == CONFIG_LOAD_RAW) {
        pszPath = g_szCRallyConfigDat;
        nSize = 256;
    } else {
        // The original reuses the custom path's pointer bits as its read size.
        pszPath = argument.path;
        nSize = argument.value;
    }
    FILE *pFile = fopen(pszPath, "rb");
    if (!pFile) {
        return bAllowFailure;
    }
    unsigned nFirstSize = nConfigType == CONFIG_LOAD_EQUIPMENT ? 128 : nSize;
    if (fread(g_abSaveFileReadBuffer, 1, nFirstSize, pFile) != nFirstSize) {
        fclose(pFile);
        return false;
    }
    if (nConfigType == CONFIG_LOAD_EQUIPMENT) {
        int anValues[3];
        memcpy(anValues, g_abSaveFileReadBuffer, sizeof(anValues));
        g_dwConfigValue0 = anValues[0];
        g_dwConfigValue1 = anValues[1];
        g_bSpeedUnitsMph = anValues[2];
        NullStub();
        if (fread(g_abSaveFileReadBuffer, 1, 128, pFile) != 128) {
            fclose(pFile);
            return false;
        }
        ApplyEquipmentConfig();
        NullStub();
    }
    fclose(pFile);
    return true;
}
