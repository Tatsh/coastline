#ifndef PR_OS_LIBC_H
#define PR_OS_LIBC_H

/**
 * @file
 * The memory and formatting routines libultra provides in place of a C library.
 */

#include <stddef.h>

#include <PR/ultratypes.h>

/** The quotient and remainder of ldiv(). */
typedef struct {
    long quot;
    long rem;
} ldiv_t;

/** The quotient and remainder of lldiv(). */
typedef struct {
    long long quot;
    long long rem;
} lldiv_t;

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

/**
 * Copy n bytes between buffers that do not overlap.
 *
 * @return dst.
 * @ghidraAddress USA: 0x80260b20
 */
void *memcpy(void *dst, const void *src, size_t n);

/**
 * Return the length of a string.
 *
 * @ghidraAddress USA: 0x80260b4c
 */
size_t strlen(const char *s);

/**
 * Find the first occurrence of a character in a string.
 *
 * @return The occurrence, or NULL when the string does not contain the character.
 * @ghidraAddress USA: 0x80260b74
 */
const char *strchr(const char *s, int c);

/**
 * Format into a buffer.
 *
 * @return The number of characters written, not counting the terminating NUL.
 * @ghidraAddress USA: 0x80260dd4
 */
int sprintf(char *dst, const char *fmt, ...);

/**
 * Divide, truncating towards zero.
 *
 * @ghidraAddress USA: 0x8026e970
 */
ldiv_t ldiv(long num, long denom);

/**
 * Divide 64-bit values, truncating towards zero.
 *
 * @ghidraAddress USA: 0x8026e870
 */
lldiv_t lldiv(long long num, long long denom);

#endif
