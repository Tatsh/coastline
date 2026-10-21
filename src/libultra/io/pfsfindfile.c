#include "controller.h"

s32 osPfsFindFile(
    OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name, s32 *file_no) {
    int j;
    int i;
    __OSDir dir;
    s32 ret = 0;
    int fail;

    if (__osCheckId(pfs) == PFS_ERR_NEW_PACK) {
        return PFS_ERR_NEW_PACK;
    }

    for (j = 0; j < pfs->dir_size; j++) {
        ERRCK(__osContRamRead(pfs->queue, pfs->channel, pfs->dir_table + j, (u8 *)&dir));
        if (dir.company_code == company_code && dir.game_code == game_code) {
            fail = 0;
            if (game_name != NULL) {
                for (i = 0; i < (int)sizeof(dir.game_name); i++) {
                    if (game_name[i] != dir.game_name[i]) {
                        fail = 1;
                        break;
                    }
                }
            }
            if (ext_name != NULL && fail == 0) {
                for (i = 0; i < (int)sizeof(dir.ext_name); i++) {
                    if (ext_name[i] != dir.ext_name[i]) {
                        fail = 1;
                        break;
                    }
                }
            }
            if (fail == 0) {
                *file_no = j;
                return ret;
            }
        }
    }
    *file_no = -1;
    return PFS_ERR_INVALID;
}
