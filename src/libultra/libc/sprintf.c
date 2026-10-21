#include "xstdio.h"

/** @ghidraAddress USA: 0x80260db0 */
static void *proutSprintf(void *dst, const char *src, size_t count) {
    return (char *)memcpy(dst, src, count) + count;
}

int sprintf(char *dst, const char *fmt, ...) {
    int ans;
    va_list ap;

    va_start(ap, fmt);
    ans = _Printf(proutSprintf, dst, fmt, ap);
    va_end(ap);
    if (ans >= 0) {
        dst[ans] = '\0';
    }
    return ans;
}
