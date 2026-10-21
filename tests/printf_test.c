/* Host check of the libultra sprintf against the host snprintf. */
#include <stdio.h>
#include <string.h>
int lu_sprintf(char *dst, const char *fmt, ...);
char spaces[33] = "                                ";
char zeroes[33] = "00000000000000000000000000000000";
const char fchar[6] = " +-#0";
const int fbit[6] = {1, 2, 4, 8, 16, 0};
char g_szHexDigitsLower[17] = "0123456789abcdef";
char g_szHexDigitsUpper[17] = "0123456789ABCDEF";
const double g_adblPowersOfTen[9] = {1e1, 1e2, 1e4, 1e8, 1e16, 1e32, 1e64, 1e128, 1e256};
static int failures;
#define CHECK(fmt, ...)                                                                            \
    do {                                                                                           \
        char a[256], b[256];                                                                       \
        int na = lu_sprintf(a, fmt, __VA_ARGS__), nb = snprintf(b, sizeof b, fmt, __VA_ARGS__);    \
        if (na != nb || strcmp(a, b)) {                                                            \
            printf("FAIL %-12s got [%s] %d want [%s] %d\n", fmt, a, na, b, nb);                    \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)
/* Where the SDK formats differently from the host, the expected text is explicit. */
#define EXPECT(want, fmt, ...)                                                                     \
    do {                                                                                           \
        char a[256];                                                                               \
        lu_sprintf(a, fmt, __VA_ARGS__);                                                           \
        if (strcmp(a, want)) {                                                                     \
            printf("FAIL %-12s got [%s] want [%s]\n", fmt, a, want);                               \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)
int main(void) {
    CHECK("%d|%5d|%-5d|%05d|%+d|% d", 42, -42, 42, 42, 42, 42);
    CHECK("%u %x %X %o %#x %#o", 4000000000u, 0xbeef, 0xbeef, 8, 255, 8);
    CHECK("%lld %llx %hd %hu", -1234567890123LL, 0x123456789abcULL, 70000, 70000);
    CHECK("%s|%10s|%-10s|%.2s", "abc", "abc", "abc", "abc");
    CHECK("%c%c %%", 'h', 'i');
    EXPECT("1234", "%p", (void *)0x1234);
    CHECK("%f %.2f %10.3f %-10.1f|", 3.14159, -2.5, 1234.5678, 0.26);
    EXPECT("0.0", "%.1f", 0.05);
    CHECK("%e %E %.3e %g %G", 12345.678, 0.000123, 1e100, 0.0001234, 123456789.0);
    CHECK("%g %g %g %#.0f", 100.0, 1e-5, 0.0, 3.0);
    EXPECT("3", "%.0f", 2.5);
    CHECK("%08.2f %.10f", -3.25, 1.0 / 3.0);
    EXPECT("Inf -Inf NaN", "%f %f %f", 1.0 / 0.0, -1.0 / 0.0, __builtin_nan(""));
    CHECK("%*d|%-*d|%.*f", 6, 7, 6, 7, 3, 2.0 / 3.0);
    CHECK("%5.3d %.0d|%x", 7, 0, 0);
    printf(failures ? "%d failures\n" : "printf check passed\n", failures);
    return failures != 0;
}
