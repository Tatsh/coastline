#ifndef COASTLINE_CONVERTEDVERTEX_H
#define COASTLINE_CONVERTEDVERTEX_H

#include "vecmath.h"

typedef struct ConvertedModelVertex {
    Vec3f position;
    Vec2f texture;
    Vec3f normal;
} ConvertedModelVertex;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Copy the eight words of a converted model vertex in address order.
 *
 * @param dest The destination vertex.
 * @param source The source vertex.
 * @ghidraAddress BRD3D: 0x1002b930
 */
void CopyConvertedModelVertex(ConvertedModelVertex *dest, const ConvertedModelVertex *source);

#ifdef __cplusplus
}
#endif

#endif
