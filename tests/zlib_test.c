/* Host check of the vendored zlib with the game's changes. Inflate every asset the extractor
 * wrote and compare with the extractor's output, which the host zlib produced. The game's tables
 * are defined here with the values of the original data. Run it from the repository root with the
 * asset directory as the argument. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>

#include "inftrees.h"
#include "system.h"
#include "zutil.h"

uInt border[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
char inflate_copyright[] = " inflate 1.0.4 Copyright 1995-1996 Mark Adler ";
uInt cplens[31] = {3,  4,  5,  6,  7,  8,  9,  10,  11,  13,  15,  17,  19,  23, 27, 31,
                   35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0,  0};
uInt cplext[31] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2,   2,  2,
                   3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 192, 192};
uInt cpdist[30] = {1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
                   33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
                   1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
uInt cpdext[30] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                   6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
int fixed_built;
inflate_huft fixed_mem[FIXEDH];
uInt fixed_bl, fixed_bd;
inflate_huft *fixed_tl, *fixed_td;
uInt inflate_mask[17] = {0x0000,
                         0x0001,
                         0x0003,
                         0x0007,
                         0x000f,
                         0x001f,
                         0x003f,
                         0x007f,
                         0x00ff,
                         0x01ff,
                         0x03ff,
                         0x07ff,
                         0x0fff,
                         0x1fff,
                         0x3fff,
                         0x7fff,
                         0xffff};
const char *z_errmsg[10];

u16 g_aFramebuffer0[320 * 240], g_aFramebuffer1[320 * 240];
u8 *g_pInflateHeapBase, *g_pInflateHeapEnd;
InflateAllocator g_inflateAllocator;
u32 g_dwScreenUnblankDelay;
void osViBlack(u8 active) {
}
void osSyncPrintf(const char *fmt, ...) {
    puts("out of memory");
    exit(2);
}

static u8 heap[100000];

static unsigned long be32(const u8 *p) {
    return (unsigned long)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static u8 *slurp(const char *path, long *size) {
    FILE *f = fopen(path, "rb");
    u8 *data;
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    rewind(f);
    data = malloc(*size + 1);
    if (fread(data, 1, *size, f) != (size_t)*size) {
        *size = -1;
    }
    fclose(f);
    return data;
}

int main(int argc, char **argv) {
    DIR *dir = opendir(argv[1]);
    struct dirent *e;
    int checked = 0, failed = 0;
    char path[4096];

    while ((e = readdir(dir)) != NULL) {
        long zsize, bsize;
        u8 *z, *b, *out;
        unsigned long total, size, pos, used = 0;
        size_t n = strlen(e->d_name);

        if (n < 3 || strcmp(e->d_name + n - 2, ".z") != 0) {
            continue;
        }
        snprintf(path, sizeof path, "%s/%s", argv[1], e->d_name);
        z = slurp(path, &zsize);
        snprintf(path, sizeof path, "%s/%.*s.bin", argv[1], (int)(n - 2), e->d_name);
        b = slurp(path, &bsize);
        total = be32(z);
        size = be32(z + 4);
        out = malloc(size + 1);
        for (pos = 8; pos < total;) {
            unsigned long len = be32(z + pos);
            z_stream s;
            int r;

            memset(&s, 0, sizeof s);
            /* The game lends a heap for each chunk, as DecompressFromRom() does. */
            g_pInflateHeapBase = heap;
            g_pInflateHeapEnd = heap + sizeof heap;
            if (inflateInit(&s) != Z_OK) {
                break;
            }
            s.next_in = z + pos + 4;
            s.avail_in = len;
            s.next_out = out + used;
            s.avail_out = size - used;
            r = inflate(&s, Z_FINISH);
            used += s.total_out;
            inflateEnd(&s);
            if (r != Z_STREAM_END) {
                printf("%s: chunk at %lu returned %d\n", e->d_name, pos, r);
                failed++;
                break;
            }
            pos += 4 + len + (len & 1);
        }
        if (used != size || (long)size != bsize || memcmp(out, b, size) != 0) {
            printf("%s: mismatch, %lu of %lu bytes\n", e->d_name, used, size);
            failed++;
        }
        checked++;
        free(z);
        free(b);
        free(out);
    }
    closedir(dir);
    printf("%d assets inflated, %d failed\n", checked, failed);
    /* A directory without assets checks nothing, and fails. */
    return failed != 0 || checked == 0;
}
