#include "common/debugmessage.h"

#include "common/fatalerror.h"
#include "common/types.h"

const char *g_pszDebugMessage;

[[noreturn]] void ShowDebugMessageFatalError(s32 unused) {
    ShowFatalErrorScreen(g_pszDebugMessage);
}

void SetDebugMessage(const char *message) {
    g_pszDebugMessage = message;
    ShowDebugMessageFatalError(0);
}
