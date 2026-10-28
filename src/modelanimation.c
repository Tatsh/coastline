#include "frametime.h"
#include "model.h"

#if TGR_PC
#include "convertedvertex.h"
#define ANIMATION_HIGH(value) ((long double)(value))
#define ANIMATION_LESS(a, b) (!((a) >= (b)))
#define ANIMATION_LENGTH(animation, length)                                                        \
    ((long double)(animation)->endTime - (animation)->startTime)

static inline s32 BlendKeyComponent(s32 from, s32 to, s32 blend) {
    s32 product = (s32)((u32)(to - from) * (u32)blend);
    return (s32)((u32)from + (u32)(product >> 12));
}
#else
#define ANIMATION_HIGH(value) (value)
#define ANIMATION_LESS(a, b) ((a) < (b))
#define ANIMATION_LENGTH(animation, length) (length)
#endif /* TGR_PC */

/* The blend between two keys, in 4096ths. */
#define BLEND_ONE 4096.0f
#define BLEND_SHIFT 12

/* Set the vertices of an animation between two keys, at a share from 0 to 1 of the way. */
static void
BlendModelKeys(ModelAnimation *animation, ModelKeyframe *from, ModelKeyframe *to, f32 share) {
#if TGR_PC
    long double scaled = (long double)share * BLEND_ONE;
    s32 blend = 0;
    s32 count = animation->vertexCount;
    s8(*fromNormals)[3] = (s8(*)[3]) & from->positions[count];
    s8(*toNormals)[3] = (s8(*)[3]) & to->positions[count];
    ConvertedModelVertex *vertex = animation->vertices.pConverted;
    s32 j, c;
    if (scaled >= -9223372036854775808.0L && scaled < 9223372036854775808.0L) {
        blend = (s32)(u32)(s64)scaled;
    }
    for (j = 0; j < count; j++, vertex++) {
        f32 *position[3] = {&vertex->position.x, &vertex->position.y, &vertex->position.z};
        f32 *normal[3] = {&vertex->normal.x, &vertex->normal.y, &vertex->normal.z};
        for (c = 0; c < 3; c++) {
            *position[c] =
                (s16)BlendKeyComponent(from->positions[j][c], to->positions[j][c], blend);
        }
        for (c = 0; c < 3; c++) {
            *normal[c] =
                (s8)BlendKeyComponent(fromNormals[j][c], toNormals[j][c], blend) * 0.0078125f;
        }
    }
#else
    s32 blend = (s32)(share * BLEND_ONE);
    s32 count = animation->vertexCount;
    /* The normals follow the positions of every vertex. */
    s8(*fromNormals)[3] = (s8(*)[3]) & from->positions[count];
    s8(*toNormals)[3] = (s8(*)[3]) & to->positions[count];
    Vtx *vertex = animation->vertices;
    s32 j;
    s32 c;

    for (j = 0; j < count; j++, vertex++) {
        for (c = 0; c < 3; c++) {
            vertex->v.ob[c] =
                from->positions[j][c] +
                ((to->positions[j][c] - from->positions[j][c]) * blend >> BLEND_SHIFT);
        }
        for (c = 0; c < 3; c++) {
            vertex->n.n[c] =
                fromNormals[j][c] + ((toNormals[j][c] - fromNormals[j][c]) * blend >> BLEND_SHIFT);
        }
    }
#endif /* TGR_PC */
}

/* Blend an animation at a time between the keys around it, searching from its start key. The
 * search past the last key reads one past the array. */
static void BlendModelAnimationAt(ModelAnimation *animation, f32 time) {
    ModelKeyframe *from;
    ModelKeyframe *to;
    s32 k;

    for (k = animation->startKey; k < animation->keyCount; k++) {
        if (time < animation->keys[k]->time) {
            break;
        }
    }
    to = animation->keys[k];
    from = animation->keys[k - 1];
    BlendModelKeys(animation,
                   from,
                   to,
                   (ANIMATION_HIGH(time) - from->time) / (ANIMATION_HIGH(to->time) - from->time));
}

/* Run an animation backwards for a frame. */
static void AdvanceReversingAnimation(ModelAnimation *animation) {
    f32 time;

    animation->time -= g_fFrameDeltaTime;
    time = animation->time;
    if (animation->startTime <= time) {
        if (ANIMATION_LESS(time, animation->endTime)) {
            BlendModelAnimationAt(animation, time);
        }
        return;
    }
    if (!(animation->flags & MODEL_ANIMATION_REPEAT)) {
        return;
    }
    /* Back at the start, it turns forwards again. */
    animation->flags &= ~MODEL_ANIMATION_REVERSING;
    animation->startKey = 0;
    time = ANIMATION_HIGH(animation->startTime) * 2.0f - time;
    animation->time = time;
    BlendModelAnimationAt(animation, time);
}

/* Run an animation forwards for a frame, wrapping or turning it back past its end. */
static void AdvanceForwardAnimation(ModelAnimation *animation) {
    f32 time;
    f32 length;
    f32 span;
    f32 limit;

    animation->time += g_fFrameDeltaTime;
    time = animation->time;
    if (ANIMATION_LESS(time, animation->startTime)) {
        BlendModelKeys(animation, animation->keys[0], animation->keys[0], 0.0f);
        return;
    }
    if (ANIMATION_LESS(time, animation->endTime)) {
        if (animation->startTime <= time) {
            BlendModelAnimationAt(animation, time);
        }
        return;
    }
    if (!(animation->flags & MODEL_ANIMATION_REPEAT)) {
        BlendModelKeys(animation,
                       animation->keys[animation->keyCount - 1],
                       animation->keys[animation->keyCount - 1],
                       0.0f);
        return;
    }

    length = animation->endTime - animation->startTime;
    span = length;
    if (animation->flags & MODEL_ANIMATION_BOUNCE) {
        /* A turn back and forth takes twice the length. */
        span = ANIMATION_LENGTH(animation, length) * 2.0f;
        limit = animation->endTime + span;
        if (limit < time) {
            time -= span;
            while (limit < time) {
                time -= span;
            }
        }
        span = span * 0.5f;
        limit = limit - span;
        if (!(limit < time)) {
            animation->flags |= MODEL_ANIMATION_REVERSING;
            time = ANIMATION_HIGH(animation->endTime) * 2.0f - time;
            animation->time = time;
            animation->startKey = 0;
            BlendModelAnimationAt(animation, time);
            return;
        }
    } else {
        limit = animation->endTime + span;
    }
    if (limit < time) {
        time -= span;
        while (limit < time) {
            time -= span;
        }
    }
    time = ANIMATION_HIGH(time) - ANIMATION_LENGTH(animation, length);
    animation->time = time;
    animation->startKey = 0;
    BlendModelAnimationAt(animation, time);
}

void AdvanceModelAnimations(Model *model) {
    ModelAnimation *animation;
    s32 count;
    s32 i;

    if (model->animations == NULL) {
        return;
    }
    count = model->animations->count;
    for (i = 0; i < count; i++) {
        animation = model->animations->entries[i];
        if (animation->flags & MODEL_ANIMATION_REVERSING) {
            AdvanceReversingAnimation(animation);
        } else {
            AdvanceForwardAnimation(animation);
        }
    }
}
