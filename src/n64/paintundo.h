#ifndef COASTLINE_N64_PAINTUNDO_H
#define COASTLINE_N64_PAINTUNDO_H

#include "common/vehiclemodel.h"

#define PAINT_UNDO_SLOT 0
#define PAINT_UNDO_ALL 1
#define PAINT_UNDO_PALETTE 2

/**
 * Initialise paint history in the remaining interface memory.
 *
 * @param memory Writable history storage.
 * @param capacity Number of available bytes in memory.
 * @param model Vehicle model containing the decal textures.
 * @param palette Decal palette to restore with colour edits.
 */
void InitPaintUndo(u8 *memory, u32 capacity, VehicleModel *model, u16 *palette);

/** Record the current pixels of one slot before an edit.
 * @param slot Index of the slot to record.
 */
void RecordPaintUndoSlot(s32 slot);

/** Record every slot before an edit that changes the whole car. */
void RecordPaintUndoAll(void);

/** Record the palette before its colour editor changes it. */
void RecordPaintUndoPalette(void);

/** Discard the latest entry when a palette edit is cancelled. */
void CancelPaintUndoPalette(void);

/** Restore the previous edit.
 * @return The restored entry type, or -1 when history is empty.
 */
s32 UndoPaintChange(void);

/** Reapply the next edit.
 * @return The reapplied entry type, or -1 when redo history is empty.
 */
s32 RedoPaintChange(void);

/** Record the current history position as saved. */
void MarkPaintUndoSaved(void);

/** Clear history after loading another decal file. */
void ResetPaintUndo(void);

/** Return whether current decals differ from the saved history position.
 * @return Nonzero if there are unsaved edits.
 */
s32 PaintUndoIsDirty(void);

/** Return whether an earlier edit can be restored.
 * @return Nonzero if undo history is available.
 */
s32 PaintUndoCanUndo(void);

#endif /* COASTLINE_N64_PAINTUNDO_H */
