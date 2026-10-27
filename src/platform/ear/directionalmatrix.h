#ifndef COASTLINE_EAR_DIRECTIONALMATRIX_H
#define COASTLINE_EAR_DIRECTIONALMATRIX_H

#include <cstdint>

struct [[gnu::packed]] EarDirectionalMatrixEntry {
    std::int32_t pan;
    float gain;
};

/**
 * @ghidraAddress EAR: 0x100046ee
 * @stub
 */
void __stdcall CopyDirectionalMatrixEntry(std::uint16_t matrix,
                                          std::uint16_t direction,
                                          EarDirectionalMatrixEntry *entry);

#endif
