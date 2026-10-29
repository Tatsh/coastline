#include <PR/os.h>

/* The retail library discards debugging output. */

#if !TGR_BOSS_RALLY
void rmonPrintf(const char *fmt, ...) {
}
#endif

void osSyncPrintf(const char *fmt, ...) {
}
