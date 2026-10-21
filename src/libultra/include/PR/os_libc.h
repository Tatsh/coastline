#ifndef PR_OS_LIBC_H
#define PR_OS_LIBC_H

/**
 * @file
 * The memory and formatting routines libultra provides in place of a C library.
 */

#include <PR/ultratypes.h>

/**
 * Copy n bytes between buffers that may overlap.
 *
 * @ghidraAddress USA: 0x802674d0
 */
void bcopy(const void *src, void *dst, int n);

/**
 * Compare n bytes of two buffers.
 *
 * @return 0 when they are equal, and nonzero otherwise.
 * @ghidraAddress USA: 0x80262540
 */
int bcmp(const void *a, const void *b, int n);

/**
 * Clear n bytes.
 *
 * @ghidraAddress USA: 0x8026bba0
 */
void bzero(void *p, int n);

#endif
