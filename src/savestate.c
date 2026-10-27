#include <PR/os.h>

#include "debugmessage.h"
#include "replay.h"
#include "timing.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <stdio.h>
#include <string.h>
#else
#include <PR/os_libc.h>
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
static void CopySnapshotBytes(u8 *destination, const void *sourceData, u32 size) {
    const u8 *source = sourceData;
    u32 word;

    while (size >= 4) {
        memcpy(&word, source, 4);
        memcpy(destination, &word, 4);
        source += 4;
        destination += 4;
        size -= 4;
    }
    while (size != 0) {
        *destination++ = *source++;
        size--;
    }
}
#define SNAPSHOT_COPY CopySnapshotBytes
#else
#define SNAPSHOT_COPY memcpy
#endif

/* The room for the message of an overflow. */
#define OVERFLOW_MESSAGE_SIZE 0x50

#if TGR_PLATFORM != TGR_PLATFORM_WIN && (TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU)
/* The marker of a keyframe in a recording of the input. */
#define KEYFRAME_MARKER 0xFF

/* The snapshots copy the variables themselves, at an offset of 0. */
#define SERIALIZE(fields, buffer, capacity) SerializeSaveData(fields, buffer, capacity, 0)
#define DESERIALIZE(fields, buffer) DeserializeSaveData(fields, buffer, 0)

s32 SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity, s32 offset) {
    s32 i;
    u8 *start = buffer;
    char message[OVERFLOW_MESSAGE_SIZE];

    for (i = 0; fields[i].data != NULL; i++) {
        SNAPSHOT_COPY(buffer, (u8 *)fields[i].data + offset, fields[i].size);
        buffer += fields[i].size;
    }
    if (capacity < buffer - start) {
        sprintf(message, "VAR SAVE OVERFLOW (%d avail, %d used)", capacity, buffer - start);
        SetDebugMessage(message);
    }
    return buffer - start;
}

s32 DeserializeSaveData(SaveField *fields, u8 *buffer, s32 offset) {
    s32 i;
    u8 *start = buffer;

    for (i = 0; fields[i].data != NULL; i++) {
        SNAPSHOT_COPY((u8 *)fields[i].data + offset, buffer, fields[i].size);
        buffer += fields[i].size;
    }
    return buffer - start;
}
#else
#define SERIALIZE(fields, buffer, capacity) SerializeSaveData(fields, buffer, capacity)
#define DESERIALIZE(fields, buffer) DeserializeSaveData(fields, buffer)

void SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity) {
    s32 i;
    u8 *start = buffer;
    char message[OVERFLOW_MESSAGE_SIZE];

    for (i = 0; fields[i].data != NULL; i++) {
        SNAPSHOT_COPY(buffer, fields[i].data, fields[i].size);
        buffer += fields[i].size;
    }
    if (capacity < buffer - start) {
        sprintf(message, "VAR SAVE OVERFLOW (%d avail, %d used)", capacity, buffer - start);
        SetDebugMessage(message);
    }
}

void DeserializeSaveData(SaveField *fields, u8 *buffer) {
    s32 i;

    for (i = 0; fields[i].data != NULL; i++) {
        SNAPSHOT_COPY(fields[i].data, buffer, fields[i].size);
        buffer += fields[i].size;
    }
}
#endif

void SaveRaceSnapshot(LastLapBuffer *buffer) {
/* The capacity given is that of the whole buffer, not of the room after the recordings. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    SERIALIZE(g_aRaceSnapshotFields, buffer->snapshot, 0x15f88);
#else
    SERIALIZE(g_aRaceSnapshotFields, buffer->snapshot, sizeof(LastLapBuffer));
#endif
}

void RestoreRaceSnapshot(LastLapBuffer *buffer) {
    DESERIALIZE(g_aRaceSnapshotFields, buffer->snapshot);
}

void SaveClockForPause(void) {
    SERIALIZE(g_aPausedClockFields, g_abPausedClockState, PAUSED_CLOCK_STATE_SIZE);
}

void RestoreClockAfterPause(void) {
    DESERIALIZE(g_aPausedClockFields, g_abPausedClockState);
}
