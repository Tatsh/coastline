#include "common/libultra/io/controller.h"
#include "common/libultra/io/siint.h"

s32 osEepromRead(OSMesgQueue *mq, u8 address, u8 *buffer) {
    s32 ret = 0;
    int i = 0;
    u8 *ptr = (u8 *)&__osEepPifRam.ramarray;
    OSContStatus sdata;
    __OSContEepromFormat eepromformat;

    if (address > EEPROM_MAXBLOCKS) {
        return -1;
    }
    __osSiGetAccess();
    ret = __osEepStatus(mq, &sdata);
    if (ret != 0 || sdata.type != CONT_EEPROM) {
        /* The SI is not released. */
        return CONT_NO_RESPONSE_ERROR;
    }
    while (sdata.status & CONT_EEPROM_BUSY) {
        __osEepStatus(mq, &sdata);
    }
    __osPackEepReadData(address);
    ret = __osSiRawStartDma(OS_WRITE, &__osEepPifRam);
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    /* The last word cleared is the control word. */
    for (i = 0; i < 16; i++) {
        ((u32 *)&__osEepPifRam)[i] = CONT_CMD_NOP;
    }
    __osEepPifRam.pifstatus = 0;
    ret = __osSiRawStartDma(OS_READ, &__osEepPifRam);
    __osContLastCmd = CONT_CMD_READ_EEPROM;
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    /* Each controller channel before the EEPROM holds a single skip byte. */
    for (i = 0; i < MAXCONTROLLERS; i++) {
        ptr++;
    }
    eepromformat = *(__OSContEepromFormat *)ptr;
    ret = CHNL_ERR(eepromformat);
    if (ret == 0) {
        for (i = 0; i < EEPROM_BLOCK_SIZE; i++) {
            *buffer++ = eepromformat.data[i];
        }
    }
    __osSiRelAccess();
    return ret;
}

void __osPackEepReadData(u8 address) {
    u8 *ptr = (u8 *)&__osEepPifRam.ramarray;
    __OSContEepromFormat eepromformat;
    int i;

    for (i = 0; i < 16; i++) {
        ((u32 *)&__osEepPifRam)[i] = CONT_CMD_NOP;
    }
    __osEepPifRam.pifstatus = CONT_CMD_EXE;

    eepromformat.txsize = CONT_CMD_READ_EEPROM_TX;
    eepromformat.rxsize = CONT_CMD_READ_EEPROM_RX;
    eepromformat.cmd = CONT_CMD_READ_EEPROM;
    eepromformat.address = address;
    for (i = 0; i < EEPROM_BLOCK_SIZE; i++) {
        eepromformat.data[i] = 0;
    }
    for (i = 0; i < MAXCONTROLLERS; i++) {
        *ptr++ = 0;
    }
    *(__OSContEepromFormat *)ptr = eepromformat;
    ptr += sizeof(__OSContEepromFormat);
    *ptr = CONT_CMD_END;
}
