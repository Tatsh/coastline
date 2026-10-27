#ifndef COASTLINE_WHEEL_FRAME_H
#define COASTLINE_WHEEL_FRAME_H

#include "vecmath.h"

/** The axes and the position of a wheel, the rows of its matrix. */
typedef struct {
    Vec3f forward;  /*!< The forward axis of the wheel. */
    f32 forwardW;   /*!< 0, the last column of the matrix. */
    Vec3f side;     /*!< The sideways axis of the wheel. */
    f32 sideW;      /*!< 0, the last column of the matrix. */
    Vec3f up;       /*!< The upward axis of the wheel. */
    f32 upW;        /*!< 0, the last column of the matrix. */
    Vec3f position; /*!< The position of the wheel. */
    f32 positionW;  /*!< 1, the last column of the matrix. */
} WheelFrame;

#endif
