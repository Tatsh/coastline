#ifndef LIBULTRA_LIBC_XSTDIO_H
#define LIBULTRA_LIBC_XSTDIO_H

/**
 * @file
 * The formatter behind sprintf() and the state it passes to the conversion routines.
 */

#include <stdarg.h>

#include <ultra64.h>

/* Conversion flags. */
#define FLAGS_SPACE 0x01 /*!< The space flag: a space before a positive number. */
#define FLAGS_PLUS 0x02  /*!< The plus flag: a sign before every number. */
#define FLAGS_MINUS 0x04 /*!< The minus flag: the field is aligned left. */
#define FLAGS_HASH 0x08  /*!< The hash flag: the alternative form of the conversion. */
#define FLAGS_ZERO 0x10  /*!< The zero flag: the field is padded with zeros. */

/* The indices of the halfwords of a double in memory, most significant first. */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _D0 3
#define _D1 2
#define _D2 1
#define _D3 0
#else
#define _D0 0
#define _D1 1
#define _D2 2
#define _D3 3
#endif

/** A routine that appends count characters to arg and returns the new position, or NULL. */
typedef void *(*outfun)(void *arg, const char *s, size_t count);

typedef struct {
    union {
        long long ll;   /*!< The value of an integer conversion. */
        double ld;      /*!< The value of a floating-point conversion. */
        u16 hw[4];      /*!< The halfwords of the floating-point value, most significant first. */
    } v;                /*!< The argument of the conversion. */
    char *s;            /*!< The converted characters. */
    int n0;             /*!< The number of prefix characters. */
    int nz0;            /*!< The number of zeroes after the prefix. */
    int n1;             /*!< The number of characters of the first run from s. */
    int nz1;            /*!< The number of zeroes after the first run. */
    int n2;             /*!< The number of characters of the second run from s. */
    int nz2;            /*!< The number of zeroes after the second run. */
    int prec;           /*!< The precision, or -1 when the format gives none. */
    int width;          /*!< The minimum field width. */
    size_t nchar;       /*!< The number of characters written so far. */
    unsigned int flags; /*!< The FLAGS_* bits of the conversion. */
    char qual;          /*!< The size qualifier, 'h', 'l', 'L', or 0. */
} _Pft;

/** @ghidraAddress USA: 0x80268dc0 */
int _Printf(outfun prout, char *arg, const char *fmt, va_list args);

/** @ghidraAddress USA: 0x8026ce50 */
void _Litob(_Pft *px, char code);

/** @ghidraAddress USA: 0x8026d660 */
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
