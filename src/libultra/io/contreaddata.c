#include "common/types.h"

#if TGR_PC
#include <PR/os_cont.h>

#include "win32/inputpoll.h"
#else
#include "controller.h"
#include "siint.h"
#endif

#if TGR_PC

void osContGetReadData(OSContPad *data) {
    int steering;
    int acceleration;
    unsigned buttons = ReadInputDevicesButtonMask(&steering, &acceleration);

    data->stick_x = steering;
    data->stick_y = acceleration;
    data->button = 0;
    if (buttons & 0x10) {
        data->button = A_BUTTON | D_JPAD;
    }
    if (buttons & 0x4) {
        data->button |= A_BUTTON | U_JPAD;
    }
    if (buttons & 0x1) {
        data->button |= L_JPAD;
    }
    if (buttons & 0x2) {
        data->button |= R_JPAD;
    }
    if (buttons & 0x8) {
        data->button |= B_BUTTON;
    }
    if (buttons & 0x100) {
        data->button |= U_CBUTTONS;
    }
    if (buttons & 0x200) {
        data->button |= L_CBUTTONS;
    }
    if (buttons & 0x400) {
        data->button |= D_CBUTTONS;
    }
    if (buttons & 0x8000) {
        data->button |= START_BUTTON;
    }
    if (buttons & 0x20) {
        data->button |= R_TRIG;
    }
    if (buttons & 0x40) {
        data->button |= L_TRIG;
    }
}

#else

s32 osContStartReadData(OSMesgQueue *mq) {
    s32 ret = 0;
    int i;

    __osSiGetAccess();
    /* A button read already in the PIF RAM needs no new write. */
    if (__osContLastCmd != CONT_CMD_READ_BUTTON) {
        __osPackReadData();
        ret = __osSiRawStartDma(OS_WRITE, &__osContPifRam);
        osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    }
    for (i = 0; i < 16; i++) {
        ((u32 *)&__osContPifRam)[i] = 0xFF;
    }
    __osContPifRam.pifstatus = 0;
    ret = __osSiRawStartDma(OS_READ, &__osContPifRam);
    __osContLastCmd = CONT_CMD_READ_BUTTON;
    __osSiRelAccess();
    return ret;
}

void osContGetReadData(OSContPad *data) {
    u8 *ptr = (u8 *)__osContPifRam.ramarray;
    __OSContReadFormat readformat;
    int i;

    for (i = 0; i < __osMaxControllers; i++, ptr += sizeof(readformat), data++) {
        readformat = *(__OSContReadFormat *)ptr;
        data->CONT_ERRNO = CHNL_ERR(readformat);
        if (data->CONT_ERRNO == 0) {
            data->button = readformat.button;
            data->stick_x = readformat.stick_x;
            data->stick_y = readformat.stick_y;
        }
    }
}

void __osPackReadData(void) {
    u8 *ptr = (u8 *)__osContPifRam.ramarray;
    __OSContReadFormat readformat;
    int i;

    for (i = 0; i < 16; i++) {
        ((u32 *)&__osContPifRam)[i] = 0;
    }
    __osContPifRam.pifstatus = CONT_CMD_EXE;

    readformat.dummy = CONT_CMD_NOP;
    readformat.txsize = CONT_CMD_READ_BUTTON_TX;
    readformat.rxsize = CONT_CMD_READ_BUTTON_RX;
    readformat.cmd = CONT_CMD_READ_BUTTON;
    readformat.button = 0xFFFF;
    readformat.stick_x = -1;
    readformat.stick_y = -1;

    for (i = 0; i < __osMaxControllers; i++) {
        *(__OSContReadFormat *)ptr = readformat;
        ptr += sizeof(readformat);
    }
    *ptr = CONT_CMD_END;
}
#endif /* TGR_PC */
