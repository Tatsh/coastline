#include "controller.h"
#include "siint.h"

s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern) {
    s32 ret = 0;
    OSMesg dummy;
    u8 bitpattern;
    OSContStatus data[MAXCONTROLLERS];
    int channel;
    u8 bits = 0;
    int crc_error_cnt = 3;

    __osSiGetAccess();
    do {
        __osPfsRequestData(CONT_CMD_REQUEST_STATUS);
        ret = __osSiRawStartDma(OS_WRITE, &__osPfsPifRam);
        osRecvMesg(queue, &dummy, OS_MESG_BLOCK);
        ret = __osSiRawStartDma(OS_READ, &__osPfsPifRam);
        osRecvMesg(queue, &dummy, OS_MESG_BLOCK);
        __osPfsGetInitData(&bitpattern, data);

        /* A controller without an address CRC error costs one of the three attempts, and the
         * status of all of them ends the loop. */
        for (channel = 0; channel < __osMaxControllers; channel++) {
            if (!(data[channel].status & CONT_ADDR_CRC_ER)) {
                crc_error_cnt--;
                break;
            }
        }
        if (channel == __osMaxControllers) {
            crc_error_cnt = 0;
        }
    } while (crc_error_cnt > 0);

    for (channel = 0; channel < __osMaxControllers; channel++) {
        if (data[channel].errno == 0 && (data[channel].status & CONT_CARD_ON)) {
            bits |= 1 << channel;
        }
    }
    __osSiRelAccess();
    *pattern = bits;
    return ret;
}

void __osPfsRequestData(u8 cmd) {
    u8 *ptr;
    __OSContRequesFormat requestformat;
    int i;

    __osContLastCmd = cmd;
    for (i = 0; i < 16; i++) {
        ((u32 *)&__osPfsPifRam)[i] = 0;
    }
    __osPfsPifRam.pifstatus = CONT_CMD_EXE;

    ptr = (u8 *)__osPfsPifRam.ramarray;
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

void __osPfsGetInitData(u8 *pattern, OSContStatus *data) {
    u8 *ptr = (u8 *)__osPfsPifRam.ramarray;
    __OSContRequesFormat requestformat;
    int i;
    u8 bits = 0;

    for (i = 0; i < __osMaxControllers; i++, ptr += sizeof(requestformat), data++) {
        requestformat = *(__OSContRequesFormat *)ptr;
        data->errno = CHNL_ERR(requestformat);
        if (data->errno == 0) {
            data->type = (requestformat.typel << 8) | requestformat.typeh;
            data->status = requestformat.status;
            bits |= 1 << i;
        }
    }
    *pattern = bits;
}
