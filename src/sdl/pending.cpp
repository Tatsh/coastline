// ponytail: the routines of the Windows libraries the race engine calls and the SDL game does not
// implement yet. Each records its first call in the log and does nothing. Every routine here draws
// through the renderers of the Windows libraries, and the SDL renderer gives each an
// implementation.
#include <SDL3/SDL.h>

#include "common/damagetexture.h"
#include "common/racerslot.h"
#include "common/renderbackend.h"
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

void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels) {
    PENDING();
}
