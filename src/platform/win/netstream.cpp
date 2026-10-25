#include "platform/win/netstream.h"

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
