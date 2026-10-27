#include <PR/os.h>

#include "input.h"
#include "rumble.h"

void StartControllerRead(void) {
    if (!g_dwContReadPending) {
        g_dwContReadPending = TRUE;
        g_wSerialInterfaceIdle = FALSE;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        osContInit(&g_SiMesgQueue, NULL, NULL);
#else
        osContStartReadData(&g_SiMesgQueue);
#endif
    }
}

void ReadControllers(void) {
    StartControllerRead();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    for (i = 0; i < 1; i++) {
#else
    for (i = 0; i < MAX_PLAYERS; i++) {
#endif
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        g_aPlayerInputState[i].TranslateControllerInput();
        g_aPlayerInputState[i].ConsumeInputFlags();
#else
        TranslateControllerInput(&g_aPlayerInputState[i]);
        ConsumeInputFlags(&g_aPlayerInputState[i]);
#endif
    }
}
