#include "common/frametime.h"
#include "common/model.h"

#if TGR_BOSS_RALLY
#include "common/convertedvertex.h"
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
#endif /* TGR_BOSS_RALLY */

/* The blend between two keys, in 4096ths. */
#define BLEND_ONE 4096.0f
#define BLEND_SHIFT 12

#if TGR_BOSS_RALLY
/* Set the vertices of an animation between two keys, at a share from 0 to 1 of the way. */
static BOSS_RALLY_INLINE void
BlendModelKeys(ModelAnimation *animation, ModelKeyframe *from, ModelKeyframe *to, f32 share) {
#if TGR_BOSS_RALLY
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
#endif /* TGR_BOSS_RALLY */
}

/* Blend an animation at a time between the keys around it, searching from its start key. The
 * search past the last key reads one past the array. */
static BOSS_RALLY_INLINE void BlendModelAnimationAt(ModelAnimation *animation, f32 time) {
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
    BlendModelKeys(
        animation,
        from,
        to,
        (f32)((ANIMATION_HIGH(time) - from->time) / (ANIMATION_HIGH(to->time) - from->time)));
}

/* Run an animation backwards for a frame. */
static BOSS_RALLY_INLINE void AdvanceReversingAnimation(ModelAnimation *animation) {
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
    time = (f32)(ANIMATION_HIGH(animation->startTime) * 2.0f - time);
    animation->time = time;
    BlendModelAnimationAt(animation, time);
}

/* Run an animation forwards for a frame, wrapping or turning it back past its end. */
static BOSS_RALLY_INLINE void AdvanceForwardAnimation(ModelAnimation *animation) {
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
        span = (f32)(ANIMATION_LENGTH(animation, length) * 2.0f);
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
            time = (f32)(ANIMATION_HIGH(animation->endTime) * 2.0f - time);
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
    time = (f32)(ANIMATION_HIGH(time) - ANIMATION_LENGTH(animation, length));
    animation->time = time;
    animation->startKey = 0;
    BlendModelAnimationAt(animation, time);
}
#endif /* TGR_BOSS_RALLY */

void AdvanceModelAnimations(Model *model) {
#if TGR_BOSS_RALLY
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
#else
    s32 i;
    s32 count;
    ModelAnimation *animation;
    f32 time;
    f32 step;
    f32 limit;
    s32 key;
    ModelKeyframe *from;
    ModelKeyframe *to;
    s16 *fromPositions;
    s16 *toPositions;
    s8 *fromNormals;
    s8 *toNormals;
    Vtx *vertices;
    s32 searchCount;
    s32 blend;
    s32 vertexCount;
    s32 flags;
    s32 offset;

    if (model->animations != NULL) {
        /* This dead use gives IDO the ROM's register order for the animation index. */
        flags = offset;
        count = model->animations->count;
        for (i = 0, offset = 0; i < count; i++, offset += 4) {
            animation = *(ModelAnimation **)((u8 *)model->animations->entries + offset);
            if (animation->flags & MODEL_ANIMATION_REVERSING) {
                animation->time -= g_fFrameDeltaTime;
                time = animation->time;
                if (animation->startTime <= time) {
                } else {
                    goto back_to_start;
                }
                if (!(time < animation->endTime)) {
                    continue;
                }
            search:
                searchCount = animation->keyCount;
                /* The empty test preserves the key-count lifetime in IDO. */
                if (searchCount) {
                }
                vertexCount = animation->keyCount;
                for (key = animation->startKey; key < vertexCount; key++) {
                    if (time < animation->keys[key]->time) {
                        break;
                    }
                }
                to = animation->keys[key];
                key--;
                from = animation->keys[key];
                time = (time - from->time) / (to->time - from->time);
                key = 0;
            blend:
                blend = time * BLEND_ONE;
                vertexCount = animation->vertexCount;
                fromPositions = &from->positions[0][0];
                toPositions = &to->positions[0][0];
                fromNormals = (s8 *)(fromPositions + vertexCount * 3);
                toNormals = (s8 *)(toPositions + vertexCount * 3);
                vertices = animation->vertices;
                for (key = 0; key < vertexCount; key++) {
                    vertices[key].v.ob[0] =
                        ((toPositions[0] - fromPositions[0]) * blend >> BLEND_SHIFT) +
                        fromPositions[0];
                    vertices[key].v.ob[1] =
                        ((toPositions[1] - fromPositions[1]) * blend >> BLEND_SHIFT) +
                        fromPositions[1];
                    vertices[key].v.ob[2] =
                        ((toPositions[2] - fromPositions[2]) * blend >> BLEND_SHIFT) +
                        fromPositions[2];
                    vertices[key].v.cn[0] =
                        ((toNormals[0] - fromNormals[0]) * blend >> BLEND_SHIFT) + fromNormals[0];
                    vertices[key].v.cn[1] =
                        ((toNormals[1] - fromNormals[1]) * blend >> BLEND_SHIFT) + fromNormals[1];
                    vertices[key].v.cn[2] =
                        ((toNormals[2] - fromNormals[2]) * blend >> BLEND_SHIFT) + fromNormals[2];
                    fromPositions += 3;
                    toPositions += 3;
                    fromNormals += 3;
                    toNormals += 3;
                }
                continue;
            back_to_start:
                flags = animation->flags;
                if (!(flags & MODEL_ANIMATION_REPEAT)) {
                    continue;
                }
                animation->flags = flags & ~MODEL_ANIMATION_REVERSING;
                animation->startKey = 0;
                time = animation->startTime * 2 - time;
                animation->time = time;
                goto search;
            } else {
                animation->time += g_fFrameDeltaTime;
                time = animation->time;
                if (time < animation->startTime) {
                    time = 0.0f;
                    to = from = animation->keys[0];
                    key = 0;
                    goto blend;
                }
                if (time < animation->endTime) {
                    if (animation->startTime <= time) {
                        goto search;
                    }
                    continue;
                }
                if (animation->flags & MODEL_ANIMATION_REPEAT) {
                    step = animation->endTime - animation->startTime;
                    limit = animation->endTime + step;
                    if (animation->flags & MODEL_ANIMATION_BOUNCE) {
                        step = (animation->endTime - animation->startTime) * 2;
                        limit = animation->endTime + step;
                        while (limit < time) {
                            time -= step;
                        }
                        step *= 0.5f;
                        limit -= step;
                        if (limit < time) {
                            goto wrap;
                        }
                        animation->flags |= MODEL_ANIMATION_REVERSING;
                        time = animation->endTime * 2 - time;
                        animation->time = time;
                    } else {
                        step = animation->endTime - animation->startTime;
                        limit = animation->endTime + step;
                    wrap:
                        while (limit < time) {
                            time -= step;
                        }
                        time -= animation->endTime - animation->startTime;
                        animation->time = time;
                    }
                    animation->startKey = 0;
                    goto search;
                } else {
                    time = 0.0f;
                    to = from = animation->keys[animation->keyCount - 1];
                    key = 0;
                    goto blend;
                }
            }
        }
    }
#endif /* TGR_BOSS_RALLY */
}
