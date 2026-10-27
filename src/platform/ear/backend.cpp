#include "backend.h"

std::int32_t g_earAudioComponentsValidated;

int __stdcall GetDirectSoundVersion() {
    return QueryDirectSoundProductVersion();
}
