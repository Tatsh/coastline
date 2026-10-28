#ifndef COASTLINE_EAR_DIRECTIONALMATRIX_H
#define COASTLINE_EAR_DIRECTIONALMATRIX_H

#include <stdint.h>

/** The number of entries of a directional matrix. */
#define EAR_DIRECTIONAL_MATRIX_ENTRY_COUNT 0x80

/** The pan and the gain of one direction of a directional matrix. */
typedef struct [[gnu::packed]] EarDirectionalMatrixEntry {
#ifdef __cplusplus
    /**
     * Set the pan and the gain of the entry.
     *
     * @param newPan The pan.
     * @param newGain The gain.
     * @ghidraAddress EAR: 0x100124b0
     */
    void SetPresetEntry(int32_t newPan, float newGain) {
        pan = newPan;
        gain = newGain;
    }
#endif
    int32_t pan; /*!< The pan, from -10000 on the left to 10000 on the right. */
    float gain;  /*!< The gain, from 0 to 1. */
} EarDirectionalMatrixEntry;

#ifdef __cplusplus
/** A directional matrix, the pan and the gain of each direction of an event. */
class [[gnu::packed]] EarPresetTable {
public:
    /** @ghidraAddress EAR: 0x10010640 */
    EarPresetTable();

    /**
     * Give every direction a pan and a gain.
     *
     * @param pan The pan.
     * @param gain The gain.
     * @ghidraAddress EAR: 0x10010666
     */
    void FillPresetTable(int32_t pan, float gain);
    /**
     * Give a direction a pan and a gain.
     *
     * @param direction The direction.
     * @param pan The pan.
     * @param gain The gain.
     * @ghidraAddress EAR: 0x100106b8
     */
    void SetPresetTableEntry(uint16_t direction, int32_t pan, float gain);
    /**
     * Copy the pan and the gain of one direction to another.
     *
     * @param destination The direction that takes the entry.
     * @param source The direction whose entry it takes.
     * @ghidraAddress EAR: 0x100106e1
     */
    void CopyPresetTableEntry(uint16_t destination, uint16_t source);
    /**
     * @param newMode The mode of the matrix.
     * @ghidraAddress EAR: 0x100124d0
     */
    void SetPresetTableMode(uint16_t newMode) {
        mode = newMode;
    }
    /**
     * @param direction The direction.
     * @return The pan of the direction.
     * @ghidraAddress EAR: 0x100124f0
     */
    int32_t GetPresetEntryInteger(uint16_t direction) const {
        return entries[direction].pan;
    }
    /**
     * @param direction The direction.
     * @return The gain of the direction.
     * @ghidraAddress EAR: 0x10012510
     */
    float GetPresetEntryFloat(uint16_t direction) const {
        return entries[direction].gain;
    }
    /**
     * Copy the entry of a direction.
     *
     * @param direction The direction.
     * @param entry Receives the entry.
     * @ghidraAddress EAR: 0x100072e0
     */
    void CopyDirectionalMatrixRow(uint16_t direction, EarDirectionalMatrixEntry *entry) const;

private:
    EarDirectionalMatrixEntry entries[EAR_DIRECTIONAL_MATRIX_ENTRY_COUNT]; /*!< By direction. */
    uint16_t mode; /*!< The directions of a built-in matrix, which is also its handle. */
};

extern "C" {
#endif

/**
 * Copy the entry of a direction of a matrix, or an empty entry for a direction or a matrix that is
 * not registered.
 *
 * @param matrix The matrix.
 * @param direction The direction.
 * @param entry Receives the entry.
 * @ghidraAddress EAR: 0x100046ee
 */
void __stdcall
CopyDirectionalMatrixEntry(uint16_t matrix, uint16_t direction, EarDirectionalMatrixEntry *entry);

/**
 * Copy the matrices into a larger hash table once the table holds one matrix a bucket.
 *
 * @ghidraAddress EAR: 0x100049cc
 */
void __stdcall GrowMatrixRegistry(void);

/**
 * @param flags The built-in matrices.
 * @return Whether the matrices were registered.
 * @ghidraAddress EAR: 0x1000ebd0
 */
int __stdcall EarRegisterMatrixExportWrapper(uint32_t flags);

/**
 * Register the built-in matrices the flags select: 0x1, 0x4, 0x10, 0x20, and 0x40 for the
 * matrices of 1, 4, 16, 32, and 64 directions.
 *
 * @param flags The built-in matrices, from 1 to 0x3ff.
 * @return Whether the matrices were registered.
 * @ghidraAddress EAR: 0x1000ebe0
 */
int __stdcall EAR_DLL_RegisterMatrix(uint32_t flags);

/**
 * @param matrix The matrix, or 0 for every matrix.
 * @return Whether the matrix was shut down.
 * @ghidraAddress EAR: 0x1000f5f0
 */
int __stdcall EarShutDownMatrixExportWrapper(uint32_t matrix);

/**
 * Shut down a directional matrix, or every matrix.
 *
 * @param matrix The matrix, or 0 for every matrix.
 * @return Whether the matrix was shut down.
 * @ghidraAddress EAR: 0x1000f600
 */
int __stdcall EAR_DLL_ShutDownMatrix(uint32_t matrix);

/**
 * Register the built-in matrix of one direction as matrix 1.
 *
 * @return Whether the matrix is registered.
 * @ghidraAddress EAR: 0x10010716
 */
int __stdcall RegisterBuiltinDirectionalMatrix1(void);

/**
 * @return Whether the matrix is registered.
 * @ghidraAddress EAR: 0x10010992
 */
int __stdcall RegisterBuiltinDirectionalMatrix4(void);

/**
 * @return Whether the matrix is registered.
 * @ghidraAddress EAR: 0x10010d4e
 */
int __stdcall RegisterBuiltinDirectionalMatrix16(void);

/**
 * @return Whether the matrix is registered.
 * @ghidraAddress EAR: 0x100110ee
 */
int __stdcall RegisterBuiltinDirectionalMatrix32(void);

/**
 * @return Whether the matrix is registered.
 * @ghidraAddress EAR: 0x10011a55
 */
int __stdcall RegisterBuiltinDirectionalMatrix64(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_DIRECTIONALMATRIX_H */
