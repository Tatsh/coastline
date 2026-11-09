#ifndef COASTLINE_CONVERTEDVERTEX_H
#define COASTLINE_CONVERTEDVERTEX_H

#include "common/vecmath.h"

/** A model vertex converted into the renderer's working format. */
typedef struct ConvertedModelVertex {
    Vec3f position; /*!< The vertex position. */
    Vec2f texture;  /*!< The texture coordinates. */
#if TGR_BOSS_RALLY
    union {
        Vec3f normal; /*!< The surface normal for lit geometry. */
        Vec3f color;  /*!< The vertex colour for unlit geometry. */
    };
#else
    Vec3f normal; /*!< The surface normal. */
#endif
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
 * @ghidraAddress BRGlide: 0x100189a0
 */
void CopyConvertedModelVertex(ConvertedModelVertex *dest, const ConvertedModelVertex *source);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_CONVERTEDVERTEX_H */
