#ifndef COASTLINE_PLATFORM_WIN_PACKEDVALUE_H
#define COASTLINE_PLATFORM_WIN_PACKEDVALUE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Round -128 times a value to the nearest integer, limited to the range of a field of six bits.
 *
 * @param fValue The value.
 * @return The field, from -32 to 31.
 * @ghidraAddress BRD3D: 0x100065a0
 * @ghidraAddress BRGlide: 0x10006910
 */
int FloatToSignedByteIndex(float fValue);

/**
 * Round -32768 times a value to the nearest integer, limited to the range of a short.
 *
 * @param fValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x100065e0
 * @ghidraAddress BRGlide: 0x10006950
 */
int ConvertFloatToFixedAngle(float fValue);

/**
 * Quantise a coordinate in steps of 1/8192 within the unsigned 24-bit range.
 *
 * @param fValue Coordinate.
 * @return Quantised coordinate.
 * @ghidraAddress BRD3D: 0x100066e0
 * @ghidraAddress BRGlide: 0x10006a50
 */
int ConvertFloatToFixedAngleNoClamp(float fValue);

/**
 * Round a value divided by 1.41015625 to the nearest integer, limited to the range of an unsigned
 * byte.
 *
 * @param fValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x10006620
 * @ghidraAddress BRGlide: 0x10006990
 */
int FloatToByteIndex(float fValue);

/**
 * Map engine speed from 400 to 8000 RPM to the nearest of 64 steps.
 *
 * @param fValue Engine speed, in revolutions per minute.
 * @return The step, from 0 to 63.
 * @ghidraAddress BRD3D: 0x10006660
 * @ghidraAddress BRGlide: 0x100069d0
 */
int FloatToEngineRpmIndex(float fValue);

/**
 * Round twice a value to the nearest integer, limited to the range of a field of 24 bits.
 *
 * @param fValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x10006730
 * @ghidraAddress BRGlide: 0x10006aa0
 */
int ConvertFloatToFixed24(float fValue);

/**
 * Round 128 times a value to the nearest integer, limited to the range of a short.
 *
 * @param fValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x10006770
 * @ghidraAddress BRGlide: 0x10006ae0
 */
int ConvertFloatToFixedAngle16B(float fValue);

/**
 * Round 256 times a value to the nearest integer, limited to the range of a short.
 *
 * @param fValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x100067b0
 * @ghidraAddress BRGlide: 0x10006b20
 */
int FloatToSignedShortIndex(float fValue);

/**
 * Round 8 times a value to the nearest integer, limited to the range of a signed byte.
 *
 * @param fValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x100067f0
 * @ghidraAddress BRGlide: 0x10006b60
 */
int ConvertFloatToSignedByte(float fValue);

/**
 * Get the light level of a brightness, the inverse of GetLightLevelScale().
 *
 * @param fScale The brightness.
 * @return 0 below 128, 1 below 171, 2 below 213, and 3 otherwise.
 * @ghidraAddress BRD3D: 0x100066a0
 * @ghidraAddress BRGlide: 0x10006a10
 */
unsigned char QuantizeFloatToLightIndex(float fScale);

/**
 * Round 128 times a value to the nearest integer, limited to the range of a signed byte.
 *
 * @param dValue The value.
 * @return The field.
 * @ghidraAddress BRD3D: 0x10030de0
 * @ghidraAddress BRGlide: 0x1002a490
 */
int ConvertDoubleToSignedByte(double dValue);

/**
 * Scale a signed field of six bits by -1/128.
 *
 * @param bValue The field in the low six bits.
 * @return The value.
 * @ghidraAddress BRD3D: 0x10007250
 * @ghidraAddress BRGlide: 0x100075c0
 */
float SignedByteToFloat(unsigned char bValue);

/**
 * Map one of 64 steps to engine speed from 400 to 8000 RPM.
 *
 * @param bIndex The step.
 * @return Engine speed, in revolutions per minute.
 * @ghidraAddress BRD3D: 0x100072c0
 * @ghidraAddress BRGlide: 0x10007630
 */
float EngineRpmIndexToFloat(unsigned char bIndex);

/**
 * Get the brightness of a light level.
 *
 * @param nLightLevel The level, from 0 to 3.
 * @return 0, 170, 212, or 255.
 * @ghidraAddress BRD3D: 0x100072e0
 * @ghidraAddress BRGlide: 0x10007650
 */
float GetLightLevelScale(char nLightLevel);

/**
 * Scale a field by 1/8192.
 *
 * @param dwValue The field.
 * @return The value.
 * @ghidraAddress BRD3D: 0x10007310
 * @ghidraAddress BRGlide: 0x10007680
 */
float ConvertUintToScaledFloat(unsigned dwValue);

/**
 * Scale a signed field of 24 bits by 1/2.
 *
 * @param nValue The field in the low 24 bits.
 * @return The value.
 * @ghidraAddress BRD3D: 0x10007340
 * @ghidraAddress BRGlide: 0x100076b0
 */
float Signed24BitToFloat(int nValue);

/**
 * Scale a field of a packet of two bytes by -1/32768.
 *
 * @param nValue The field.
 * @return The value.
 * @ghidraAddress BRD3D: 0x10007280
 * @ghidraAddress BRGlide: 0x100075f0
 */
float ConvertFixedAngleToFloat(short nValue);

/**
 * Scale a field of a packet of one unsigned byte by 1.41015625.
 *
 * @param bValue The field.
 * @return The value.
 * @ghidraAddress BRD3D: 0x100072a0
 * @ghidraAddress BRGlide: 0x10007610
 */
float ByteToFloat(unsigned char bValue);

/**
 * Scale a field of a packet of two bytes by 1/128.
 *
 * @param nValue The field.
 * @return The value.
 * @ghidraAddress BRD3D: 0x10007380
 * @ghidraAddress BRGlide: 0x100076f0
 */
float FixedToRadians(short nValue);

/**
 * Scale a field of a packet of two bytes by 1/256.
 *
 * @param nValue The field.
 * @return The value.
 * @ghidraAddress BRD3D: 0x100073a0
 * @ghidraAddress BRGlide: 0x10007710
 */
float SignedShortToFloat(short nValue);

/**
 * Scale a field of a packet of one signed byte by 1/8.
 *
 * @param bValue The field.
 * @return The value.
 * @ghidraAddress BRD3D: 0x100073c0
 * @ghidraAddress BRGlide: 0x10007730
 */
float ConvertCharToScaledFloat(signed char bValue);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_PACKEDVALUE_H */
