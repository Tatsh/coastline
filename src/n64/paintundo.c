#include "n64/paintundo.h"

#include <PR/os_libc.h>

#include "n64/decaleditor.h"
#include "n64/paintshop.h"

#define PAINT_UNDO_HEADER_SIZE 4
#define PAINT_UNDO_TRAILER_SIZE 2
#define PAINT_UNDO_PALETTE_SIZE 0x20

/* Patched ROMs must not extend the original BSS past its fixed end address. */
static u8 *s_historyMemory = (u8 *)1;
static u32 s_historyCapacity = 1;
static u32 s_historyUsed = 1;
static u32 s_historyCursor = 1;
static u32 s_savedCursor = 1;
static s32 s_savedValid = 1;
static VehicleModel *s_historyModel = (VehicleModel *)1;
static u16 *s_historyPalette = (u16 *)1;
static u16 s_slotSizes[DECAL_SLOTS] = {1};

static u16 ReadRecordSize(const u8 *bytes) {
    return ((u16)bytes[0] << 8) | bytes[1];
}

static void WriteRecordSize(u8 *bytes, u16 size) {
    bytes[0] = size >> 8;
    bytes[1] = size;
}

static u8 *SlotPixels(s32 slot) {
    return s_historyModel->textures[s_historyModel->materialTextures[slot]].image;
}

static u32 PayloadSize(s32 type, s32 slot) {
    u32 size = 0;
    s32 i;

    if (type == PAINT_UNDO_SLOT) {
        return s_slotSizes[slot];
    }
    if (type == PAINT_UNDO_PALETTE) {
        return PAINT_UNDO_PALETTE_SIZE;
    }
    for (i = 0; i < DECAL_SLOTS; i++) {
        size += s_slotSizes[i];
    }
    return size;
}

static void RefreshSavedPosition(void) {
    if (s_savedValid && s_savedCursor > s_historyUsed) {
        s_savedValid = FALSE;
    }
}

static void RecordPaintUndo(s32 type, s32 slot) {
    u32 recordSize = PAINT_UNDO_HEADER_SIZE + PayloadSize(type, slot) + PAINT_UNDO_TRAILER_SIZE;
    u8 *record;
    u8 *payload;
    u16 oldestSize;
    s32 i;
    u32 byte;

    s_historyUsed = s_historyCursor;
    RefreshSavedPosition();
    if (recordSize > s_historyCapacity || recordSize > 0xFFFF) {
        s_historyUsed = s_historyCursor = 0;
        s_savedValid = FALSE;
        return;
    }
    while (s_historyUsed + recordSize > s_historyCapacity) {
        oldestSize = ReadRecordSize(s_historyMemory);
        for (byte = oldestSize; byte < s_historyUsed; byte++) {
            s_historyMemory[byte - oldestSize] = s_historyMemory[byte];
        }
        s_historyUsed -= oldestSize;
        s_historyCursor -= oldestSize;
        if (s_savedValid) {
            if (s_savedCursor < oldestSize) {
                s_savedValid = FALSE;
            } else {
                s_savedCursor -= oldestSize;
            }
        }
    }
    record = s_historyMemory + s_historyUsed;
    WriteRecordSize(record, recordSize);
    record[2] = type;
    record[3] = slot;
    payload = record + PAINT_UNDO_HEADER_SIZE;
    if (type == PAINT_UNDO_SLOT) {
        memcpy(payload, SlotPixels(slot), s_slotSizes[slot]);
    } else if (type == PAINT_UNDO_PALETTE) {
        memcpy(payload, s_historyPalette, PAINT_UNDO_PALETTE_SIZE);
    } else {
        for (i = 0; i < DECAL_SLOTS; i++) {
            memcpy(payload, SlotPixels(i), s_slotSizes[i]);
            payload += s_slotSizes[i];
        }
    }
    WriteRecordSize(record + recordSize - PAINT_UNDO_TRAILER_SIZE, recordSize);
    s_historyCursor = s_historyUsed += recordSize;
}

static void SwapBytes(u8 *saved, u8 *current, u32 size) {
    u32 i;
    u8 value;

    for (i = 0; i < size; i++) {
        value = current[i];
        current[i] = saved[i];
        saved[i] = value;
    }
}

static s32 SwapRecord(u32 offset) {
    u8 *record = s_historyMemory + offset;
    u8 *payload = record + PAINT_UNDO_HEADER_SIZE;
    s32 type = record[2];
    s32 slot = record[3];
    s32 i;

    if (type == PAINT_UNDO_SLOT) {
        SwapBytes(payload, SlotPixels(slot), s_slotSizes[slot]);
    } else if (type == PAINT_UNDO_PALETTE) {
        SwapBytes(payload, (u8 *)s_historyPalette, PAINT_UNDO_PALETTE_SIZE);
    } else {
        for (i = 0; i < DECAL_SLOTS; i++) {
            SwapBytes(payload, SlotPixels(i), s_slotSizes[i]);
            payload += s_slotSizes[i];
        }
    }
    return type;
}

void InitPaintUndo(u8 *memory, u32 capacity, VehicleModel *model, u16 *palette) {
    s32 i;
    ModelTexture *texture;
    u32 size;

    s_historyMemory = memory;
    s_historyCapacity = capacity;
    s_historyModel = model;
    s_historyPalette = palette;
    for (i = 0; i < DECAL_SLOTS; i++) {
        texture = &model->textures[model->materialTextures[i]];
        size = MODEL_TEXTURE_SIZE(texture);
        s_slotSizes[i] = size < DECAL_BUFFER_SIZE ? size : DECAL_BUFFER_SIZE;
    }
    ResetPaintUndo();
}

void RecordPaintUndoSlot(s32 slot) {
    RecordPaintUndo(PAINT_UNDO_SLOT, slot);
}

void RecordPaintUndoAll(void) {
    RecordPaintUndo(PAINT_UNDO_ALL, 0);
}

void RecordPaintUndoPalette(void) {
    RecordPaintUndo(PAINT_UNDO_PALETTE, 0);
}

void CancelPaintUndoPalette(void) {
    u16 size;

    if (!s_historyCursor) {
        return;
    }
    size = ReadRecordSize(s_historyMemory + s_historyCursor - PAINT_UNDO_TRAILER_SIZE);
    if (s_historyMemory[s_historyCursor - size + 2] != PAINT_UNDO_PALETTE) {
        return;
    }
    s_historyUsed = s_historyCursor -= size;
    RefreshSavedPosition();
}

s32 UndoPaintChange(void) {
    u16 size;

    if (!s_historyCursor) {
        return -1;
    }
    size = ReadRecordSize(s_historyMemory + s_historyCursor - PAINT_UNDO_TRAILER_SIZE);
    s_historyCursor -= size;
    return SwapRecord(s_historyCursor);
}

s32 RedoPaintChange(void) {
    s32 type;

    if (s_historyCursor == s_historyUsed) {
        return -1;
    }
    type = SwapRecord(s_historyCursor);
    s_historyCursor += ReadRecordSize(s_historyMemory + s_historyCursor);
    return type;
}

void MarkPaintUndoSaved(void) {
    s_savedCursor = s_historyCursor;
    s_savedValid = TRUE;
}

void ResetPaintUndo(void) {
    s_historyUsed = 0;
    s_historyCursor = 0;
    s_savedCursor = 0;
    s_savedValid = TRUE;
}

s32 PaintUndoIsDirty(void) {
    return !s_savedValid || s_historyCursor != s_savedCursor;
}

s32 PaintUndoCanUndo(void) {
    return s_historyCursor != 0;
}
