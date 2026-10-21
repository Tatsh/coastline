#include "controller.h"
#include "siint.h"

s32 osMotorStop(OSPfs *pfs) {
    s32 ret;
    u8 *ptr;
    __OSContRamReadFormat ramreadformat;

    __osSiGetAccess();
    __osContLastCmd = CONT_CMD_WRITE_MEMPACK;
    __osSiRawStartDma(OS_WRITE, &_MotorStopData[pfs->channel]);
    osRecvMesg(pfs->queue, NULL, OS_MESG_BLOCK);
    ret = __osSiRawStartDma(OS_READ, &__osPfsPifRam);
    osRecvMesg(pfs->queue, NULL, OS_MESG_BLOCK);

    ptr = (u8 *)&__osPfsPifRam + pfs->channel;
    bcopy(ptr, &ramreadformat, sizeof(ramreadformat));
    ret = CHNL_ERR(ramreadformat);
    /* The Pak answers with the CRC of the block it received, which is 0 for the block of 0s. */
    if (ret == 0 && ramreadformat.datacrc != 0) {
        ret = PFS_ERR_CONTRFAIL;
    }
    __osSiRelAccess();
    return ret;
}

s32 osMotorStart(OSPfs *pfs) {
    s32 ret;
    u8 *ptr;
    __OSContRamReadFormat ramreadformat;

    __osSiGetAccess();
    __osContLastCmd = CONT_CMD_WRITE_MEMPACK;
    __osSiRawStartDma(OS_WRITE, &_MotorStartData[pfs->channel]);
    osRecvMesg(pfs->queue, NULL, OS_MESG_BLOCK);
    ret = __osSiRawStartDma(OS_READ, &__osPfsPifRam);
    osRecvMesg(pfs->queue, NULL, OS_MESG_BLOCK);

    ptr = (u8 *)&__osPfsPifRam + pfs->channel;
    bcopy(ptr, &ramreadformat, sizeof(ramreadformat));
    ret = CHNL_ERR(ramreadformat);
    /* 0xEB is the CRC of the block of 1s. */
    if (ret == 0 && ramreadformat.datacrc != 0xEB) {
        ret = PFS_ERR_CONTRFAIL;
    }
    __osSiRelAccess();
    return ret;
}

/* Build the PIF RAM image of a block write to the motor. Unlike the other packing routines, it
 * leaves the status word to the store after the loop. */
static void _MakeMotorData(int channel, u16 address, u8 *buffer, OSPifRam *mdata) {
    u8 *ptr = (u8 *)mdata->ramarray;
    __OSContRamReadFormat ramreadformat;
    int i;

    for (i = 0; i < 15; i++) {
        mdata->ramarray[i] = 0;
    }
    mdata->pifstatus = CONT_CMD_EXE;

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

s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel) {
    int i;
    s32 ret;
    u8 temp[BLOCKSIZE];

    pfs->queue = mq;
    pfs->channel = channel;
    pfs->status = 0;
    pfs->activebank = 0x80;

    /* A Rumble Pak reads back 0x80 from its bank register, which a Controller Pak does not. */
    for (i = 0; i < BLOCKSIZE; i++) {
        temp[i] = 0x80;
    }
    ret = __osContRamWrite(mq, channel, PFS_BANK_LAPPED_BY, temp, FALSE);
    if (ret == PFS_ERR_NEW_PACK) {
        ret = __osContRamWrite(mq, channel, PFS_BANK_LAPPED_BY, temp, FALSE);
    }
    if (ret != 0) {
        return ret;
    }
    ret = __osContRamRead(mq, channel, PFS_BANK_LAPPED_BY, temp);
    if (ret == PFS_ERR_NEW_PACK) {
        ret = PFS_ERR_CONTRFAIL;
    }
    if (ret != 0) {
        return ret;
    }
    if (temp[BLOCKSIZE - 1] != 0x80) {
        return PFS_ERR_DEVICE;
    }

    for (i = 0; i < BLOCKSIZE; i++) {
        _motorstartbuf[i] = 1;
        _motorstopbuf[i] = 0;
    }
    _MakeMotorData(channel, MOTOR_ADDRESS, _motorstartbuf, &_MotorStartData[channel]);
    _MakeMotorData(channel, MOTOR_ADDRESS, _motorstopbuf, &_MotorStopData[channel]);
    return 0;
}
