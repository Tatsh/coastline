#include "controller.h"

s32 osPfsFreeBlocks(OSPfs *pfs, s32 *bytes_not_used) {
    int j;
    int pages = 0;
    __OSInode inode;
    s32 ret = 0;
    u8 bank;
    int offset;

    if (!(pfs->status & PFS_INITIALIZED)) {
        return PFS_ERR_INVALID;
    }
    if (__osCheckId(pfs) == PFS_ERR_NEW_PACK) {
        return PFS_ERR_NEW_PACK;
    }

    for (bank = 0; bank < pfs->banks; bank++) {
        ERRCK(__osPfsRWInode(pfs, &inode, PFS_READ, bank));
        offset = (bank > 0) ? 1 : pfs->inode_start_page;
        for (j = offset; j < PFS_INODE_SIZE_PER_PAGE; j++) {
            if (inode.inode_page[j].ipage == PFS_PAGE_NOT_USED) {
                pages++;
            }
        }
    }
    *bytes_not_used = pages * PFS_ONE_PAGE * BLOCKSIZE;
    return 0;
}
