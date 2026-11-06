#include <assert.h>

#include "common/paintshape.h"

int main(void) {
    s32 x = 27;
    s32 y = 14;

    ConstrainPaintEndpoint(10, 10, &x, &y, FALSE);
    assert(x == 27 && y == 10);

    x = 14;
    y = -9;
    ConstrainPaintEndpoint(10, 10, &x, &y, FALSE);
    assert(x == 10 && y == -9);

    x = -5;
    y = 16;
    ConstrainPaintEndpoint(10, 10, &x, &y, TRUE);
    assert(x == 4 && y == 16);

    x = 19;
    y = -3;
    ConstrainPaintEndpoint(10, 10, &x, &y, TRUE);
    assert(x == 19 && y == 1);
    return 0;
}
