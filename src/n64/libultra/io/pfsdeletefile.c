#include "common/libultra/io/controller.h"

static s32 __osPfsReleasePages(OSPfs *pfs,
                               __OSInode *inode,
                               u8 start_page,
                               u16 *sum,
                               u8 bank,
                               __OSInodeUnit *last_page,
                               int flag);
static s32 __osBlockSum(OSPfs *pfs, u8 page_no, u16 *sum, u8 bank);

s32 osPfsDeleteFile(OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name) {
    s32 file_no;
    int k;
    s32 ret;
    __OSInode inode;
    __OSDir dir;
    u16 sum = 0;
    __OSInodeUnit last_page;
    u8 startpage;
    u8 bank;

    if (company_code == 0 || game_code == 0) {
        return PFS_ERR_INVALID;
    }
    if (!(pfs->status & PFS_INITIALIZED)) {
        return PFS_ERR_INVALID;
    }
    if (__osCheckId(pfs) == PFS_ERR_NEW_PACK) {
        return PFS_ERR_NEW_PACK;
    }

    SET_ACTIVEBANK_TO_ZERO;
    ERRCK(osPfsFindFile(pfs, company_code, game_code, game_name, ext_name, &file_no));
    if (file_no == -1) {
        return PFS_ERR_INVALID;
    }
    ERRCK(__osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + file_no, (u8 *)&dir));

    startpage = dir.start_page.inode_t.page;
    for (bank = dir.start_page.inode_t.bank; bank < pfs->banks;) {
        ERRCK(__osPfsRWInode(pfs, &inode, PFS_READ, bank));
        ERRCK(__osPfsReleasePages(pfs, &inode, startpage, &sum, bank, &last_page, TRUE));
        ERRCK(__osPfsRWInode(pfs, &inode, PFS_WRITE, bank));
        if (last_page.ipage == PFS_EOF) {
            break;
        }
        bank = last_page.inode_t.bank;
        startpage = last_page.inode_t.page;
    }
    if (bank >= pfs->banks) {
        return PFS_ERR_INCONSISTENT;
    }

    dir.game_code = 0;
    dir.company_code = 0;
    dir.start_page.ipage = 0;
    dir.data_sum = 0;
    for (k = 0; k < (int)sizeof(dir.game_name); k++) {
        dir.game_name[k] = 0;
    }
    for (k = 0; k < (int)sizeof(dir.ext_name); k++) {
        dir.ext_name[k] = 0;
    }
    dir.status = 0;
    ret = __osContRamWrite(pfs->queue, pfs->channel, pfs->dir_table + file_no, (u8 *)&dir, FALSE);
    return ret;
}

/* Free the pages of a chain within one bank, and report where the chain continues.
 * @ghidraAddress USA: 0x80267f00
 * @ghidraAddress EUR: 0x802696f0
 * @ghidraAddress JPN: 0x802691a0
 */
static s32 __osPfsReleasePages(OSPfs *pfs,
                               __OSInode *inode,
                               u8 start_page,
                               u16 *sum,
                               u8 bank,
                               __OSInodeUnit *last_page,
                               int flag) {
    __OSInodeUnit next_page;
    __OSInodeUnit old_page;
    s32 ret = 0;
    int offset;

    next_page = inode->inode_page[start_page];
    if (next_page.ipage != PFS_EOF) {
        offset = (next_page.inode_t.bank > 0) ? 1 : pfs->inode_start_page;
    } else {
        offset = (bank > 0) ? 1 : pfs->inode_start_page;
    }
    if (next_page.inode_t.page < offset && next_page.ipage != PFS_EOF) {
        return PFS_ERR_INCONSISTENT;
    }

    *last_page = next_page;
    if (flag == TRUE) {
        inode->inode_page[start_page].ipage = PFS_PAGE_NOT_USED;
    }
    ERRCK(__osBlockSum(pfs, start_page, sum, bank));
    if (next_page.ipage == PFS_EOF) {
        return 0;
    }

    while (next_page.ipage >= pfs->inode_start_page) {
        old_page = next_page;
        next_page = inode->inode_page[next_page.inode_t.page];
        inode->inode_page[old_page.inode_t.page].ipage = PFS_PAGE_NOT_USED;
        ERRCK(__osBlockSum(pfs, old_page.inode_t.page, sum, bank));
        if (next_page.inode_t.bank != bank) {
            break;
        }
    }
    /* When the chain moves to another bank, the original also frees the page of the same number
     * in this bank. */
    if (next_page.ipage >= pfs->inode_start_page) {
        inode->inode_page[next_page.inode_t.page].ipage = PFS_PAGE_NOT_USED;
    }
    *last_page = next_page;
    return 0;
}

/* Add the byte sums of the 8 blocks of a page to *sum, then select bank 0 again.
 * @ghidraAddress USA: 0x80268138
 * @ghidraAddress EUR: 0x80269928
 * @ghidraAddress JPN: 0x802693d8
 */
static s32 __osBlockSum(OSPfs *pfs, u8 page_no, u16 *sum, u8 bank) {
    int i;
    s32 ret = 0;
    u8 buf[BLOCKSIZE];

    pfs->activebank = bank;
    ERRCK(__osPfsSelectBank(pfs));
    for (i = 0; i < PFS_ONE_PAGE; i++) {
        ret = __osContRamRead(pfs->queue, pfs->channel, page_no * PFS_ONE_PAGE + i, buf);
        if (ret != 0) {
            pfs->activebank = 0;
            __osPfsSelectBank(pfs);
            return ret;
        }
        *sum = (u16)(*sum + __osSumcalc(buf, BLOCKSIZE));
    }
    pfs->activebank = 0;
    ret = __osPfsSelectBank(pfs);
    return ret;
}
