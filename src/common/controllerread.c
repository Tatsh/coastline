#include <PR/os.h>

#include "common/game.h"
#include "common/input.h"
#include "common/rumble.h"

#if TGR_BOSS_RALLY
namespace {
    /**
     * Whether a read of the controllers is started and not yet received.
     *
     * @ghidraAddress USA: 0x8028ab6c
     * @ghidraAddress EUR: 0x8028ce18
     * @ghidraAddress JPN: 0x8028c848
     * @ghidraAddress BRD3D: 0x106c56e8
     * @ghidraAddress BRGlide: 0x106ec778
     */
    s32 g_dwContReadPending;
} // namespace
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
void InitControllersAndPaks(void) {
}
#endif

void StartControllerRead(void) {
    if (!g_dwContReadPending) {
        g_dwContReadPending = TRUE;
        g_wSerialInterfaceIdle = FALSE;
#if TGR_BOSS_RALLY
        osContInit(&g_SiMesgQueue, NULL, NULL);
#else
        osContStartReadData(&g_SiMesgQueue);
#endif
    }
}

void ReadControllers(void) {
    StartControllerRead();
#if TGR_BOSS_RALLY
    osContInit(&g_SiMesgQueue, NULL, NULL);
#else
    osRecvMesg(&g_SiMesgQueue, NULL, OS_MESG_BLOCK);
#endif
    osContGetReadData(g_aContPadData);
    g_wSerialInterfaceIdle = TRUE;
    g_dwContReadPending = FALSE;
}

void UpdateControllerState(void) {
    s32 i;

    ReadControllers();
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    /* The second player of the SDL game has the second pad. */
    for (i = 0; i < (g_dwNumLocalPlayers > 1 ? 2 : 1); i++) {
#elif TGR_BOSS_RALLY
    for (i = 0; i < 1; i++) {
#else
    for (i = 0; i < MAX_PLAYERS; i++) {
#endif
#if TGR_BOSS_RALLY
        g_aPlayerInputState[i].TranslateControllerInput();
        g_aPlayerInputState[i].ConsumeInputFlags();
#else
        TranslateControllerInput(&g_aPlayerInputState[i]);
        ConsumeInputFlags(&g_aPlayerInputState[i]);
#endif
    }
}
