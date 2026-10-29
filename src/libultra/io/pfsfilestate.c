#include "common/libultra/io/controller.h"

/* The original reads the page unset when the first bank of the chain is out of range. */
s32 osPfsFileState(OSPfs *pfs, s32 file_no, OSPfsState *state) {
    s32 ret;
    int pages;
    __OSInode inode;
    __OSDir dir;
#ifdef ENABLE_PATCHES
    /* A first bank out of range leaves page 0 of bank 0, and the chain is inconsistent. */
    __OSInodeUnit page = {{0, 0}};
#else
    __OSInodeUnit page;
#endif /* ENABLE_PATCHES */
    int j;
    u8 bank;
    u8 start;

    if (file_no >= pfs->dir_size || file_no < 0) {
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
    if (dir.company_code == 0 || dir.game_code == 0) {
        return PFS_ERR_INVALID;
    }
    if (dir.start_page.ipage < pfs->inode_start_page) {
        return PFS_ERR_INCONSISTENT;
    }

    /* Count the pages of the chain, reading the page table of each bank it passes through. */
    pages = 0;
    start = dir.start_page.inode_t.page;
    bank = dir.start_page.inode_t.bank;
    while (bank < pfs->banks) {
        ERRCK(__osPfsRWInode(pfs, &inode, PFS_READ, bank));
        page = inode.inode_page[start];
        pages++;
        while (page.ipage >= pfs->inode_start_page) {
            pages++;
            page = inode.inode_page[page.inode_t.page];
            if (page.inode_t.bank != bank) {
                bank = page.inode_t.bank;
                start = page.inode_t.page;
                break;
            }
        }
        if (page.ipage == PFS_EOF) {
            break;
        }
    }
    if (page.ipage != PFS_EOF) {
        return PFS_ERR_INCONSISTENT;
    }

    state->file_size = pages * (PFS_ONE_PAGE * BLOCKSIZE);
    state->company_code = dir.company_code;
    state->game_code = dir.game_code;
    for (j = 0; j < (int)sizeof(dir.game_name); j++) {
        state->game_name[j] = dir.game_name[j];
    }
    for (j = 0; j < (int)sizeof(dir.ext_name); j++) {
        state->ext_name[j] = dir.ext_name[j];
    }
    return 0;
}
