// ponytail: the routines of the Windows libraries the race engine calls and the SDL game does not
// implement yet. Each records its first call in the log and does nothing. Every routine here calls
// Windows (the renderers or the application window), and later milestones give each an SDL
// implementation.
#include <cstdlib>

#include <SDL3/SDL.h>

#include "common/damagetexture.h"
#include "common/fatalerror.h"
#include "common/racerslot.h"
#include "common/renderbackend.h"
#include "common/season.h"
#include "common/session.h"
#include "common/vehicle.h"
#include "common/vehiclecolor.h"
#include "common/vehiclemodel.h"

#define PENDING()                                                                                  \
    do {                                                                                           \
        static bool s_bLogged;                                                                     \
        if (!s_bLogged) {                                                                          \
            s_bLogged = true;                                                                      \
            SDL_Log("%s is not implemented in the SDL game yet.", __func__);                       \
        }                                                                                          \
    } while (false)

// The application and the game session.
[[noreturn]] void ShowFatalErrorScreen(const char *pszMessage) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", pszMessage);
    std::exit(EXIT_FAILURE);
}

[[noreturn]] void ShutdownApplication(int nExitCode) {
    std::exit(nExitCode);
}

void ShutdownGameSession(void) {
    PENDING();
}

// The textures of the vehicles and the racers.
void ApplyEnemyCarColorPanels(Vehicle *vehicle, RacerSlot *slot) {
    PENDING();
}

void MakeEnemyCarColorPanels(RacerSlot *slot) {
    PENDING();
}

void RefreshVehicleMaterialTextures(VehicleModel *model) {
    PENDING();
}

void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels) {
    PENDING();
}

// The renderer.
void CopyTileColumnScaled(int nTileIndex, int nSrcX, int nSrcY, int nSrcRow) {
    PENDING();
}

void CreateGlobalEffectTextures(void) {
    PENDING();
}

void GetPrecipitationTextureDimensions(int nIndex) {
    PENDING();
}
