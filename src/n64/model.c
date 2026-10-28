#include "model.h"

#include "system.h"

/* The offsets a file holds are below this; the pointers they become are above it. */
#define MODEL_OFFSET_LIMIT 0x7FFFFFFF

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
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    ReadConsoleModel(dst, romEnd - romStart);
#else
    RelocateModelPointers(dst);
#endif
    return dst;
}

Model *DecompressModel(void *dst, u32 romOffset) {
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    ReadConsoleModel(dst, DecompressFromRom(dst, romOffset, NULL));
#else
    DecompressFromRom(dst, romOffset, NULL);
    RelocateModelPointers(dst);
#endif
    return dst;
}
