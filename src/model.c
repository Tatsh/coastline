#include "model.h"

#include "system.h"

/* The offsets a file holds are below this; the pointers they become are above it. */
#define MODEL_OFFSET_LIMIT 0x7FFFFFFF

/* The blend between two keys, in 4096ths. */
#define BLEND_ONE 4096.0f
#define BLEND_SHIFT 12

void RemapPointer(u32 *pointer, u32 lo, u32 hi, u32 base) {
    if (*pointer >= lo && *pointer < hi) {
        *pointer = *pointer - lo + base;
    }
}

void FixupDisplayListPointers(Gfx *displayList, u32 lo, u32 hi, u32 base) {
    if (displayList == NULL) {
        return;
    }
    /* The display lists it calls are not followed. */
    for (;; displayList++) {
        switch ((u8)(displayList->words.w0 >> 24)) {
        case G_VTX:
        case G_SETTIMG:
            RemapPointer(&displayList->words.w1, lo, hi, base);
            break;
        case G_ENDDL:
            return;
        }
    }
}

void SetModelAnimationFlags(Model *model, s32 set, s32 clear) {
    ModelAnimation *animation;
    s32 count;
    s32 i;

    clear = ~clear;
    if (model->animations != NULL) {
        count = model->animations->count;
        for (i = 0; i < count; i++) {
            animation = model->animations->entries[i];
            animation->flags |= set;
            animation->flags &= clear;
        }
    }
}

void PlayModelAnimationsOnce(Model *model) {
    SetModelAnimationFlags(model, 0, MODEL_ANIMATION_REPEAT | MODEL_ANIMATION_BOUNCE);
}

void LoopModelAnimations(Model *model) {
    SetModelAnimationFlags(model, MODEL_ANIMATION_REPEAT, MODEL_ANIMATION_BOUNCE);
}

void BounceModelAnimations(Model *model) {
    SetModelAnimationFlags(model, MODEL_ANIMATION_REPEAT | MODEL_ANIMATION_BOUNCE, 0);
}

/* Set the vertices of an animation between two keys, at a share from 0 to 1 of the way. */
static void
BlendModelKeys(ModelAnimation *animation, ModelKeyframe *from, ModelKeyframe *to, f32 share) {
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
    BlendModelKeys(animation, from, to, (time - from->time) / (to->time - from->time));
}

/* Run an animation backwards for a frame. */
static void AdvanceReversingAnimation(ModelAnimation *animation) {
    f32 time;

    animation->time -= g_fFrameDeltaTime;
    time = animation->time;
    if (animation->startTime <= time) {
        if (time < animation->endTime) {
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
    time = animation->startTime * 2.0f - time;
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
    if (time < animation->startTime) {
        BlendModelKeys(animation, animation->keys[0], animation->keys[0], 0.0f);
        return;
    }
    if (time < animation->endTime) {
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
        span = length * 2.0f;
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
            time = animation->endTime * 2.0f - time;
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
    time -= length;
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

void RelocateModelPointers(Model *model) {
    u32 base = (u32)model;
    s32 i;
    s32 k;

    /* The fields of the file hold offsets from its start until they are remapped. */
    if (model->animations != NULL) {
        RemapPointer((u32 *)&model->animations, 0, MODEL_OFFSET_LIMIT, base);
        for (i = 0; i < model->animations->count; i++) {
            RemapPointer((u32 *)&model->animations->entries[i], 0, MODEL_OFFSET_LIMIT, base);
            RemapPointer(
                (u32 *)&model->animations->entries[i]->vertices, 0, MODEL_OFFSET_LIMIT, base);
            RemapPointer((u32 *)&model->animations->entries[i]->unreconstructed8,
                         0,
                         MODEL_OFFSET_LIMIT,
                         base);
            for (k = 0; k < model->animations->entries[i]->keyCount; k++) {
                RemapPointer(
                    (u32 *)&model->animations->entries[i]->keys[k], 0, MODEL_OFFSET_LIMIT, base);
            }
        }
    }
    for (i = 0; (u32)i < model->partCount; i++) {
        RemapPointer((u32 *)&model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, base);
        /* An offset of 0 has become the model itself, and is walked as a display list. */
        if (model->parts[i].displayList != NULL) {
            RemapPointer((u32 *)&model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, base);
            FixupDisplayListPointers(model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, base);
        }
    }
}

Model *DmaReadAndFixupTrackModel(void *dst, u32 romStart, u32 romEnd) {
    DmaRead(dst, romStart, romEnd - romStart);
    RelocateModelPointers(dst);
    return dst;
}

Model *DecompressModel(void *dst, u32 romOffset) {
    DecompressFromRom(dst, romOffset, NULL);
    RelocateModelPointers(dst);
    return dst;
}
