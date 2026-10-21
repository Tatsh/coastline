#ifndef LIBULTRA_LIBC_XSTDIO_H
#define LIBULTRA_LIBC_XSTDIO_H

/**
 * @file
 * The formatter behind sprintf() and the state it passes to the conversion routines.
 */

#include <stdarg.h>

#include <ultra64.h>

/* Conversion flags. */
#define FLAGS_SPACE 0x01
#define FLAGS_PLUS 0x02
#define FLAGS_MINUS 0x04
#define FLAGS_HASH 0x08
#define FLAGS_ZERO 0x10

/** A routine that appends count characters to arg and returns the new position, or NULL. */
typedef void *(*outfun)(void *arg, const char *s, size_t count);

/**
 * One conversion in progress. A field is n0 prefix characters from the caller's buffer, nz0
 * zeroes, n1 characters from s, nz1 zeroes, n2 more characters from s and nz2 zeroes.
 */
typedef struct {
    union {
        long long ll;
        double ld;
        u16 hw[4];
    } v;
    char *s;
    int n0;
    int nz0;
    int n1;
    int nz1;
    int n2;
    int nz2;
    int prec;
    int width;
    size_t nchar;
    unsigned int flags;
    char qual;
} _Pft;

/**
 * Format the arguments, passing each run of output to prout.
 *
 * @return The number of characters written.
 * @ghidraAddress USA: 0x80268dc0
 */
int _Printf(outfun prout, char *arg, const char *fmt, va_list args);

/**
 * Convert the integer in px to digits in the base of the conversion code.
 *
 * @ghidraAddress USA: 0x8026ce50
 */
void _Litob(_Pft *px, char code);

/**
 * Convert the double in px to digits in the form of the conversion code.
 *
 * @ghidraAddress USA: 0x8026d660
 */
void _Ldtob(_Pft *px, char code);

/**
 * Padding for field widths.
 *
 * @ghidraAddress USA: 0x802a6230
 */
extern char spaces[33];

/** @ghidraAddress USA: 0x802a6254 */
extern char zeroes[33];

/**
 * The flag characters, in the order of fbit.
 *
 * @ghidraAddress USA: 0x802ab984
 */
extern const char fchar[6];

/** @ghidraAddress USA: 0x802ab98c */
extern const int fbit[6];

/** @ghidraAddress USA: 0x802a63e0 */
extern char g_szHexDigitsLower[17];

/** @ghidraAddress USA: 0x802a63f4 */
extern char g_szHexDigitsUpper[17];

/**
 * Ten raised to the powers of two from 1 to 256.
 *
 * @ghidraAddress USA: 0x802abb00
 */
extern const double g_adblPowersOfTen[9];

#endif
