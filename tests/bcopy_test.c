/* Host check of bcopy and bzero over overlapping ranges, alignments and lengths. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
void lu_bcopy(const void *src, void *dst, int n);
void lu_bzero(void *p, int n);
int lu_bcmp(const void *a, const void *b, int n);
int main(void) {
    unsigned char buf[64], ref[64];
    for (int so = 0; so < 8; so++) {
        for (int doff = 0; doff < 8; doff++) {
            for (int n = 0; n < 40; n++) {
                for (int i = 0; i < 64; i++) {
                    buf[i] = ref[i] = (unsigned char)(i * 7 + 1);
                }
                lu_bcopy(buf + so, buf + doff + 8, n);
                memmove(ref + doff + 8, ref + so, n);
                assert(!memcmp(buf, ref, 64));
                lu_bcopy(buf + doff + 8, buf + so, n);
                memmove(ref + so, ref + doff + 8, n);
                assert(!memcmp(buf, ref, 64));
                lu_bzero(buf + so, n);
                memset(ref + so, 0, n);
                assert(!memcmp(buf, ref, 64));
                assert(lu_bcmp(buf, ref, 64) == 0);
            }
        }
    }
    buf[3] ^= 1;
    assert(lu_bcmp(buf, ref, 64) == 1);
    puts("bcopy check passed");
    return 0;
}
