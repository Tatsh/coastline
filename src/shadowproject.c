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

    vertex->texture.x =
        (((long double)x * g_afMvpMatrix[0][0] + (long double)z * g_afMvpMatrix[2][0]) +
         (long double)y * g_afMvpMatrix[1][0]) +
        g_afMvpMatrix[3][0];
    vertex->texture.y =
        (((long double)x * g_afMvpMatrix[0][1] + (long double)z * g_afMvpMatrix[2][1]) +
         (long double)y * g_afMvpMatrix[1][1]) +
        g_afMvpMatrix[3][1];
}
