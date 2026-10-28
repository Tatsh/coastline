#include "directionalmatrix.h"

#include <cstring>
#include <new>

#include "error.h"
#include "mfc.h"
#include "registry.h"
#include "runtime.h"
#include "volume.h"

/** The pan of the left side. */
#define EAR_PAN_LEFT (-10000)
/** The pan of the right side. */
#define EAR_PAN_RIGHT 10000

// @ghidraAddress EAR: 0x1002e6e0
static const EarDirectionalMatrixEntry g_earEmptyDirectionalMatrixEntry = {0, 0};
// @ghidraAddress EAR: 0x1002edd8
static const float g_earDirectionalGainUnity = 1;
// @ghidraAddress EAR: 0x1002ede0
static const float g_earDirectionalMatrixGain083 = 0.83f;
// @ghidraAddress EAR: 0x1002ede4
static const float g_earDirectionalMatrixGain060 = 0.6f;
// @ghidraAddress EAR: 0x1002ede8
static const float g_earDirectionalMatrixGain045 = 0.45f;
// @ghidraAddress EAR: 0x1002edec
static const float g_earDirectionalInterpolationDivisorTwo = 2;

/** The gain percentage of the pans between the middle and the sides. */
#define EAR_HALF_PAN_GAIN_PERCENT 50.0

// @ghidraAddress EAR: 0x1002edf0
static const float g_earDirectionalInterpolationDivisorSixteen = 16;
// @ghidraAddress EAR: 0x1002edf8
static const double g_earDirectionalInterpolationDivisorEight = 8;
// @ghidraAddress EAR: 0x1002ee00
static const float g_earDirectionalInterpolationDivisorFive = 5;
// @ghidraAddress EAR: 0x1002ee04
static const float g_earDirectionalInterpolationDivisorThreeFloat = 3;
// @ghidraAddress EAR: 0x1002ee08
static const double g_earDirectionalInterpolationDivisorThree = 3;
// @ghidraAddress EAR: 0x1002eddc
static const float g_earLargeDirectionalMatrixGain083 = 0.83f;

/** The step of the gain between two gains over a number of directions. */
static double DirectionalGainStep(long double upper, long double lower, long double directions) {
    return (double)((upper - lower) / directions);
}

/** A gain a number of steps below or above another, rounded to a float as the original pushes it.
 */
static float StepDirectionalGain(float gain, uint32_t steps, double step, int below) {
    long double offset = (long double)(uint64_t)steps * step;
    return (float)(below ? gain - offset : gain + offset);
}

/** The pan percentage a number of steps from the middle. */
static double StepPanPercent(uint32_t steps, double step) {
    return (double)((long double)(uint64_t)steps * step);
}

/** The gain halfway between two gains, through the x87 precision of the original. */
static float InterpolateDirectionalGain(float upper, float lower) {
    return (float)(((long double)upper - lower) / g_earDirectionalInterpolationDivisorTwo + lower);
}

EarPresetTable::EarPresetTable() {
    mode = 0;
    FillPresetTable(0, 0);
}

void EarPresetTable::FillPresetTable(int32_t pan, float gain) {
    for (uint16_t direction = 0; direction < EAR_DIRECTIONAL_MATRIX_ENTRY_COUNT; ++direction) {
        entries[direction].SetPresetEntry(pan, gain);
    }
}

void EarPresetTable::SetPresetTableEntry(uint16_t direction, int32_t pan, float gain) {
    entries[direction].SetPresetEntry(pan, gain);
}

void EarPresetTable::CopyPresetTableEntry(uint16_t destination, uint16_t source) {
    std::memmove(&entries[destination], &entries[source], sizeof(EarDirectionalMatrixEntry));
}

void EarPresetTable::CopyDirectionalMatrixRow(uint16_t direction,
                                              EarDirectionalMatrixEntry *entry) const {
    std::memmove(entry, &entries[direction], sizeof(EarDirectionalMatrixEntry));
}

void __stdcall
CopyDirectionalMatrixEntry(uint16_t matrix, uint16_t direction, EarDirectionalMatrixEntry *entry) {
    if (direction >= EAR_DIRECTIONAL_MATRIX_ENTRY_COUNT) {
        std::memmove(entry, &g_earEmptyDirectionalMatrixEntry, sizeof(*entry));
        return;
    }
    if (g_MatrixRegistry.IsEmpty()) {
        std::memmove(entry, &g_earEmptyDirectionalMatrixEntry, sizeof(*entry));
        return;
    }
    EarPresetTable *table;
    if (!g_MatrixRegistry.LookupWordPointerMapValue(matrix, table)) {
        std::memmove(entry, &g_earEmptyDirectionalMatrixEntry, sizeof(*entry));
        return;
    }
    table->CopyDirectionalMatrixRow(direction, entry);
}

void __stdcall GrowMatrixRegistry(void) {
    g_MatrixRegistry.GrowWhenFull();
}

/**
 * Allocate a matrix for a built-in matrix that is not registered yet.
 *
 * @param handle The handle of the built-in matrix.
 * @param table Receives the matrix, or nullptr when the matrix is registered or none is allocated.
 * @return Whether the matrix is registered already.
 */
static int AllocateBuiltinDirectionalMatrix(uint16_t handle, EarPresetTable *&table) {
    if (g_MatrixRegistry.LookupWordPointerMapValue(handle, table)) {
        SetEarLastError(0x16);
        return 1;
    }
    GrowMatrixRegistry();
    void *storage = MfcAllocateWithNewHandler(sizeof(EarPresetTable));
    table = storage ? new (storage) EarPresetTable : nullptr;
    if (table == nullptr) {
        SetEarLastError(0x10);
    }
    return 0;
}

int __stdcall RegisterBuiltinDirectionalMatrix1(void) {
    EarPresetTable *table;
    if (AllocateBuiltinDirectionalMatrix(1, table)) {
        return 1;
    }
    if (table == nullptr) {
        return 0;
    }
    table->FillPresetTable(0, 0);
    table->SetPresetTableEntry(0, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(1, EAR_PAN_LEFT, g_earDirectionalMatrixGain060);
    table->SetPresetTableEntry(2, EAR_PAN_LEFT, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(3, EAR_PAN_LEFT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(4, 0, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(5, EAR_PAN_RIGHT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(6, EAR_PAN_RIGHT, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(7, EAR_PAN_RIGHT, g_earDirectionalMatrixGain060);
    table->SetPresetTableEntry(8, 0, g_earDirectionalMatrixGain045);
    table->SetPresetTableEntry(9, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(10, 0, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(11, EAR_PAN_LEFT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(12, EAR_PAN_RIGHT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(13, EAR_PAN_LEFT / 2, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(14, EAR_PAN_RIGHT / 2, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(15, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(16, 0, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(17, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(18, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(19, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(20, 0, g_earDirectionalGainUnity);
    table->SetPresetTableMode(1);
    g_MatrixRegistry.SetWordPointerMapValue(1, table);
    return 1;
}

int __stdcall RegisterBuiltinDirectionalMatrix4(void) {
    EarPresetTable *table;
    if (AllocateBuiltinDirectionalMatrix(4, table)) {
        return 1;
    }
    if (table == nullptr) {
        return 0;
    }
    table->FillPresetTable(0, 0);
    table->SetPresetTableEntry(4, EAR_PAN_LEFT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(
        9,
        ConvertPercentGainToLeftPan(EAR_HALF_PAN_GAIN_PERCENT),
        InterpolateDirectionalGain(g_earDirectionalGainUnity, g_earDirectionalMatrixGain083));
    table->SetPresetTableEntry(14, 0, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(
        19, ConvertPercentGainToRightPan(EAR_HALF_PAN_GAIN_PERCENT), table->GetPresetEntryFloat(9));
    table->SetPresetTableEntry(24, EAR_PAN_RIGHT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(3, EAR_PAN_LEFT, g_earDirectionalGainUnity);
    // The half step is stored as a float before the second read of the entry adds it.
    float halfStep =
        (float)(((long double)g_earDirectionalGainUnity - table->GetPresetEntryFloat(9)) /
                g_earDirectionalInterpolationDivisorTwo);
    table->SetPresetTableEntry(8,
                               ConvertPercentGainToLeftPan(EAR_HALF_PAN_GAIN_PERCENT),
                               (float)((long double)table->GetPresetEntryFloat(9) + halfStep));
    table->SetPresetTableEntry(13, 0, table->GetPresetEntryFloat(9));
    table->SetPresetTableEntry(
        18, ConvertPercentGainToRightPan(EAR_HALF_PAN_GAIN_PERCENT), table->GetPresetEntryFloat(8));
    table->SetPresetTableEntry(23, EAR_PAN_RIGHT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(2, EAR_PAN_LEFT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(
        7, ConvertPercentGainToLeftPan(EAR_HALF_PAN_GAIN_PERCENT), g_earDirectionalGainUnity);
    table->SetPresetTableEntry(12, 0, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(
        17, ConvertPercentGainToRightPan(EAR_HALF_PAN_GAIN_PERCENT), g_earDirectionalGainUnity);
    table->SetPresetTableEntry(22, EAR_PAN_RIGHT, g_earDirectionalGainUnity);
    table->SetPresetTableEntry(1, EAR_PAN_LEFT, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(
        6,
        ConvertPercentGainToLeftPan(EAR_HALF_PAN_GAIN_PERCENT),
        InterpolateDirectionalGain(g_earDirectionalMatrixGain083, g_earDirectionalMatrixGain060));
    table->SetPresetTableEntry(11, 0, g_earDirectionalMatrixGain060);
    table->SetPresetTableEntry(
        16, ConvertPercentGainToRightPan(EAR_HALF_PAN_GAIN_PERCENT), table->GetPresetEntryFloat(6));
    table->SetPresetTableEntry(21, EAR_PAN_RIGHT, g_earDirectionalMatrixGain083);
    table->SetPresetTableEntry(0, EAR_PAN_LEFT, g_earDirectionalMatrixGain060);
    table->SetPresetTableEntry(
        5,
        ConvertPercentGainToLeftPan(EAR_HALF_PAN_GAIN_PERCENT),
        InterpolateDirectionalGain(g_earDirectionalMatrixGain060, g_earDirectionalMatrixGain045));
    table->SetPresetTableEntry(10, 0, g_earDirectionalMatrixGain045);
    table->SetPresetTableEntry(
        15, ConvertPercentGainToRightPan(EAR_HALF_PAN_GAIN_PERCENT), table->GetPresetEntryFloat(5));
    table->SetPresetTableEntry(20, EAR_PAN_RIGHT, g_earDirectionalMatrixGain060);
    table->SetPresetTableMode(4);
    g_MatrixRegistry.SetWordPointerMapValue(4, table);
    return 1;
}

int __stdcall RegisterBuiltinDirectionalMatrix16(void) {
    EarPresetTable *table;
    if (AllocateBuiltinDirectionalMatrix(0x10, table)) {
        return 1;
    }
    if (table == nullptr) {
        return 0;
    }
    table->FillPresetTable(0, 0);
    double panStep = 6.25;
    for (uint32_t direction = 0; direction <= 0x10; ++direction) {
        double step = DirectionalGainStep(g_earDirectionalGainUnity,
                                          g_earDirectionalMatrixGain083,
                                          g_earDirectionalInterpolationDivisorSixteen);
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x30),
            ConvertPercentGainToLeftPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x50 - direction),
            ConvertPercentGainToRightPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        step = DirectionalGainStep(g_earDirectionalMatrixGain060,
                                   g_earDirectionalMatrixGain045,
                                   g_earDirectionalInterpolationDivisorSixteen);
        table->SetPresetTableEntry(
            static_cast<uint16_t>((direction + 0x70) % EAR_DIRECTIONAL_MATRIX_ENTRY_COUNT),
            ConvertPercentGainToRightPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 1));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x10 - direction),
            ConvertPercentGainToLeftPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 1));
        step = DirectionalGainStep(g_earDirectionalGainUnity,
                                   g_earDirectionalMatrixGain083,
                                   g_earDirectionalInterpolationDivisorSixteen);
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x30 - direction),
            EAR_PAN_LEFT,
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        // The gains towards the back climb from 0.6 by the step between 1 and 0.83.
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x10),
            EAR_PAN_LEFT,
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 0));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x50),
            EAR_PAN_RIGHT,
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x70 - direction),
            EAR_PAN_RIGHT,
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 0));
    }
    table->SetPresetTableMode(0x10);
    g_MatrixRegistry.SetWordPointerMapValue(0x10, table);
    return 1;
}

/**
 * Step the entries on both sides of two directions from their own gains by a step, keeping their
 * pans.
 */
static void StepDirectionalEntries(EarPresetTable *table,
                                   uint16_t descending,
                                   uint16_t ascending,
                                   uint32_t count,
                                   double step,
                                   int below) {
    for (uint32_t direction = 0; direction <= count; ++direction) {
        table->SetPresetTableEntry(
            static_cast<uint16_t>(descending - direction),
            table->GetPresetEntryInteger(descending),
            StepDirectionalGain(table->GetPresetEntryFloat(descending), direction, step, below));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(ascending + direction),
            table->GetPresetEntryInteger(ascending),
            StepDirectionalGain(table->GetPresetEntryFloat(ascending), direction, step, below));
    }
}

/** Build the directions the matrices of 32 and 64 directions share, from the sides inwards. */
static void FillLargeDirectionalMatrix(EarPresetTable *table) {
    table->FillPresetTable(0, 0);
    double panStep = 12.5;
    uint32_t direction;
    for (direction = 0; direction < 9; ++direction) {
        double step = DirectionalGainStep(g_earDirectionalGainUnity,
                                          g_earDirectionalMatrixGain083,
                                          g_earDirectionalInterpolationDivisorEight);
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x54),
            ConvertPercentGainToLeftPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x64 - direction),
            ConvertPercentGainToRightPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        step = DirectionalGainStep(g_earDirectionalMatrixGain060,
                                   g_earDirectionalMatrixGain045,
                                   g_earDirectionalInterpolationDivisorEight);
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x74),
            ConvertPercentGainToRightPan(StepPanPercent(direction, panStep)),
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 1));
        if (direction < 8) {
            table->SetPresetTableEntry(
                static_cast<uint16_t>(0x44 - direction),
                ConvertPercentGainToLeftPan(StepPanPercent(direction, panStep)),
                StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 1));
        }
        step = DirectionalGainStep(g_earDirectionalGainUnity,
                                   g_earDirectionalMatrixGain083,
                                   g_earDirectionalInterpolationDivisorEight);
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x54 - direction),
            EAR_PAN_LEFT,
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x44),
            EAR_PAN_LEFT,
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 0));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(direction + 0x64),
            EAR_PAN_RIGHT,
            StepDirectionalGain(g_earDirectionalGainUnity, direction, step, 1));
        table->SetPresetTableEntry(
            static_cast<uint16_t>(0x74 - direction),
            EAR_PAN_RIGHT,
            StepDirectionalGain(g_earDirectionalMatrixGain060, direction, step, 0));
    }
    for (direction = 0; direction <= 8; ++direction) {
        table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0x28),
                                    static_cast<uint16_t>(direction + 0x58));
        if (direction + 0x38 < 0x3d) {
            table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0x38),
                                        static_cast<uint16_t>(direction + 0x78));
        } else {
            table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0x18),
                                        static_cast<uint16_t>(direction + 0x38));
        }
    }
    StepDirectionalEntries(table,
                           0x28,
                           0x30,
                           4,
                           DirectionalGainStep(table->GetPresetEntryFloat(0x28),
                                               g_earDirectionalMatrixGain083,
                                               g_earDirectionalInterpolationDivisorFive),
                           1);
    StepDirectionalEntries(table,
                           0x38,
                           0x20,
                           4,
                           DirectionalGainStep(g_earDirectionalMatrixGain083,
                                               table->GetPresetEntryFloat(0x38),
                                               g_earDirectionalInterpolationDivisorFive),
                           0);
    for (direction = 0; direction <= 4; ++direction) {
        table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0x12),
                                    static_cast<uint16_t>(direction + 0x2a));
        if (direction + 0x1a < 0x1d) {
            table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0x1a),
                                        static_cast<uint16_t>(direction + 0x3a));
        } else {
            table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0xa),
                                        static_cast<uint16_t>(direction + 0x1a));
        }
    }
    StepDirectionalEntries(table,
                           0x12,
                           0x16,
                           2,
                           DirectionalGainStep(table->GetPresetEntryFloat(0x12),
                                               g_earDirectionalMatrixGain083,
                                               g_earDirectionalInterpolationDivisorThreeFloat),
                           1);
    StepDirectionalEntries(table,
                           0x1a,
                           0xe,
                           2,
                           DirectionalGainStep(g_earDirectionalMatrixGain083,
                                               table->GetPresetEntryFloat(0x1a),
                                               g_earDirectionalInterpolationDivisorThreeFloat),
                           0);
    for (direction = 0; direction <= 2; ++direction) {
        table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 7),
                                    static_cast<uint16_t>(direction + 0x13));
        if (direction + 0xb == 0xd) {
            table->CopyPresetTableEntry(5, 0xd);
        } else {
            table->CopyPresetTableEntry(static_cast<uint16_t>(direction + 0xb),
                                        static_cast<uint16_t>(direction + 0x1b));
        }
    }
    float front = table->GetPresetEntryFloat(7);
    double step = DirectionalGainStep(
        front, table->GetPresetEntryFloat(5), g_earDirectionalInterpolationDivisorTwo);
    table->SetPresetTableEntry(
        6, table->GetPresetEntryInteger(7), (float)(table->GetPresetEntryFloat(7) - step));
    table->SetPresetTableEntry(
        0xa, table->GetPresetEntryInteger(9), (float)(table->GetPresetEntryFloat(9) - step));
}

int __stdcall RegisterBuiltinDirectionalMatrix32(void) {
    EarPresetTable *table;
    if (AllocateBuiltinDirectionalMatrix(0x20, table)) {
        return 1;
    }
    if (table == nullptr) {
        return 0;
    }
    FillLargeDirectionalMatrix(table);
    table->CopyPresetTableEntry(1, 5);
    table->CopyPresetTableEntry(2, 7);
    table->CopyPresetTableEntry(3, 9);
    table->CopyPresetTableEntry(4, 0xb);
    table->SetPresetTableEntry(0, 0, g_earLargeDirectionalMatrixGain083);
    for (uint32_t direction = 0x7f; direction > 0x7c; --direction) {
        double backStep = DirectionalGainStep(g_earDirectionalGainUnity,
                                              g_earLargeDirectionalMatrixGain083,
                                              g_earDirectionalInterpolationDivisorThree);
        long double gain = g_earLargeDirectionalMatrixGain083 +
                           (long double)(uint64_t)(0x80 - direction) * backStep;
        // The original records the gain as a double it never reads.
        [[maybe_unused]] double recordedGain = (double)gain;
        table->SetPresetTableEntry(static_cast<uint16_t>(direction), 0, (float)gain);
    }
    table->SetPresetTableMode(0x20);
    g_MatrixRegistry.SetWordPointerMapValue(0x20, table);
    return 1;
}

int __stdcall RegisterBuiltinDirectionalMatrix64(void) {
    EarPresetTable *table;
    if (AllocateBuiltinDirectionalMatrix(0x40, table)) {
        return 1;
    }
    if (table == nullptr) {
        return 0;
    }
    FillLargeDirectionalMatrix(table);
    table->SetPresetTableEntry(
        1, table->GetPresetEntryInteger(5), g_earLargeDirectionalMatrixGain083);
    table->SetPresetTableEntry(
        2, table->GetPresetEntryInteger(7), g_earLargeDirectionalMatrixGain083);
    table->SetPresetTableEntry(3, table->GetPresetEntryInteger(9), g_earDirectionalMatrixGain060);
    table->SetPresetTableEntry(4, table->GetPresetEntryInteger(0xb), g_earDirectionalMatrixGain060);
    table->SetPresetTableEntry(0, 0, g_earDirectionalGainUnity);
    for (uint32_t direction = 0x7f; direction > 0x7c; --direction) {
        double backStep = DirectionalGainStep(g_earDirectionalGainUnity,
                                              g_earLargeDirectionalMatrixGain083,
                                              g_earDirectionalInterpolationDivisorThree);
        long double gain =
            g_earDirectionalGainUnity - (long double)(uint64_t)(0x80 - direction) * backStep;
        // The original records the gain as a double it never reads.
        [[maybe_unused]] double recordedGain = (double)gain;
        table->SetPresetTableEntry(static_cast<uint16_t>(direction), 0, (float)gain);
    }
    table->SetPresetTableMode(0x40);
    g_MatrixRegistry.SetWordPointerMapValue(0x40, table);
    return 1;
}

int __stdcall EarRegisterMatrixExportWrapper(uint32_t flags) {
    return EAR_DLL_RegisterMatrix(flags);
}

int __stdcall EAR_DLL_RegisterMatrix(uint32_t flags) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (flags == 0) {
        SetEarLastError(0x15);
        return 0;
    }
    if (flags >= 0x400) {
        SetEarLastError(0x17);
        return 0;
    }
    if ((flags & 1) && !RegisterBuiltinDirectionalMatrix1()) {
        return 0;
    }
    if ((flags & 4) && !RegisterBuiltinDirectionalMatrix4()) {
        return 0;
    }
    if ((flags & 0x10) && !RegisterBuiltinDirectionalMatrix16()) {
        return 0;
    }
    if ((flags & 0x20) && !RegisterBuiltinDirectionalMatrix32()) {
        return 0;
    }
    if ((flags & 0x40) && !RegisterBuiltinDirectionalMatrix64()) {
        return 0;
    }
    return 1;
}

int __stdcall EarShutDownMatrixExportWrapper(uint32_t matrix) {
    return EAR_DLL_ShutDownMatrix(matrix);
}

int __stdcall EAR_DLL_ShutDownMatrix(uint32_t matrix) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return 0;
    }
    if (g_MatrixRegistry.IsEmpty()) {
        if (matrix != 0) {
            SetEarLastError(0x27);
        }
        return 0;
    }
    int locked = 0;
    while (g_bPlaybackUpdateLocked) {
    }
    if (!g_bRegistryMutationLocked) {
        locked = 1;
        g_bRegistryMutationLocked = locked;
    }
    if (matrix != 0) {
        EarPresetTable *found;
        if (g_MatrixRegistry.LookupWordPointerMapValue(static_cast<uint16_t>(matrix), found)) {
            ::operator delete(found);
            g_MatrixRegistry.RemoveKey(static_cast<uint16_t>(matrix));
        } else {
            SetEarLastError(0x27);
        }
    } else {
        POSITION position = g_MatrixRegistry.GetStartPosition();
        do {
            WORD key;
            void *value;
            g_MatrixRegistry.GetNextAssoc(position, key, value);
            ::operator delete(value);
        } while (position);
        g_MatrixRegistry.RemoveAll();
    }
    if (locked) {
        g_bRegistryMutationLocked = 0;
    }
    return 1;
}
