#include <ultra64.h>

#include "osint.h"

void osSetEventMesg(OSEvent e, OSMesgQueue *mq, OSMesg m) {
    register OSIntMask saveMask = __osDisableInt();
    __OSEventState *es = &__osEventStateTab[e];

    es->messageQueue = mq;
    es->message = m;
    __osRestoreInt(saveMask);
}
