#include "render.h"
#include "trackshadow.h"

long double ShadowTextureDistanceLeft(const ConvertedModelVertex *vertex) {
    return vertex->texture.x;
}

long double ShadowTextureDistanceRight(const ConvertedModelVertex *vertex) {
    return 1024.0L - vertex->texture.x;
}

long double ShadowTextureDistanceTop(const ConvertedModelVertex *vertex) {
    return vertex->texture.y;
}

long double ShadowTextureDistanceBottom(const ConvertedModelVertex *vertex) {
    return 1024.0L - vertex->texture.y;
}

void ProjectShadowTextureCoordinates(ConvertedModelVertex *vertex) {
    f32 x = vertex->position.x;
    f32 y = vertex->position.y;
    f32 z = vertex->position.z;

#if TGR_RENDERER == TGR_RENDERER_GLIDE
    /* BRGlide adds the products in the order of the rows, and BRD3D adds the z product second. */
    vertex->texture.x =
        (((long double)x * g_afMvpMatrix[0][0] + (long double)y * g_afMvpMatrix[1][0]) +
         (long double)z * g_afMvpMatrix[2][0]) +
        g_afMvpMatrix[3][0];
    vertex->texture.y =
        (((long double)x * g_afMvpMatrix[0][1] + (long double)y * g_afMvpMatrix[1][1]) +
         (long double)z * g_afMvpMatrix[2][1]) +
        g_afMvpMatrix[3][1];
#else
    vertex->texture.x =
        (((long double)x * g_afMvpMatrix[0][0] + (long double)z * g_afMvpMatrix[2][0]) +
         (long double)y * g_afMvpMatrix[1][0]) +
        g_afMvpMatrix[3][0];
    vertex->texture.y =
        (((long double)x * g_afMvpMatrix[0][1] + (long double)z * g_afMvpMatrix[2][1]) +
         (long double)y * g_afMvpMatrix[1][1]) +
        g_afMvpMatrix[3][1];
#endif
}

void UpdateShadowCornerVertex(const TrackInstance *instance,
                              ConvertedModelVertex quad[4],
                              s32 cornerValid[4],
                              s32 corner,
                              const ConvertedModelVertex *vertex,
                              f32 s,
                              f32 t,
                              const f32 vehicleMatrix[4][4]) {
    ConvertedModelVertex *output = &quad[corner];
    long double vertexS = (long double)vertex->texture.x - s;
    long double vertexT = (long double)vertex->texture.y - t;
    long double cornerS = (long double)output->texture.x - s;
    long double cornerT = (long double)output->texture.y - t;
    /* The original spills three differences, but retains the corner t difference in x87. */
    f32 roundedVertexS = vertexS;
    f32 roundedVertexT = vertexT;
    f32 roundedCornerS = cornerS;
    long double vertexDistance = vertexT * roundedVertexT + vertexS * roundedVertexS;
    long double cornerDistance = cornerS * roundedCornerS + cornerT * cornerT;
    long double z;

    if (!(vertexDistance < cornerDistance)) {
        return;
    }
    z = (((long double)instance->matrix[2][2] * vertex->position.z +
          (long double)instance->matrix[1][2] * vertex->position.y) +
         (long double)instance->matrix[0][2] * vertex->position.x) +
        instance->matrix[3][2];
    if (!(z <= (long double)vehicleMatrix[3][2] + 1.0L)) {
        return;
    }
    output->position.x = (((long double)instance->matrix[2][0] * vertex->position.z +
                           (long double)instance->matrix[1][0] * vertex->position.y) +
                          (long double)instance->matrix[0][0] * vertex->position.x) +
                         instance->matrix[3][0];
    output->position.y = (((long double)instance->matrix[2][1] * vertex->position.z +
                           (long double)instance->matrix[1][1] * vertex->position.y) +
                          (long double)instance->matrix[0][1] * vertex->position.x) +
                         instance->matrix[3][1];
    output->position.z = z;
    output->texture.x = vertex->texture.x;
    output->texture.y = vertex->texture.y;
    cornerValid[corner] = 1;
}
