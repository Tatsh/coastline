#if TGR_BOSS_RALLY
#include <PR/os_cont.h>
#else
#include "common/libultra/io/controller.h"
#include "common/libultra/io/siint.h"

/* The controllers need half a second after power on before they answer. */
#define CONT_POWER_UP_USEC 500000
#endif

s32 osContInit(OSMesgQueue *mq, u8 *bitpattern, OSContStatus *data) {
#if TGR_BOSS_RALLY
    return 1;
#else
    OSMesg dummy;
    s32 ret = 0;
    OSTime t;
    OSTimer mytimer;
    OSMesgQueue timerMesgQueue;

    if (__osContinitialized) {
        return 0;
    }
    __osContinitialized = 1;

    t = osGetTime();
    if (t < (u64)CONT_POWER_UP_USEC * osClockRate / 1000000) {
        osCreateMesgQueue(&timerMesgQueue, &dummy, 1);
        osSetTimer(&mytimer,
                   (u64)CONT_POWER_UP_USEC * osClockRate / 1000000 - t,
                   0,
                   &timerMesgQueue,
                   &dummy);
        osRecvMesg(&timerMesgQueue, &dummy, OS_MESG_BLOCK);
    }

    __osMaxControllers = MAXCONTROLLERS;
    __osPackRequestData(CONT_CMD_REQUEST_STATUS);
    ret = __osSiRawStartDma(OS_WRITE, &__osContPifRam);
    osRecvMesg(mq, &dummy, OS_MESG_BLOCK);
    ret = __osSiRawStartDma(OS_READ, &__osContPifRam);
    osRecvMesg(mq, &dummy, OS_MESG_BLOCK);
    __osContGetInitData(bitpattern, data);
    __osContLastCmd = CONT_CMD_REQUEST_STATUS;
    __osSiCreateAccessQueue();
    osCreateMesgQueue(&__osEepromTimerQ, &__osEepromTimerMsg, 1);
    return ret;
#endif /* TGR_BOSS_RALLY */
}

#if TGR_PLATFORM == TGR_PLATFORM_N64
void __osContGetInitData(u8 *pattern, OSContStatus *data) {
    u8 *ptr;
    __OSContRequesFormat requestformat;
    int i;
    u8 bits;

    bits = 0;
    ptr = (u8 *)__osContPifRam.ramarray;
    for (i = 0; i < __osMaxControllers; i++, ptr += sizeof(requestformat), data++) {
        requestformat = *(__OSContRequesFormat *)ptr;
        data->CONT_ERRNO = CHNL_ERR(requestformat);
        if (data->CONT_ERRNO == 0) {
            data->type = (requestformat.typel << 8) | requestformat.typeh;
            data->status = requestformat.status;
            bits |= 1 << i;
        }
    }
    *pattern = bits;
}

void __osPackRequestData(u8 cmd) {
    u8 *ptr;
    __OSContRequesFormat requestformat;
    int i;

    /* The loop clears the status word too, and the next store sets it. */
    for (i = 0; i < 16; i++) {
        ((u32 *)&__osContPifRam)[i] = 0;
    }
    __osContPifRam.pifstatus = CONT_CMD_EXE;

    ptr = (u8 *)__osContPifRam.ramarray;
    requestformat.dummy = CONT_CMD_NOP;
    requestformat.txsize = CONT_CMD_REQUEST_STATUS_TX;
    requestformat.rxsize = CONT_CMD_REQUEST_STATUS_RX;
    requestformat.cmd = cmd;
    requestformat.typeh = CONT_CMD_NOP;
    requestformat.typel = CONT_CMD_NOP;
    requestformat.status = CONT_CMD_NOP;
    requestformat.dummy1 = CONT_CMD_NOP;

    for (i = 0; i < __osMaxControllers; i++) {
        *(__OSContRequesFormat *)ptr = requestformat;
        ptr += sizeof(requestformat);
    }
    *ptr = CONT_CMD_END;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
