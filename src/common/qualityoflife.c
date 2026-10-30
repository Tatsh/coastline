#include "common/qualityoflife.h"

bool IsSecondPressWithin(unsigned *pLastPress, unsigned now, unsigned window) {
    bool second = *pLastPress != 0 && now - *pLastPress <= window;

    *pLastPress = second ? 0 : now;
    return second;
}
