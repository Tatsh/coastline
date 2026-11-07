#include "common/render.h"
#include "common/vecmath.h"

/* NormalizeLightMatrix() scales a matrix to this range, less a margin, for the lighting. */
#define LIGHT_MATRIX_RANGE 1.9997559f
#define LIGHT_MATRIX_MARGIN 0.375f

/* Scale the four elements of a row of a matrix. */
#define SCALE_ROW(row, scale)                                                                      \
    {                                                                                              \
        (row)[0] *= scale;                                                                         \
        (row)[1] *= scale;                                                                         \
        (row)[2] *= scale;                                                                         \
        (row)[3] *= scale;                                                                         \
    }

f32 GetMaxAbsValue12(f32 m[4][4]) {
    f32 *element;
    f32 max, min;

    max = 0.0f;
    min = 0.0f;
    for (element = &m[0][0]; element < &m[3][0]; element++) {
        if (*element < 0.0f) {
            if (*element < min) {
                min = *element;
            }
        } else if (max < *element) {
            max = *element;
        }
    }
    min = -min;
    if (max < min) {
        return min;
    }
    return max;
}

void NormalizeLightMatrix(f32 m[4][4]) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    f32 *element;
    f32 max, min;

    max = 0.0f;
    min = 0.0f;
    for (element = &m[0][0]; element < &m[3][0]; element++) {
        if (*element < 0.0f) {
            if (*element < min) {
                min = *element;
            }
        } else if (max < *element) {
            max = *element;
        }
    }
    min = -min;
    if (max < min) {
        max = min;
    }
    /* The largest magnitude becomes the scale. */
    max = LIGHT_MATRIX_RANGE / (max + LIGHT_MATRIX_MARGIN);
    SCALE_ROW(m[0], max);
    SCALE_ROW(m[1], max);
    SCALE_ROW(m[2], max);
    SCALE_ROW(m[3], max);
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
}
