#include "controller.h"

u16 __osSumcalc(u8 *ptr, int length) {
    int i;
    u32 sum = 0;

    for (i = 0; i < length; i++) {
        sum += *ptr++;
        sum &= 0xFFFF;
    }
    return sum;
}

s32 __osIdCheckSum(u16 *ptr, u16 *csum, u16 *icsum) {
    u16 data = 0;
    u32 j;

    *csum = *icsum = 0;
    /* The sums cover the halfwords before the checksums. */
    for (j = 0; j < 28; j += 2) {
        data = *(u16 *)((u8 *)ptr + j);
        *csum += data;
        *icsum += ~data;
    }
    return 0;
}

s32 __osRepairPackId(OSPfs *pfs, __OSPackId *badid, __OSPackId *newid) {
    s32 ret = 0;
    u8 temp[BLOCKSIZE];
    u8 comp[BLOCKSIZE];
    u8 mask = 0;
    int i;
    int j;
    u16 index[4];

    SET_ACTIVEBANK_TO_ZERO;
    newid->repaired = -1;
    newid->random = osGetCount();
    newid->serial_mid = badid->serial_mid;
    newid->serial_low = badid->serial_low;

    /* Count the banks. Each bank gets its number written to its first block, which must read
     * back, and must not appear through bank 0 if the bank mirrors it. */
    for (j = 0; j < PFS_MAX_BANKS;) {
        pfs->activebank = j;
        ERRCK(__osPfsSelectBank(pfs));
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, 0, temp));
        temp[0] = j | 0x80;
        for (i = 1; i < BLOCKSIZE; i++) {
            temp[i] = ~temp[i];
        }
        ERRCK(__osContRamWrite(pfs->queue, pfs->channel, 0, temp, FALSE));
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, 0, comp));
        for (i = 0; i < BLOCKSIZE; i++) {
            if (comp[i] != temp[i]) {
                break;
            }
        }
        if (i != BLOCKSIZE) {
            break;
        }
        if (j > 0) {
            pfs->activebank = 0;
            ERRCK(__osPfsSelectBank(pfs));
            ERRCK(__osContRamRead(pfs->queue, pfs->channel, 0, temp));
            if (temp[0] != 0x80) {
                break;
            }
        }
        j++;
    }

    pfs->activebank = 0;
    ERRCK(__osPfsSelectBank(pfs));
    mask = (j > 0) ? 1 : 0;
    newid->deviceid = (badid->deviceid & 0xFFFE) | mask;
    newid->banks = j;
    newid->version = badid->version;
    __osIdCheckSum((u16 *)newid, &newid->checksum, &newid->inverted_checksum);

    index[0] = PFS_ID_0AREA;
    index[1] = PFS_ID_1AREA;
    index[2] = PFS_ID_2AREA;
    index[3] = PFS_ID_3AREA;
    for (i = 0; i < 4; i++) {
        ERRCK(__osContRamWrite(pfs->queue, pfs->channel, index[i], (u8 *)newid, TRUE));
    }
    ERRCK(__osContRamRead(pfs->queue, pfs->channel, PFS_ID_0AREA, temp));
    for (i = 0; i < BLOCKSIZE; i++) {
        if (temp[i] != ((u8 *)newid)[i]) {
            return PFS_ERR_ID_FATAL;
        }
    }
    return 0;
}

s32 __osCheckPackId(OSPfs *pfs, __OSPackId *temp) {
    u16 index[4];
    s32 ret = 0;
    u16 sum;
    u16 isum;
    int i, j;

    SET_ACTIVEBANK_TO_ZERO;
    index[0] = PFS_ID_0AREA;
    index[1] = PFS_ID_1AREA;
    index[2] = PFS_ID_2AREA;
    index[3] = PFS_ID_3AREA;

    /* The caller found the first copy bad, so the search starts at the second. */
    for (i = 1; i < 4; i++) {
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, index[i], (u8 *)temp));
        __osIdCheckSum((u16 *)temp, &sum, &isum);
        if (temp->checksum == sum && temp->inverted_checksum == isum) {
            break;
        }
    }
    if (i == 4) {
        return PFS_ERR_ID_FATAL;
    }
    for (j = 0; j < 4; j++) {
        if (j != i) {
            ERRCK(__osContRamWrite(pfs->queue, pfs->channel, index[j], (u8 *)temp, TRUE));
        }
    }
    return 0;
}

s32 __osGetId(OSPfs *pfs) {
    int k;
    u16 sum, isum;
    u8 temp[BLOCKSIZE];
    __OSPackId newid;
    s32 ret;
    __OSPackId *id;

    SET_ACTIVEBANK_TO_ZERO;
    ERRCK(__osContRamRead(pfs->queue, pfs->channel, PFS_ID_0AREA, temp));
    __osIdCheckSum((u16 *)temp, &sum, &isum);
    id = (__OSPackId *)temp;
    if (id->checksum != sum || id->inverted_checksum != isum) {
        ret = __osCheckPackId(pfs, id);
        if (ret == PFS_ERR_ID_FATAL) {
            ERRCK(__osRepairPackId(pfs, id, &newid));
            id = &newid;
        } else if (ret != 0) {
            return ret;
        }
    }
    if (!(id->deviceid & 1)) {
        ERRCK(__osRepairPackId(pfs, id, &newid));
        id = &newid;
        if (!(id->deviceid & 1)) {
            return PFS_ERR_DEVICE;
        }
    }

    for (k = 0; k < (int)sizeof(pfs->id); k++) {
        pfs->id[k] = ((u8 *)id)[k];
    }
    pfs->version = id->version;
    pfs->banks = id->banks;
    pfs->inode_start_page = 1 + DEF_DIR_PAGES + 2 * pfs->banks;
    pfs->dir_size = DEF_DIR_PAGES * PFS_ONE_PAGE;
    pfs->inode_table = 1 * PFS_ONE_PAGE;
    pfs->minode_table = (1 + pfs->banks) * PFS_ONE_PAGE;
    pfs->dir_table = pfs->minode_table + pfs->banks * PFS_ONE_PAGE;
    ERRCK(__osContRamRead(pfs->queue, pfs->channel, PFS_LABEL_AREA, pfs->label));
    return 0;
}

s32 __osCheckId(OSPfs *pfs) {
    int k;
    u8 temp[BLOCKSIZE];
    s32 ret;

    SET_ACTIVEBANK_TO_ZERO;
    ret = __osContRamRead(pfs->queue, pfs->channel, PFS_ID_0AREA, temp);
    if (ret != 0) {
        if (ret != PFS_ERR_NEW_PACK) {
            return ret;
        }
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, PFS_ID_0AREA, temp));
    }
    for (k = 0; k < BLOCKSIZE; k++) {
        if (pfs->id[k] != temp[k]) {
            return PFS_ERR_NEW_PACK;
        }
    }
    return 0;
}

s32 __osPfsRWInode(OSPfs *pfs, __OSInode *inode, u8 flag, u8 bank) {
    u8 sum;
    int j;
    s32 ret;
    int offset;
    u8 *addr;

    SET_ACTIVEBANK_TO_ZERO;
    /* Bank 0 starts its table with the pages of the system area, and the other banks with one
     * page of checksum. */
    offset = (bank > 0) ? 1 : pfs->inode_start_page;

    if (flag == PFS_WRITE) {
        inode->inode_page[0].inode_t.page =
            __osSumcalc((u8 *)&inode->inode_page[offset], (PFS_INODE_SIZE_PER_PAGE - offset) * 2);
    }

    for (j = 0; j < PFS_ONE_PAGE; j++) {
        addr = (u8 *)inode->inode_page + j * BLOCKSIZE;
        if (flag == PFS_WRITE) {
            ret = __osContRamWrite(
                pfs->queue, pfs->channel, pfs->inode_table + bank * PFS_ONE_PAGE + j, addr, FALSE);
            ret = __osContRamWrite(
                pfs->queue, pfs->channel, pfs->minode_table + bank * PFS_ONE_PAGE + j, addr, FALSE);
        } else {
            ret = __osContRamRead(
                pfs->queue, pfs->channel, pfs->inode_table + bank * PFS_ONE_PAGE + j, addr);
        }
        if (ret != 0) {
            return ret;
        }
    }

    if (flag == PFS_READ) {
        sum = __osSumcalc((u8 *)&inode->inode_page[offset], (PFS_INODE_SIZE_PER_PAGE - offset) * 2);
        if (sum != inode->inode_page[0].inode_t.page) {
            /* The main table is bad, so the mirror replaces it if the mirror is good. */
            for (j = 0; j < PFS_ONE_PAGE; j++) {
                addr = (u8 *)inode->inode_page + j * BLOCKSIZE;
                ret = __osContRamRead(
                    pfs->queue, pfs->channel, pfs->minode_table + bank * PFS_ONE_PAGE + j, addr);
            }
            if (sum != inode->inode_page[0].inode_t.page) {
                return PFS_ERR_INCONSISTENT;
            }
            for (j = 0; j < PFS_ONE_PAGE; j++) {
                addr = (u8 *)inode->inode_page + j * BLOCKSIZE;
                ret = __osContRamWrite(pfs->queue,
                                       pfs->channel,
                                       pfs->inode_table + bank * PFS_ONE_PAGE + j,
                                       addr,
                                       FALSE);
            }
        } else {
            for (j = 0; j < PFS_ONE_PAGE; j++) {
                addr = (u8 *)inode->inode_page + j * BLOCKSIZE;
                ret = __osContRamWrite(pfs->queue,
                                       pfs->channel,
                                       pfs->minode_table + bank * PFS_ONE_PAGE + j,
                                       addr,
                                       FALSE);
            }
        }
    }
    return 0;
}

s32 __osPfsSelectBank(OSPfs *pfs) {
    u8 temp[BLOCKSIZE];
    int i;
    s32 ret = 0;

    for (i = 0; i < BLOCKSIZE; i++) {
        temp[i] = pfs->activebank;
    }
    ret = __osContRamWrite(pfs->queue, pfs->channel, PFS_BANK_LAPPED_BY, temp, FALSE);
    return ret;
}
