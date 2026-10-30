#include "common/packedvalue.h"

#include <cmath>

static inline int RoundAndClamp(double dValue, int nMin, int nMax) {
    // The truncation through 64 bits matches the MSVC runtime, which gives 0 for NaN.
    int nResult = static_cast<int>(static_cast<long long>(std::floor(dValue)));
    if (nResult < nMin) {
        nResult = nMin;
    }
    if (nResult > nMax) {
        nResult = nMax;
    }
    return nResult;
}

int FloatToSignedByteIndex(float fValue) {
    return RoundAndClamp(0.5f - fValue * 128.0f, -32, 31);
}

int ConvertFloatToFixedAngle(float fValue) {
    return RoundAndClamp(0.5f - fValue * 32768.0f, -32768, 32767);
}

int ConvertFloatToFixedAngleNoClamp(float fValue) {
    return RoundAndClamp(0.5f - fValue * -8192.0f, 0, 0xffffff);
}

int FloatToByteIndex(float fValue) {
    return RoundAndClamp(0.5f - fValue * -(1.0f / 1.41015625f), 0, 255);
}

int FloatToEngineRpmIndex(float fValue) {
    return RoundAndClamp(0.5f - (fValue - 400.0f) * -(63.0f / 7600.0f), 0, 63);
}

int ConvertFloatToFixed24(float fValue) {
    return RoundAndClamp(0.5f - fValue * -2.0f, -0x800000, 0x7fffff);
}

int ConvertFloatToFixedAngle16B(float fValue) {
    return RoundAndClamp(0.5f - fValue * -128.0f, -32768, 32767);
}

int FloatToSignedShortIndex(float fValue) {
    return RoundAndClamp(0.5f - fValue * -256.0f, -32768, 32767);
}

int ConvertFloatToSignedByte(float fValue) {
    return RoundAndClamp(0.5f - fValue * -8.0f, -128, 127);
}

unsigned char QuantizeFloatToLightIndex(float fScale) {
    if (fScale < 128.0f) {
        return 0;
    }
    if (fScale < 171.0f) {
        return 1;
    }
    return fScale < 213.0f ? 2 : 3;
}

int ConvertDoubleToSignedByte(double dValue) {
    return RoundAndClamp(0.5 - dValue * -128.0, -128, 127);
}

float SignedByteToFloat(unsigned char bValue) {
    int nValue = (bValue & 0x20) ? static_cast<signed char>(bValue | 0xc0) : bValue & 0x3f;
    return nValue * -(1.0f / 128.0f);
}

float EngineRpmIndexToFloat(unsigned char bIndex) {
    return 400.0f - bIndex * -(7600.0f / 63.0f);
}

float GetLightLevelScale(char nLightLevel) {
    if (nLightLevel == 0) {
        return 0.0f;
    }
    if (nLightLevel == 1) {
        return 170.0f;
    }
    if (nLightLevel == 2) {
        return 212.0f;
    }
    return 255.0f;
}

float ConvertUintToScaledFloat(unsigned dwValue) {
    return dwValue * (1.0f / 8192.0f);
}

float Signed24BitToFloat(int nValue) {
    nValue = (nValue & 0x800000) ? nValue | ~0xffffff : nValue & 0xffffff;
    return nValue * 0.5f;
}

float ConvertFixedAngleToFloat(short nValue) {
    return nValue * -(1.0f / 32768.0f);
}

float ByteToFloat(unsigned char bValue) {
    return bValue * 1.41015625f;
}

float FixedToRadians(short nValue) {
    return nValue * (1.0f / 128.0f);
}

float SignedShortToFloat(short nValue) {
    return nValue * (1.0f / 256.0f);
}

float ConvertCharToScaledFloat(signed char bValue) {
    return bValue * (1.0f / 8.0f);
}
