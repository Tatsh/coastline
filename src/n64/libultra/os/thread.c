#include "common/libultra/os/osint.h"

void __osDequeueThread(OSThread **queue, OSThread *t) {
    /* The queue head stands in for a thread whose next field is the first in the queue. */
    register OSThread *pred = (OSThread *)queue;
    register OSThread *succ = pred->next;

    while (succ != NULL) {
        if (succ == t) {
            pred->next = t->next;
            return;
        }
        pred = succ;
        succ = pred->next;
    }
}
