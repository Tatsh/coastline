#include "common/libultra/io/controller.h"
#include "common/libultra/io/siint.h"

s32 osEepromWrite(OSMesgQueue *mq, u8 address, u8 *buffer) {
    s32 ret = 0;
    int i;
    u8 *ptr = (u8 *)&__osEepPifRam.ramarray;
    __OSContEepromFormat eepromformat;
    OSContStatus sdata;

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
    __osPackEepWriteData(address, buffer);
    ret = __osSiRawStartDma(OS_WRITE, &__osEepPifRam);
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    /* The last word cleared is the control word. */
    for (i = 0; i < 16; i++) {
        ((u32 *)&__osEepPifRam)[i] = CONT_CMD_NOP;
    }
    __osEepPifRam.pifstatus = 0;
    ret = __osSiRawStartDma(OS_READ, &__osEepPifRam);
    __osContLastCmd = CONT_CMD_WRITE_EEPROM;
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    /* Each controller channel before the EEPROM holds a single skip byte. */
    for (i = 0; i < MAXCONTROLLERS; i++) {
        ptr++;
    }
    eepromformat = *(__OSContEepromFormat *)ptr;
    ret = CHNL_ERR(eepromformat);
    __osSiRelAccess();
    return ret;
}

void __osPackEepWriteData(u8 address, u8 *buffer) {
    u8 *ptr = (u8 *)&__osEepPifRam.ramarray;
    __OSContEepromFormat eepromformat;
    int i;

    for (i = 0; i < 16; i++) {
        ((u32 *)&__osEepPifRam)[i] = CONT_CMD_NOP;
    }
    __osEepPifRam.pifstatus = CONT_CMD_EXE;

    eepromformat.txsize = CONT_CMD_WRITE_EEPROM_TX;
    eepromformat.rxsize = CONT_CMD_WRITE_EEPROM_RX;
    eepromformat.cmd = CONT_CMD_WRITE_EEPROM;
    eepromformat.address = address;
    for (i = 0; i < EEPROM_BLOCK_SIZE; i++) {
        eepromformat.data[i] = *buffer++;
    }
    for (i = 0; i < MAXCONTROLLERS; i++) {
        *ptr++ = 0;
    }
    *(__OSContEepromFormat *)ptr = eepromformat;
    ptr += sizeof(__OSContEepromFormat);
    *ptr = CONT_CMD_END;
}

s32 __osEepStatus(OSMesgQueue *mq, OSContStatus *data) {
    s32 ret = 0;
    int i;
    u8 *ptr = (u8 *)__osEepPifRam.ramarray;
    __OSContRequesFormat requestformat;

    for (i = 0; i < 16; i++) {
        ((u32 *)&__osEepPifRam)[i] = 0;
    }
    __osEepPifRam.pifstatus = CONT_CMD_EXE;

    ptr = (u8 *)__osEepPifRam.ramarray;
    for (i = 0; i < MAXCONTROLLERS; i++) {
        *ptr++ = 0;
    }
    requestformat.dummy = CONT_CMD_NOP;
    requestformat.txsize = CONT_CMD_REQUEST_STATUS_TX;
    requestformat.rxsize = CONT_CMD_REQUEST_STATUS_RX;
    requestformat.cmd = CONT_CMD_REQUEST_STATUS;
    requestformat.typeh = CONT_CMD_NOP;
    requestformat.typel = CONT_CMD_NOP;
    requestformat.status = CONT_CMD_NOP;
    requestformat.dummy1 = CONT_CMD_NOP;
    *(__OSContRequesFormat *)ptr = requestformat;
    ptr += sizeof(__OSContRequesFormat);
    *ptr = CONT_CMD_END;

    ret = __osSiRawStartDma(OS_WRITE, &__osEepPifRam);
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    /* The status request records the command of an EEPROM write. */
    __osContLastCmd = CONT_CMD_WRITE_EEPROM;
    ret = __osSiRawStartDma(OS_READ, &__osEepPifRam);
    osRecvMesg(mq, NULL, OS_MESG_BLOCK);
    if (ret != 0) {
        return ret;
    }

    /* The skip bytes of the response are cleared again as they are passed. */
    ptr = (u8 *)&__osEepPifRam;
    for (i = 0; i < MAXCONTROLLERS; i++) {
        *ptr++ = 0;
    }
    requestformat = *(__OSContRequesFormat *)ptr;
    data->CONT_ERRNO = CHNL_ERR(requestformat);
    data->type = (requestformat.typel << 8) | requestformat.typeh;
    data->status = requestformat.status;
    if (data->CONT_ERRNO != 0) {
        return data->CONT_ERRNO;
    }
    return 0;
}
