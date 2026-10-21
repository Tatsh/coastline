/* Host check of the 64-bit conversions against the branches of the disassembly. */
#include <assert.h>
#include <stdio.h>

unsigned long long __d_to_ull(double d);
unsigned long long __f_to_ull(float f);
double __ull_to_d(unsigned long long u);
float __ull_to_f(unsigned long long u);

int main(void) {
    const unsigned long long all = ~0ULL;

    assert(__d_to_ull(5.75) == 5);
    assert(__d_to_ull(-0.5) == 0);
    assert(__d_to_ull(-1.0) == all);
    assert(__d_to_ull(9223372036854775808.0) == 1ULL << 63);
    assert(__d_to_ull(9223372036854777856.0) == (1ULL << 63 | 2048));
    assert(__d_to_ull(18446744073709551616.0) == all);
    assert(__f_to_ull(3.5f) == 3);
    assert(__f_to_ull(-2.0f) == all);
    assert(__f_to_ull(9223372036854775808.0f) == 1ULL << 63);
    /* The top bit is corrected by 2^32, not by 2^64. */
    assert(__ull_to_d(1ULL << 63) == -9223372036854775808.0 + 4294967296.0);
    assert(__ull_to_d(12345) == 12345.0);
    assert(__ull_to_f(1ULL << 63) == -9223372036854775808.0f + 4294967296.0f);
    puts("llcvt check passed");
    return 0;
}
