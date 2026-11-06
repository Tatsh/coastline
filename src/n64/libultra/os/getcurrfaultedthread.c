#include "common/libultra/os/osint.h"

OSThread *__osGetCurrFaultedThread(void) {
    return __osFaultedThread;
}
