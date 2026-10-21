#include "controller.h"
#include "siint.h"

s32 __osContRamWrite(OSMesgQueue *mq, int channel, u16 address, u8 *buffer, int force) {
    s32 ret = 0;
    u8 *ptr;
    __OSContRamReadFormat ramreadformat;
    int retry = 2;

    /* The ID and label blocks change only when the caller insists. */
    if (force != TRUE && address < PFS_LABEL_AREA && address != 0) {
        return 0;
    }

    __osSiGetAccess();
    __osContLastCmd = CONT_CMD_WRITE_MEMPACK;
    __osPackRamWriteData(channel, address, buffer);
    ret = __osSiRawStartDma(OS_WRITE, &__osPfsPifRam);
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    do {
        ret = __osSiRawStartDma(OS_READ, &__osPfsPifRam);
        osRecvMesg(mq, NULL, OS_MESG_BLOCK);

        ptr = (u8 *)&__osPfsPifRam + channel;
        bcopy(ptr, &ramreadformat, sizeof(ramreadformat));
        ret = CHNL_ERR(ramreadformat);
        if (ret == 0) {
            /* The Pak answers with the CRC of the data it received. */
            if (__osContDataCrc(buffer) != ramreadformat.datacrc) {
                ret = __osPfsGetStatus(mq, channel);
                if (ret != 0) {
                    __osSiRelAccess();
                    return ret;
                }
                ret = PFS_ERR_CONTRFAIL;
            }
        } else {
            ret = PFS_ERR_NOPACK;
        }
    } while (ret == PFS_ERR_CONTRFAIL && retry-- >= 0);
    __osSiRelAccess();
    return ret;
}

void __osPackRamWriteData(int channel, u16 address, u8 *buffer) {
    u8 *ptr = (u8 *)__osPfsPifRam.ramarray;
    __OSContRamReadFormat ramreadformat;
    int i;

    for (i = 0; i < 16; i++) {
        ((u32 *)&__osPfsPifRam)[i] = 0;
    }
    __osPfsPifRam.pifstatus = CONT_CMD_EXE;

    ramreadformat.dummy = CONT_CMD_NOP;
    ramreadformat.txsize = CONT_CMD_WRITE_MEMPACK_TX;
    ramreadformat.rxsize = CONT_CMD_WRITE_MEMPACK_RX;
    ramreadformat.cmd = CONT_CMD_WRITE_MEMPACK;
    ramreadformat.address = (address << 5) | __osContAddressCrc(address);
    ramreadformat.datacrc = CONT_CMD_NOP;
    for (i = 0; i < BLOCKSIZE; i++) {
        ramreadformat.data[i] = *buffer++;
    }

    for (i = 0; i < channel; i++) {
        *ptr++ = 0;
    }
    bcopy(&ramreadformat, ptr, sizeof(ramreadformat));
    ptr += sizeof(ramreadformat);
    *ptr = CONT_CMD_END;
}
