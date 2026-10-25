#include "platform/win/netstream.h"

#include <cmath>

NetStream g_aNetPlayerStreams[NET_PLAYER_STREAM_COUNT];

// The number of bits of a byte.
#define BYTE_BITS 8

NetStream::NetStream() {
    nWriteBit = 0;
    nWritePos = 0;
    nReadBit = 0;
    nReadPos = 0;
    pBuffer = abBuffer;
}

void NetStream::StreamAttachBuffer(unsigned char *pData, int nSize) {
    nWriteBit = 0;
    nReadBit = 0;
    nReadPos = 0;
    nWritePos = nSize;
    pBuffer = pData;
}

void NetStream::StreamResetReadWrite() {
    nWriteBit = 0;
    nWritePos = 0;
    nReadBit = 0;
    nReadPos = 0;
}

void NetStream::StreamResetRead() {
    nReadBit = 0;
    nReadPos = 0;
}

void NetStream::StreamSkip(int nCount) {
    StreamAlignReadByte();
    nReadPos += nCount;
}

unsigned char NetStream::StreamReadByte() {
    StreamAlignReadByte();
    return pBuffer[nReadPos++];
}

unsigned NetStream::StreamReadInt16() {
    const unsigned char *pData;

    StreamAlignReadByte();
    pData = pBuffer + nReadPos;
    nReadPos += 2;
    return static_cast<unsigned>(pData[0]) << 8 | pData[1];
}

unsigned NetStream::StreamReadInt24() {
    const unsigned char *pData;

    StreamAlignReadByte();
    pData = pBuffer + nReadPos;
    nReadPos += 3;
    return (static_cast<unsigned>(pData[0]) << 8 | pData[1]) << 8 | pData[2];
}

int NetStream::StreamReadInt32() {
    const unsigned char *pData;

    StreamAlignReadByte();
    pData = pBuffer + nReadPos;
    nReadPos += 4;
    return static_cast<int>(
        ((static_cast<unsigned>(pData[0]) << 8 | pData[1]) << 8 | pData[2]) << 8 | pData[3]);
}

unsigned NetStream::BitStreamReadBits(int nBits) {
    unsigned nValue = 0;
    // The bits read are counted and the count is not used, as in the original.
    [[maybe_unused]] int nTotalBits = 0;

    while (nBits != 0) {
        int nAvailable = BYTE_BITS - nReadBit;
        int nTake;
        int nShift;
        unsigned nBitsRead;

        if (nAvailable > nBits) {
            nShift = nAvailable - nBits;
            nTake = nBits;
        } else {
            nShift = 0;
            nTake = nAvailable;
        }
        nBitsRead = (((1u << nTake) - 1) << nShift &
                     static_cast<unsigned>(static_cast<signed char>(pBuffer[nReadPos]))) >>
                    nShift;
        nReadBit += nTake;
        nValue = nValue << nTake | nBitsRead;
        nTotalBits += nTake;
        if (nReadBit >= BYTE_BITS) {
            nReadPos++;
            nReadBit = 0;
        }
        nBits -= nTake;
    }
    return nValue;
}

void NetStream::StreamAlignReadByte() {
    if (nReadBit != 0) {
        nReadBit = 0;
        nReadPos++;
    }
}

bool NetStream::StreamIsAtEnd() {
    return nReadPos + (nReadBit != 0 ? 1 : 0) >= nWritePos;
}

void NetStream::StreamWriteByte(unsigned char bValue) {
    StreamFlushWriteByte();
    pBuffer[nWritePos++] = bValue;
}

void NetStream::StreamWriteInt16(unsigned nValue) {
    StreamFlushWriteByte();
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue >> 8);
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue);
}

void NetStream::StreamWriteInt24(unsigned nValue) {
    StreamFlushWriteByte();
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue >> 16);
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue >> 8);
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue);
}

void NetStream::StreamWriteInt32(unsigned nValue) {
    StreamFlushWriteByte();
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue >> 24);
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue >> 16);
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue >> 8);
    pBuffer[nWritePos++] = static_cast<unsigned char>(nValue);
}

void NetStream::BitStreamWriteBits(unsigned nValue, int nBits) {
    while (nBits != 0) {
        int nAvailable = BYTE_BITS - nWriteBit;
        int nTake;
        int nShift;
        unsigned char bKept;
        unsigned char bBits;
        unsigned char *pByte;

        if (nAvailable > nBits) {
            nShift = nAvailable - nBits;
            nTake = nBits;
        } else {
            nShift = 0;
            nTake = nAvailable;
        }
        // The bits already written of the byte are kept, from its most significant bit.
        bKept = static_cast<unsigned char>(((1u << nWriteBit) - 1) << (BYTE_BITS - nWriteBit));
        nBits -= nTake;
        pByte = pBuffer + nWritePos;
        bBits = static_cast<unsigned char>((((1u << nTake) - 1) << nBits & nValue) >> nBits);
        *pByte = static_cast<unsigned char>(bBits << nShift) | (*pByte & bKept);
        nWriteBit += nTake;
        if (nWriteBit >= BYTE_BITS) {
            nWriteBit = 0;
            nWritePos++;
        }
    }
}

void NetStream::StreamFlushWriteByte() {
    if (nWriteBit != 0) {
        nWriteBit = 0;
        nWritePos++;
    }
}

int NetStream::StreamGetWriteSize() {
    return nWritePos + (nWriteBit != 0 ? 1 : 0);
}

unsigned char *NetStream::StreamGetBufferPtr() {
    return pBuffer;
}

static inline int RoundAndClamp(double dValue, int nMin, int nMax) {
    int nResult = static_cast<int>(std::floor(dValue));
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

int FloatToByteIndex(float fValue) {
    return RoundAndClamp(0.5f - fValue * -(1.0f / 1.41015625f), 0, 255);
}

int FloatToSectorIndex(float fValue) {
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

float IndexToWorldCoord(unsigned char bIndex) {
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

NetPlayerProgress g_aNetPlayerProgressSort[NET_PLAYER_STREAM_COUNT];

int CompareNetworkPlayerProgressDescending(const void *pA, const void *pB) {
    const NetPlayerProgress *pFirst = static_cast<const NetPlayerProgress *>(pA);
    const NetPlayerProgress *pSecond = static_cast<const NetPlayerProgress *>(pB);

    // A greater progress gives 0 as an equal one does, never -1.
    return pFirst->fProgress < pSecond->fProgress ? 1 : 0;
}

// The tags of the packets in the high bits of their first byte.
#define NET_PACKET_TAG_TIME_SYNC 0xc0
#define NET_PACKET_TAG_RANKING 0x20
// The bytes of the packets: the tag and the player, then the fields.
#define TIME_SYNC_PACKET_SIZE 6
#define RANKING_PACKET_SIZE (1 + NET_PLAYER_STREAM_COUNT / 2)

bool WriteTimeSyncPacket(NetStream *pStream,
                         unsigned char bPlayer,
                         unsigned nTime,
                         unsigned nValue) {
    if (pStream->StreamGetWriteSize() + TIME_SYNC_PACKET_SIZE > NET_PACKET_MAX_SIZE) {
        return false;
    }
    pStream->StreamWriteByte(bPlayer | NET_PACKET_TAG_TIME_SYNC);
    pStream->StreamWriteInt24(nTime);
    pStream->StreamWriteInt16(nValue);
    return true;
}

int g_adwNetPlayerRankPosition[NET_PLAYER_STREAM_COUNT];

bool WritePlayerRankingPacket(NetStream *pStream, unsigned char bPlayer) {
    if (pStream->StreamGetWriteSize() + RANKING_PACKET_SIZE > NET_PACKET_MAX_SIZE) {
        return false;
    }
    pStream->StreamWriteByte(bPlayer | NET_PACKET_TAG_RANKING);
    for (int i = 0; i < NET_PLAYER_STREAM_COUNT; i += 2) {
        pStream->StreamWriteByte(static_cast<unsigned char>(
            g_adwNetPlayerRankPosition[i] | g_adwNetPlayerRankPosition[i + 1] << 4));
    }
    return true;
}
