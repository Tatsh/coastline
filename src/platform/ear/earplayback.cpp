#include "earplayback.h"

int EarPlayback::GetStatus() {
    return status;
}

int EarPlayback::IsActive() {
    return active;
}

void EarPlayback::MarkComplete() {
    completionRequested = 1;
}
