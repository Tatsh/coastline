/* Host check of the helpers of the quality of life changes in src/common/qualityoflife.c. */
#include <assert.h>
#include <stdio.h>

#include "common/qualityoflife.h"

static void TestSecondPress(void) {
    unsigned last = 0;

    /* A first press is not a second press, and a press within the window completes the pair. */
    assert(!IsSecondPressWithin(&last, 1000, 500));
    assert(IsSecondPressWithin(&last, 1500, 500));
    /* The pair is consumed, so a third press starts over. */
    assert(!IsSecondPressWithin(&last, 1600, 500));
    /* A press after the window starts a new pair in place of completing the old one. */
    assert(!IsSecondPressWithin(&last, 2101, 500));
    assert(IsSecondPressWithin(&last, 2200, 500));
    /* The counter of milliseconds wraps between the presses. */
    last = 0;
    assert(!IsSecondPressWithin(&last, 0xffffff00u, 500));
    assert(IsSecondPressWithin(&last, 0x00000010u, 500));
}

int main(void) {
    TestSecondPress();
    puts("qualityoflife: ok");
    return 0;
}
