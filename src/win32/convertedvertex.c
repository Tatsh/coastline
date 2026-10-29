#include "common/convertedvertex.h"

#include <string.h>

static inline void CopyVertexComponent(f32 *dest, const f32 *source) {
    u32 word;

    memcpy(&word, source, sizeof(word));
    memcpy(dest, &word, sizeof(word));
}

void CopyConvertedModelVertex(ConvertedModelVertex *dest, const ConvertedModelVertex *source) {
    CopyVertexComponent(&dest->position.x, &source->position.x);
    CopyVertexComponent(&dest->position.y, &source->position.y);
    CopyVertexComponent(&dest->position.z, &source->position.z);
    CopyVertexComponent(&dest->texture.x, &source->texture.x);
    CopyVertexComponent(&dest->texture.y, &source->texture.y);
    CopyVertexComponent(&dest->normal.x, &source->normal.x);
    CopyVertexComponent(&dest->normal.y, &source->normal.y);
    CopyVertexComponent(&dest->normal.z, &source->normal.z);
}
