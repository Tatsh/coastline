#include <assert.h>
#include <string.h>

#include "n64/decaleditor.h"
#include "n64/paintundo.h"

static VehicleModel model;
static ModelTexture textures[DECAL_SLOTS];
static u8 pixels[DECAL_SLOTS][64];
static u16 palette[16];
static u8 memory[1024];
static u8 expandedMemory[0x240000];

static void Init(void) {
    s32 i;

    memset(&model, 0, sizeof(model));
    memset(textures, 0, sizeof(textures));
    memset(pixels, 0, sizeof(pixels));
    memset(palette, 0, sizeof(palette));
    model.textures = textures;
    for (i = 0; i < DECAL_SLOTS; i++) {
        model.materialTextures[i] = i;
        textures[i].image = pixels[i];
        textures[i].size = sizeof(pixels[i]);
    }
    InitPaintUndo(memory, sizeof(memory), &model, palette);
}

int main(void) {
    s32 i;

    Init();
    assert(!PaintUndoIsDirty() && !PaintUndoCanUndo());
    RecordPaintUndoSlot(0);
    pixels[0][0] = 1;
    RecordPaintUndoSlot(1);
    pixels[1][0] = 2;
    assert(PaintUndoCanUndo() && PaintUndoIsDirty());
    assert(UndoPaintChange() == PAINT_UNDO_SLOT && pixels[1][0] == 0);
    assert(UndoPaintChange() == PAINT_UNDO_SLOT && pixels[0][0] == 0);
    assert(!PaintUndoIsDirty() && !PaintUndoCanUndo());
    assert(RedoPaintChange() == PAINT_UNDO_SLOT && pixels[0][0] == 1);
    assert(RedoPaintChange() == PAINT_UNDO_SLOT && pixels[1][0] == 2);
    MarkPaintUndoSaved();
    assert(!PaintUndoIsDirty());

    RecordPaintUndoAll();
    for (i = 0; i < DECAL_SLOTS; i++) {
        pixels[i][0] = i + 10;
    }
    assert(UndoPaintChange() == PAINT_UNDO_ALL);
    assert(pixels[0][0] == 1 && pixels[1][0] == 2 && pixels[9][0] == 0);
    assert(!PaintUndoIsDirty());
    assert(RedoPaintChange() == PAINT_UNDO_ALL);
    assert(pixels[0][0] == 10 && pixels[9][0] == 19);

    RecordPaintUndoPalette();
    palette[0] = 0x1234;
    assert(UndoPaintChange() == PAINT_UNDO_PALETTE && palette[0] == 0);
    assert(RedoPaintChange() == PAINT_UNDO_PALETTE && palette[0] == 0x1234);
    RecordPaintUndoPalette();
    CancelPaintUndoPalette();
    assert(UndoPaintChange() == PAINT_UNDO_PALETTE && palette[0] == 0);

    RecordPaintUndoSlot(2);
    pixels[2][0] = 5;
    assert(RedoPaintChange() == -1);
    assert(UndoPaintChange() == PAINT_UNDO_SLOT && pixels[2][0] == 12);

    InitPaintUndo(expandedMemory, sizeof(expandedMemory), &model, palette);
    for (i = 0; i < 4096; i++) {
        RecordPaintUndoSlot(0);
        pixels[0][0] = (u8)(i + 1);
    }
    for (i = 4095; i >= 0; i--) {
        assert(UndoPaintChange() == PAINT_UNDO_SLOT);
        assert(pixels[0][0] == (i ? (u8)i : 10));
    }
    assert(!PaintUndoCanUndo());

    InitPaintUndo(memory, 80, &model, palette);
    for (i = 0; i < 200; i++) {
        RecordPaintUndoSlot(0);
        pixels[0][0] = i;
    }
    assert(PaintUndoCanUndo());
    assert(UndoPaintChange() == PAINT_UNDO_SLOT);
    assert(UndoPaintChange() == -1);

    InitPaintUndo(memory, 0, &model, palette);
    RecordPaintUndoSlot(0);
    assert(PaintUndoIsDirty() && !PaintUndoCanUndo());
    ResetPaintUndo();
    assert(!PaintUndoIsDirty());
    return 0;
}
