#include <ultra64.h>

/* The quotient rounds towards zero, whatever the division instruction does with negative
 * operands. */
lldiv_t lldiv(long long num, long long denom) {
    lldiv_t r;

    r.quot = num / denom;
    r.rem = num - denom * r.quot;
    if (r.quot < 0 && r.rem > 0) {
        r.quot++;
        r.rem -= denom;
    }
    return r;
}

ldiv_t ldiv(long num, long denom) {
    ldiv_t r;

    r.quot = num / denom;
    r.rem = num - denom * r.quot;
    if (r.quot < 0 && r.rem > 0) {
        r.quot++;
        r.rem -= denom;
    }
    return r;
}
