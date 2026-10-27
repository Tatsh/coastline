#ifndef COASTLINE_EAR_DIRECTIONALMATRIX_H
#define COASTLINE_EAR_DIRECTIONALMATRIX_H

#include <stdint.h>

typedef struct [[gnu::packed]] EarDirectionalMatrixEntry {
    int32_t pan;
    float gain;
} EarDirectionalMatrixEntry;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress EAR: 0x100046ee
 * @stub
 */
void __stdcall
CopyDirectionalMatrixEntry(uint16_t matrix, uint16_t direction, EarDirectionalMatrixEntry *entry);

/**
 * @ghidraAddress EAR: 0x1000ebd0
 * @stub
 */
int __stdcall EarRegisterMatrixExportWrapper(uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
