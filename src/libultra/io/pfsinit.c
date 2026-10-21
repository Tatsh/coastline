#include "controller.h"
#include "siint.h"

s32 osPfsInit(OSMesgQueue *queue, OSPfs *pfs, int channel) {
    s32 ret = 0;

    __osSiGetAccess();
    ret = __osPfsGetStatus(queue, channel);
    __osSiRelAccess();
    if (ret != 0) {
        return ret;
    }

    pfs->queue = queue;
    pfs->channel = channel;
    pfs->status = 0;
    /* No bank is known to be active, so the first access selects bank 0. */
    pfs->activebank = 0xFF;
    ERRCK(__osGetId(pfs));

    ret = osPfsChecker(pfs);
    pfs->status |= PFS_INITIALIZED;
    return ret;
}
