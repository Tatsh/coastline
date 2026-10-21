#include <ultra64.h>

#include "osint.h"

OSTimer __osBaseTimer;
OSTime __osCurrentTime;
u32 __osBaseCounter;
u32 __osViIntrCount;
u32 __osTimerCounter;
OSTimer *__osTimerList = &__osBaseTimer;

void __osTimerServicesInit(void) {
    __osCurrentTime = 0;
    __osBaseCounter = 0;
    __osViIntrCount = 0;
    __osTimerList->prev = __osTimerList;
    __osTimerList->next = __osTimerList->prev;
    __osTimerList->value = 0;
    __osTimerList->interval = __osTimerList->value;
    __osTimerList->mq = NULL;
    __osTimerList->msg = NULL;
}

void __osTimerInterrupt(void) {
    OSTimer *t;
    u32 count;
    u32 elapsedCycles;

    if (__osTimerList->next == __osTimerList) {
        return;
    }
    for (;;) {
        t = __osTimerList->next;
        if (t == __osTimerList) {
            __osSetCompare(0);
            __osTimerCounter = 0;
            break;
        }
        count = osGetCount();
        elapsedCycles = count - __osTimerCounter;
        __osTimerCounter = count;
        if (elapsedCycles < t->value) {
            t->value -= elapsedCycles;
            __osSetTimerIntr(t->value);
            return;
        }
        t->prev->next = t->next;
        t->next->prev = t->prev;
        t->next = NULL;
        t->prev = NULL;
        if (t->mq != NULL) {
            osSendMesg(t->mq, t->msg, OS_MESG_NOBLOCK);
        }
        if (t->interval != 0) {
            t->value = t->interval;
            __osInsertTimer(t);
        }
    }
}

void __osSetTimerIntr(OSTime value) {
    OSTime newValue;
    OSIntMask savedMask = __osDisableInt();

    __osTimerCounter = osGetCount();
    newValue = value + __osTimerCounter;
    __osSetCompare((u32)newValue);
    __osRestoreInt(savedMask);
}

OSTime __osInsertTimer(OSTimer *t) {
    OSTimer *timep;
    OSTime tim;
    OSIntMask savedMask = __osDisableInt();

    for (timep = __osTimerList->next, tim = t->value; timep != __osTimerList && tim > timep->value;
         timep = timep->next) {
        tim -= timep->value;
    }
    t->value = tim;
    if (timep != __osTimerList) {
        timep->value -= tim;
    }
    t->next = timep;
    t->prev = timep->prev;
    timep->prev->next = t;
    timep->prev = t;
    __osRestoreInt(savedMask);
    return tim;
}
