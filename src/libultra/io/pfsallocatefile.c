#include "controller.h"

static s32 __osPfsDeclearPage(OSPfs *pfs,
                              __OSInode *inode,
                              int file_size_in_pages,
                              int *first_page,
                              u8 bank,
                              int *decleared,
                              int *last_page);
static s32 __osClearPage(OSPfs *pfs, int page_no, u8 *data, u8 bank);

/* The original reads the first page uninitialised when no bank is searched. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
s32 osPfsAllocateFile(OSPfs *pfs,
                      u16 company_code,
                      u32 game_code,
                      u8 *game_name,
                      u8 *ext_name,
                      int file_size_in_bytes,
                      s32 *file_no) {
    int start_page;
    int decleared;
    int last_page;
    int old_last_page = 0;
    int j;
    s32 ret = 0;
    int file_size_in_pages;
    __OSInode inode;
    __OSInode backup_inode;
    __OSDir dir;
    u8 bank;
    u8 old_bank = 0;
    int firsttime = 0;
    s32 bytes;
    __OSInodeUnit fpage;

    if (company_code == 0 || game_code == 0) {
        return PFS_ERR_INVALID;
    }
    file_size_in_pages =
        (file_size_in_bytes + PFS_ONE_PAGE * BLOCKSIZE - 1) / (PFS_ONE_PAGE * BLOCKSIZE);
    if (!(pfs->status & PFS_INITIALIZED)) {
        return PFS_ERR_INVALID;
    }
    if (__osCheckId(pfs) == PFS_ERR_NEW_PACK) {
        return PFS_ERR_NEW_PACK;
    }

    ret = osPfsFindFile(pfs, company_code, game_code, game_name, ext_name, file_no);
    if (ret != 0 && ret != PFS_ERR_INVALID) {
        return ret;
    }
    if (*file_no != -1) {
        return PFS_ERR_EXIST;
    }
    ret = osPfsFreeBlocks(pfs, &bytes);
    if (file_size_in_bytes > bytes) {
        return PFS_DATA_FULL;
    }

    if (file_size_in_pages != 0) {
        /* A free directory entry has zero codes. */
        ret = osPfsFindFile(pfs, 0, 0, NULL, NULL, file_no);
        if (ret != 0 && ret != PFS_ERR_INVALID) {
            return ret;
        }
        if (*file_no == -1) {
            return PFS_DIR_FULL;
        }

        for (bank = 0; bank < pfs->banks; bank++) {
            ERRCK(__osPfsRWInode(pfs, &inode, PFS_READ, bank));
            ERRCK(__osPfsDeclearPage(
                pfs, &inode, file_size_in_pages, &start_page, bank, &decleared, &last_page));
            if (start_page != -1) {
                if (firsttime == 0) {
                    fpage.inode_t.page = start_page;
                    fpage.inode_t.bank = bank;
                } else {
                    /* Link the end of the chain in the previous bank to this one. */
                    backup_inode.inode_page[old_last_page].inode_t.bank = bank;
                    backup_inode.inode_page[old_last_page].inode_t.page = start_page;
                    ERRCK(__osPfsRWInode(pfs, &backup_inode, PFS_WRITE, old_bank));
                }
                for (j = 0; j < PFS_INODE_SIZE_PER_PAGE; j++) {
                    backup_inode.inode_page[j].ipage = inode.inode_page[j].ipage;
                }
                old_last_page = last_page;
                old_bank = bank;
                firsttime++;
                if (file_size_in_pages > decleared) {
                    file_size_in_pages -= decleared;
                } else {
                    file_size_in_pages = 0;
                    break;
                }
            }
        }
        if (file_size_in_pages > 0 || start_page == -1) {
            return PFS_ERR_INCONSISTENT;
        }

        backup_inode.inode_page[old_last_page].inode_t.bank = bank;
        backup_inode.inode_page[old_last_page].inode_t.page = start_page;
        ERRCK(__osPfsRWInode(pfs, &backup_inode, PFS_WRITE, old_bank));

        /* The original leaves the status and reserved bytes of the entry as the stack held them. */
        dir.start_page = fpage;
        dir.company_code = company_code;
        dir.game_code = game_code;
        dir.data_sum = 0;
        for (j = 0; j < (int)sizeof(dir.game_name); j++) {
            dir.game_name[j] = *game_name++;
        }
        for (j = 0; j < (int)sizeof(dir.ext_name); j++) {
            dir.ext_name[j] = *ext_name++;
        }
        ERRCK(__osContRamWrite(
            pfs->queue, pfs->channel, pfs->dir_table + *file_no, (u8 *)&dir, FALSE));
        return ret;
    } else {
        return PFS_ERR_INVALID;
    }
}
#pragma GCC diagnostic pop

/* Claim free pages of one bank for a file, chaining and clearing them.
 * @ghidraAddress USA: 0x80263fb4
 * @ghidraAddress JPN: 0x802651d4
 * @ghidraAddress EUR: 0x80265734
 */
static s32 __osPfsDeclearPage(OSPfs *pfs,
                              __OSInode *inode,
                              int file_size_in_pages,
                              int *first_page,
                              u8 bank,
                              int *decleared,
                              int *last_page) {
    int j;
    int spage, old_page;
    u8 clear_buf[BLOCKSIZE];
    int i;
    s32 ret = 0;
    int offset;

    offset = (bank > 0) ? 1 : pfs->inode_start_page;
    for (j = offset; j < PFS_INODE_SIZE_PER_PAGE; j++) {
        if (inode->inode_page[j].ipage == PFS_PAGE_NOT_USED) {
            break;
        }
    }
    if (j == PFS_INODE_SIZE_PER_PAGE) {
        *first_page = -1;
        return ret;
    }

    for (i = 0; i < BLOCKSIZE; i++) {
        clear_buf[i] = 0;
    }
    spage = j;
    *decleared = 1;
    old_page = j;
    j++;
    while (file_size_in_pages > *decleared && j < PFS_INODE_SIZE_PER_PAGE) {
        if (inode->inode_page[j].ipage == PFS_PAGE_NOT_USED) {
            inode->inode_page[old_page].inode_t.bank = bank;
            inode->inode_page[old_page].inode_t.page = j;
            ERRCK(__osClearPage(pfs, old_page, clear_buf, bank));
            old_page = j;
            (*decleared)++;
        }
        j++;
    }

    *first_page = spage;
    if (j == PFS_INODE_SIZE_PER_PAGE && file_size_in_pages > *decleared) {
        /* The file continues in the next bank, which links from this page. */
        *last_page = old_page;
        return ret;
    } else {
        inode->inode_page[old_page].ipage = PFS_EOF;
        ret = __osClearPage(pfs, old_page, clear_buf, bank);
        *last_page = 0;
        return ret;
    }
}

/* Write a page of zeroes to a page of a bank, then select bank 0 again. The result is that of
 * selecting bank 0, whatever the writes returned.
 * @ghidraAddress USA: 0x80264218
 * @ghidraAddress JPN: 0x80265438
 * @ghidraAddress EUR: 0x80265998
 */
static s32 __osClearPage(OSPfs *pfs, int page_no, u8 *data, u8 bank) {
    int i;
    s32 ret = 0;

    pfs->activebank = bank;
    ERRCK(__osPfsSelectBank(pfs));
    for (i = 0; i < PFS_ONE_PAGE; i++) {
        ret = __osContRamWrite(pfs->queue, pfs->channel, page_no * PFS_ONE_PAGE + i, data, FALSE);
        if (ret != 0) {
            break;
        }
    }
    pfs->activebank = 0;
    ret = __osPfsSelectBank(pfs);
    return ret;
}
