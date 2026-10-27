#include "model.h"

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
