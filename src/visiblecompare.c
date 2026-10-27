#include "trackrender.h"

s32 CompareVisibleCellDistance(const void *a, const void *b) {
    s16 second = ((const VisibleCell *)b)->distance;
    s16 first = ((const VisibleCell *)a)->distance;

    if (second < first) {
        return 1;
    }
    if (first < second) {
        return -1;
    }
    return 0;
}
