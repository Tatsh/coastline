#include "controller.h"

/* Whether a page table entry names a page of the Pak that a file may use. */
#define VALID_PAGE(pfs, unit)                                                                      \
    ((unit).ipage >= (pfs)->inode_start_page && (unit).inode_t.bank < (pfs)->banks &&              \
     (unit).inode_t.page > 0 && (unit).inode_t.page < PFS_INODE_SIZE_PER_PAGE)

/* Follow a page chain one step, reading the page table of a bank the chain enters.
 * @ghidraAddress USA: 0x802634e0 */
static s32 __osPfsGetNextPage(OSPfs *pfs, u8 *bank, __OSInode *inode, __OSInodeUnit *page) {
    s32 ret;

    if (page->inode_t.bank != *bank) {
        *bank = page->inode_t.bank;
        ERRCK(__osPfsRWInode(pfs, inode, PFS_READ, *bank));
    }
    *page = inode->inode_page[page->inode_t.page];
    if (!VALID_PAGE(pfs, *page)) {
        if (page->ipage == PFS_EOF) {
            return PFS_ERR_INVALID;
        }
        return PFS_ERR_INCONSISTENT;
    }
    return 0;
}

s32 osPfsReadWriteFile(
    OSPfs *pfs, s32 file_no, u8 flag, int offset, int size_in_bytes, u8 *data_buffer) {
    int cur_block;
    int siz_block;
    u8 *buffer;
    __OSDir dir;
    __OSInode inode;
    __OSInodeUnit cur_page;
    u16 blockno;
    u8 bank;
    s32 ret;

    if (file_no >= pfs->dir_size || file_no < 0) {
        return PFS_ERR_INVALID;
    }
    if (size_in_bytes <= 0 || (size_in_bytes % BLOCKSIZE) != 0) {
        return PFS_ERR_INVALID;
    }
    if (offset < 0 || (offset % BLOCKSIZE) != 0) {
        return PFS_ERR_INVALID;
    }
    if (!(pfs->status & PFS_INITIALIZED)) {
        return PFS_ERR_INVALID;
    }
    if (__osCheckId(pfs) == PFS_ERR_NEW_PACK) {
        return PFS_ERR_NEW_PACK;
    }

    SET_ACTIVEBANK_TO_ZERO;
    ERRCK(__osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + file_no, (u8 *)&dir));
    if (!VALID_PAGE(pfs, dir.start_page)) {
        if (dir.start_page.ipage == PFS_EOF) {
            return PFS_ERR_INVALID;
        }
        return PFS_ERR_INCONSISTENT;
    }
    if (flag == PFS_READ && !(dir.status & DIR_STATUS_OCCUPIED)) {
        return PFS_ERR_BAD_DATA;
    }

    /* Each page holds 8 blocks, so the chain advances one page per 8 blocks of offset. */
    bank = 0xFF;
    cur_block = offset / BLOCKSIZE;
    cur_page = dir.start_page;
    while (cur_block >= PFS_ONE_PAGE) {
        ERRCK(__osPfsGetNextPage(pfs, &bank, &inode, &cur_page));
        cur_block -= PFS_ONE_PAGE;
    }

    siz_block = size_in_bytes / BLOCKSIZE;
    buffer = data_buffer;
    while (siz_block > 0) {
        if (cur_block == PFS_ONE_PAGE) {
            ERRCK(__osPfsGetNextPage(pfs, &bank, &inode, &cur_page));
            cur_block = 0;
        }
        if (pfs->activebank != cur_page.inode_t.bank) {
            pfs->activebank = cur_page.inode_t.bank;
            ERRCK(__osPfsSelectBank(pfs));
        }
        blockno = cur_page.inode_t.page * PFS_ONE_PAGE + cur_block;
        if (flag == PFS_READ) {
            ret = __osContRamRead(pfs->queue, pfs->channel, blockno, buffer);
        } else {
            ret = __osContRamWrite(pfs->queue, pfs->channel, blockno, buffer, FALSE);
        }
        if (ret != 0) {
            return ret;
        }
        buffer += BLOCKSIZE;
        cur_block++;
        siz_block--;
    }

    /* The first write marks the file as holding data. */
    if (flag == PFS_WRITE && !(dir.status & DIR_STATUS_OCCUPIED)) {
        dir.status |= DIR_STATUS_OCCUPIED;
        pfs->activebank = 0;
        ERRCK(__osPfsSelectBank(pfs));
        ERRCK(__osContRamWrite(
            pfs->queue, pfs->channel, pfs->dir_table + file_no, (u8 *)&dir, FALSE));
    }
    return 0;
}
