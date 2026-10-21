#include "controller.h"

/* The map entry of a page is its group of four pages within the bank's 32 groups, and the bank
 * picks one of 8 rows. */
#define CACHE_INDEX(unit) ((unit).inode_t.page / 4 + ((unit).inode_t.bank % 8) * BLOCKSIZE)

/* Record which banks point into the pages of which other banks. */
static s32 corrupted_init(OSPfs *pfs, __OSInodeCache *cache) {
    int i;
    int n;
    int offset;
    u8 bank;
    __OSInodeUnit tpage;
    __OSInode tmp_inode;
    s32 ret;

    for (i = 0; i < 256; i++) {
        cache->map[i] = 0;
    }
    cache->bank = -1;

    for (bank = 0; bank < pfs->banks; bank++) {
        offset = (bank > 0) ? 1 : pfs->inode_start_page;
        ret = __osPfsRWInode(pfs, &tmp_inode, PFS_READ, bank);
        if (ret != 0 && ret != PFS_ERR_INCONSISTENT) {
            return ret;
        }
        for (i = offset; i < PFS_INODE_SIZE_PER_PAGE; i++) {
            tpage = tmp_inode.inode_page[i];
            if (tpage.ipage >= pfs->inode_start_page && tpage.inode_t.bank != bank) {
                n = CACHE_INDEX(tpage);
                cache->map[n] |= 1 << (bank % 8);
            }
        }
    }
    return 0;
}

/* Return how many page table entries point at fpage, or PFS_ERR_NEW_PACK once two do. */
static s32 corrupted(OSPfs *pfs, __OSInodeUnit fpage, __OSInodeCache *cache) {
    int j;
    int n;
    int hit = 0;
    u8 bank;
    int offset;
    s32 ret = 0;

    n = CACHE_INDEX(fpage);
    for (bank = 0; bank < pfs->banks; bank++) {
        offset = (bank > 0) ? 1 : pfs->inode_start_page;
        if (bank == fpage.inode_t.bank || (cache->map[n] & (1 << (bank % 8)))) {
            if (bank != cache->bank) {
                ret = __osPfsRWInode(pfs, &cache->inode, PFS_READ, bank);
                if (ret != 0 && ret != PFS_ERR_INCONSISTENT) {
                    return ret;
                }
                cache->bank = bank;
            }
            for (j = offset; hit < 2 && j < PFS_INODE_SIZE_PER_PAGE; j++) {
                if (cache->inode.inode_page[j].ipage == fpage.ipage) {
                    hit++;
                }
            }
            if (hit >= 2) {
                return PFS_ERR_NEW_PACK;
            }
        }
    }
    return hit;
}

s32 osPfsChecker(OSPfs *pfs) {
    int j;
    s32 ret;
    __OSInodeUnit next_node;
    __OSInode checked_inode;
    __OSInode tmp_inode;
    __OSDir tmp_dir;
    __OSInodeUnit file_next_node[DEF_DIR_PAGES * PFS_ONE_PAGE];
    __OSInodeCache cache;
    int fixed = 0;
    u8 bank;
    int offset;
    int cc, cl;

    ret = __osCheckId(pfs);
    if (ret == PFS_ERR_NEW_PACK) {
        ret = __osGetId(pfs);
    }
    if (ret != 0) {
        return ret;
    }
    ERRCK(corrupted_init(pfs, &cache));

    /* Clear every directory entry whose page chain is broken or shared. */
    for (j = 0; j < pfs->dir_size; j++) {
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + j, (u8 *)&tmp_dir));
        if (tmp_dir.company_code != 0 && tmp_dir.game_code != 0) {
            next_node.ipage = tmp_dir.start_page.ipage;
            cc = 0;
            cl = 0;
            bank = 0xFF;
            while (next_node.ipage >= pfs->inode_start_page &&
                   next_node.inode_t.bank < pfs->banks && next_node.inode_t.page > 0 &&
                   next_node.inode_t.page < PFS_INODE_SIZE_PER_PAGE) {
                if (bank != next_node.inode_t.bank) {
                    bank = next_node.inode_t.bank;
                    ret = __osPfsRWInode(pfs, &tmp_inode, PFS_READ, bank);
                    if (ret != 0 && ret != PFS_ERR_INCONSISTENT) {
                        return ret;
                    }
                }
                /* The first page has one reference from the directory as well. */
                cc = corrupted(pfs, next_node, &cache) - cl;
                if (cc != 0) {
                    break;
                }
                cl = 1;
                next_node.ipage = tmp_inode.inode_page[next_node.inode_t.page].ipage;
            }
            if (cc == 0 && next_node.ipage == PFS_EOF) {
                continue;
            }
        } else if (tmp_dir.company_code == 0 && tmp_dir.game_code == 0) {
            continue;
        }
        tmp_dir.company_code = 0;
        tmp_dir.game_code = 0;
        tmp_dir.start_page.ipage = 0;
        tmp_dir.status = 0;
        tmp_dir.data_sum = 0;
        SET_ACTIVEBANK_TO_ZERO;
        ERRCK(
            __osContRamWrite(pfs->queue, pfs->channel, pfs->dir_table + j, (u8 *)&tmp_dir, FALSE));
        fixed++;
    }

    for (j = 0; j < pfs->dir_size; j++) {
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + j, (u8 *)&tmp_dir));
        if (tmp_dir.company_code != 0 && tmp_dir.game_code != 0 &&
            tmp_dir.start_page.ipage >= (u16)pfs->inode_start_page) {
            file_next_node[j].ipage = tmp_dir.start_page.ipage;
        } else {
            file_next_node[j].ipage = 0;
        }
    }

    /* Rebuild each page table from the chains of the surviving files. Pages no chain reaches
     * become free. */
    for (bank = 0; bank < pfs->banks; bank++) {
        ret = __osPfsRWInode(pfs, &tmp_inode, PFS_READ, bank);
        if (ret != 0 && ret != PFS_ERR_INCONSISTENT) {
            return ret;
        }
        offset = (bank > 0) ? 1 : pfs->inode_start_page;
        for (j = 0; j < offset; j++) {
            checked_inode.inode_page[j].ipage = tmp_inode.inode_page[j].ipage;
        }
        for (; j < PFS_INODE_SIZE_PER_PAGE; j++) {
            checked_inode.inode_page[j].ipage = PFS_PAGE_NOT_USED;
        }
        for (j = 0; j < pfs->dir_size; j++) {
            while (file_next_node[j].inode_t.bank == bank &&
                   file_next_node[j].ipage >= (u16)pfs->inode_start_page) {
                u8 pp = file_next_node[j].inode_t.page;
                __OSInodeUnit unit = tmp_inode.inode_page[pp];

                checked_inode.inode_page[pp] = unit;
                file_next_node[j] = unit;
            }
        }
        ERRCK(__osPfsRWInode(pfs, &checked_inode, PFS_WRITE, bank));
    }

    if (fixed) {
        pfs->status |= PFS_CORRUPTED;
    } else {
        pfs->status &= ~PFS_CORRUPTED;
    }
    return 0;
}
