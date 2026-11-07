#include "common/system.h"

#include <PR/R4300.h>
#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/os_libc.h>

#include "common/audio.h"
#include "common/camera.h"
#include "common/debugtext.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/input.h"
#include "common/libultra/os/osint.h"
#include "common/model.h"
#include "common/race.h"
#include "common/render.h"
#include "common/rumble.h"
#include "common/texrect.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackmenuitem.h"
#include "common/trackrender.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/weather.h"
#include "n64/boot.h"
#include "zlib.h"

/* The offsets a file holds are below this; the pointers they become are above it. */
#define MODEL_OFFSET_LIMIT ((void *)0x7FFFFFFF)
#define MODEL_BASE ((void *)base)

s32 IsVehicleModelValid(s32 car) {
    return g_aCarModels[car].modelRomEnd != 0;
}

void RelocateModelPointers(Model *model) {
    u32 base = (u32)model;
    s32 i;
    s32 k;

    /* The fields of the file hold offsets from its start until they are remapped. */
    if (model->animations != NULL) {
        RemapPointer((u32 *)&model->animations, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
        for (i = 0; i < model->animations->count; i++) {
            RemapPointer((u32 *)&model->animations->entries[i], 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
            RemapPointer(
                (u32 *)&model->animations->entries[i]->vertices, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
            RemapPointer((u32 *)&model->animations->entries[i]->unreconstructed8,
                         0,
                         MODEL_OFFSET_LIMIT,
                         MODEL_BASE);
            for (k = 0; k < model->animations->entries[i]->keyCount; k++) {
                RemapPointer((u32 *)&model->animations->entries[i]->keys[k],
                             0,
                             MODEL_OFFSET_LIMIT,
                             MODEL_BASE);
            }
        }
    }
    for (i = 0; (u32)i < model->partCount; i++) {
        RemapPointer((u32 *)&model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
        /* An offset of 0 has become the model itself, and is walked as a display list. */
        if (model->parts[i].displayList != NULL) {
            RemapPointer((u32 *)&model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
            FixupDisplayListPointers(
                model->parts[i].displayList, 0, MODEL_OFFSET_LIMIT, MODEL_BASE);
        }
    }
}

Model *DmaReadAndFixupTrackModel(void *dst, u32 romStart, u32 romEnd) {
    DmaRead(dst, romStart, romEnd - romStart);
    RelocateModelPointers(dst);
    return dst;
}
