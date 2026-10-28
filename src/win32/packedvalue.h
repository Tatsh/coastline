#ifndef COASTLINE_PLATFORM_WIN_PACKEDVALUE_H
#define COASTLINE_PLATFORM_WIN_PACKEDVALUE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Round 128 times a value to the nearest integer, limited to the range of a signed byte.
 *
 * @param dValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x10030de0
 * @ghidraAddress BRGlide: 0x1002a490
 */
int ConvertDoubleToSignedByte(double dValue);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_PACKEDVALUE_H */
