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
    long quot; /*!< The quotient. */
    long rem;  /*!< The remainder, the dividend less the divisor times quot. */
} ldiv_t;

/** The quotient and remainder of lldiv(). */
typedef struct {
    long long quot; /*!< The quotient. */
    long long rem;  /*!< The remainder, the dividend less the divisor times quot. */
} lldiv_t;

/**
 * Copy n bytes between buffers that may overlap.
 *
 * @param src The buffer to copy from.
 * @param dst The buffer to copy to.
 * @param n The count of bytes.
 * @return dst.
 *
 * @ghidraAddress USA: 0x802674d0
 * @ghidraAddress JPN: 0x80268770
 * @ghidraAddress EUR: 0x80268cc0
 */
void *bcopy(const void *src, void *dst, int n);

/**
 * Compare n bytes of two buffers.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param n The count of bytes to compare.
 * @return 0 when they are equal, and nonzero otherwise.
 * @ghidraAddress USA: 0x80262540
 * @ghidraAddress JPN: 0x80263760
 * @ghidraAddress EUR: 0x80263cc0
 */
int bcmp(const void *a, const void *b, int n);

/**
 * Clear n bytes.
 *
 * @param p The buffer to clear.
 * @param n The count of bytes to clear.
 * @ghidraAddress USA: 0x8026bba0
 * @ghidraAddress JPN: 0x8026d620
 * @ghidraAddress EUR: 0x8026db70
 */
void bzero(void *p, int n);

/**
 * Copy n bytes between buffers that do not overlap.
 *
 * @param dst The buffer to copy to.
 * @param src The buffer to copy from.
 * @param n The count of bytes.
 * @return dst.
 * @ghidraAddress USA: 0x80260b20
 * @ghidraAddress JPN: 0x80261a60
 * @ghidraAddress EUR: 0x80261fb0
 */
void *memcpy(void *dst, const void *src, size_t n);

/**
 * Return the length of a string.
 *
 * @param s The NUL-terminated string.
 * @return The count of characters before the terminating NUL.
 * @ghidraAddress USA: 0x80260b4c
 * @ghidraAddress JPN: 0x80261a8c
 * @ghidraAddress EUR: 0x80261fdc
 */
size_t strlen(const char *s);

/**
 * Find the first occurrence of a character in a string.
 *
 * @param s The NUL-terminated string.
 * @param c The character to find, converted to char.
 * @return The occurrence, or NULL when the character is absent from the string.
 * @ghidraAddress USA: 0x80260b74
 * @ghidraAddress JPN: 0x80261ab4
 * @ghidraAddress EUR: 0x80262004
 */
const char *strchr(const char *s, int c);

/**
 * Format into a buffer.
 *
 * @param dst The buffer written, NUL-terminated and large enough for the output.
 * @param fmt The printf format, followed by its arguments.
 * @return The number of characters written, not counting the terminating NUL.
 * @ghidraAddress USA: 0x80260dd4
 * @ghidraAddress JPN: 0x80261d14
 * @ghidraAddress EUR: 0x80262264
 */
int sprintf(char *dst, const char *fmt, ...);

/**
 * Divide, truncating towards zero.
 *
 * @param num The dividend.
 * @param denom The divisor, not 0.
 * @return The quotient and the remainder, the remainder with the sign of the dividend.
 * @ghidraAddress USA: 0x8026e970
 * @ghidraAddress JPN: 0x802703f0
 * @ghidraAddress EUR: 0x80270940
 */
ldiv_t ldiv(long num, long denom);

/**
 * Divide 64-bit values, truncating towards zero.
 *
 * @param num The dividend.
 * @param denom The divisor, not 0.
 * @return The quotient and the remainder, the remainder with the sign of the dividend.
 * @ghidraAddress USA: 0x8026e870
 * @ghidraAddress JPN: 0x802702f0
 * @ghidraAddress EUR: 0x80270840
 */
lldiv_t lldiv(long long num, long long denom);

#endif
