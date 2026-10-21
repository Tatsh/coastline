#include <ultra64.h>

#include "../os/osint.h"
#include "piint.h"

OSPiHandle CartRomHandle;

OSPiHandle *osCartRomInit(void) {
    u32 domain = 0;
    u32 saveMask;

    if (CartRomHandle.baseAddress == PHYS_TO_K1(PI_DOM1_ADDR2)) {
        return &CartRomHandle;
    }
    CartRomHandle.type = DEVICE_TYPE_CART;
    CartRomHandle.baseAddress = PHYS_TO_K1(PI_DOM1_ADDR2);
    /* The first word of the cartridge records its timing. */
    osPiRawReadIo(0, &domain);
    CartRomHandle.latency = domain & 0xFF;
    CartRomHandle.pulse = (domain >> 8) & 0xFF;
    CartRomHandle.pageSize = (domain >> 16) & 0x0F;
    CartRomHandle.relDuration = (domain >> 20) & 0x0F;
    CartRomHandle.domain = PI_DOMAIN1;
    bzero(&CartRomHandle.transferInfo, sizeof(CartRomHandle.transferInfo));
    saveMask = __osDisableInt();
    CartRomHandle.next = __osPiTable;
    __osPiTable = &CartRomHandle;
    __osRestoreInt(saveMask);
    return &CartRomHandle;
}
