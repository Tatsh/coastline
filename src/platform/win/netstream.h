#pragma once

/**
 * @file
 * The streams of the packets of network play: bytes and bits, the most significant first.
 */

/** The size of the buffer of a NetStream. */
#define NET_STREAM_BUFFER_SIZE 0x200
/** The number of entries of g_aNetPlayerStreams. */
#define NET_PLAYER_STREAM_COUNT 16

/** A stream of the bytes and bits of a packet, read and written from the most significant bit. */
class NetStream {
public:
    int nReadBit;           /*!< The bits read of the byte at nReadPos. */
    int nReadPos;           /*!< The byte read next. */
    int nWriteBit;          /*!< The bits written of the byte at nWritePos. */
    int nWritePos;          /*!< The byte written next, and the end of the data to read. */
    unsigned char *pBuffer; /*!< The bytes, abBuffer or a buffer attached. */
    unsigned char abBuffer[NET_STREAM_BUFFER_SIZE]; /*!< The bytes of a stream of its own. */

    /**
     * An empty stream of its own bytes.
     *
     * @ghidraAddress BRD3D: 0x10073b40
     * @ghidraAddress BRGlide: 0x1006cd80
     */
    NetStream();

    /**
     * Read the bytes of a buffer from their start.
     *
     * @param pData The buffer.
     * @param nSize The number of bytes.
     * @ghidraAddress BRD3D: 0x10073b60
     * @ghidraAddress BRGlide: 0x1006cda0
     */
    void StreamAttachBuffer(unsigned char *pData, int nSize);

    /**
     * Empty the stream.
     *
     * @ghidraAddress BRD3D: 0x10073b80
     * @ghidraAddress BRGlide: 0x1006cdc0
     */
    void StreamResetReadWrite();

    /**
     * Read the stream again from its start.
     *
     * @ghidraAddress BRD3D: 0x10073b90
     * @ghidraAddress BRGlide: 0x1006cdd0
     */
    void StreamResetRead();

    /**
     * Skip bytes, from the next whole byte.
     *
     * @param nCount The number of bytes.
     * @ghidraAddress BRD3D: 0x10073ba0
     * @ghidraAddress BRGlide: 0x1006cde0
     */
    void StreamSkip(int nCount);

    /**
     * Read a byte, from the next whole byte.
     *
     * @return The byte.
     * @ghidraAddress BRD3D: 0x10073bc0
     * @ghidraAddress BRGlide: 0x1006ce00
     */
    unsigned char StreamReadByte();

    /**
     * Read two bytes as an unsigned number, from the next whole byte.
     *
     * @return The number.
     * @ghidraAddress BRD3D: 0x10073be0
     * @ghidraAddress BRGlide: 0x1006ce20
     */
    unsigned StreamReadInt16();

    /**
     * Read three bytes as an unsigned number, from the next whole byte.
     *
     * @return The number.
     * @ghidraAddress BRD3D: 0x10073c10
     * @ghidraAddress BRGlide: 0x1006ce50
     */
    unsigned StreamReadInt24();

    /**
     * Read four bytes as a number, from the next whole byte.
     *
     * @return The number.
     * @ghidraAddress BRD3D: 0x10073c40
     * @ghidraAddress BRGlide: 0x1006ce80
     */
    int StreamReadInt32();

    /**
     * Read bits as an unsigned number.
     *
     * @param nBits The number of bits.
     * @return The number.
     * @ghidraAddress BRD3D: 0x10073c90
     * @ghidraAddress BRGlide: 0x1006ced0
     */
    unsigned BitStreamReadBits(int nBits);

    /**
     * Move the reading to the next whole byte.
     *
     * @ghidraAddress BRD3D: 0x10073d20
     * @ghidraAddress BRGlide: 0x1006cf60
     */
    void StreamAlignReadByte();

    /**
     * Whether the reading arrived at the end of the data written.
     *
     * @return Whether it arrived.
     * @ghidraAddress BRD3D: 0x10073d40
     * @ghidraAddress BRGlide: 0x1006cf80
     */
    bool StreamIsAtEnd();

    /**
     * Write a byte, from the next whole byte.
     *
     * @param bValue The byte.
     * @ghidraAddress BRD3D: 0x10073d60
     * @ghidraAddress BRGlide: 0x1006cfa0
     */
    void StreamWriteByte(unsigned char bValue);

    /**
     * Write the low two bytes of a number, from the next whole byte.
     *
     * @param nValue The number.
     * @ghidraAddress BRD3D: 0x10073d80
     * @ghidraAddress BRGlide: 0x1006cfc0
     */
    void StreamWriteInt16(unsigned nValue);

    /**
     * Write the low three bytes of a number, from the next whole byte.
     *
     * @param nValue The number.
     * @ghidraAddress BRD3D: 0x10073dc0
     * @ghidraAddress BRGlide: 0x1006d000
     */
    void StreamWriteInt24(unsigned nValue);

    /**
     * Write the four bytes of a number, from the next whole byte.
     *
     * @param nValue The number.
     * @ghidraAddress BRD3D: 0x10073e10
     * @ghidraAddress BRGlide: 0x1006d050
     */
    void StreamWriteInt32(unsigned nValue);

    /**
     * Write the low bits of a number.
     *
     * @param nValue The number.
     * @param nBits The number of bits.
     * @ghidraAddress BRD3D: 0x10073e70
     * @ghidraAddress BRGlide: 0x1006d0b0
     */
    void BitStreamWriteBits(unsigned nValue, int nBits);

    /**
     * Move the writing to the next whole byte.
     *
     * @ghidraAddress BRD3D: 0x10073f20
     * @ghidraAddress BRGlide: 0x1006d160
     */
    void StreamFlushWriteByte();

    /**
     * The number of bytes written, counting a byte partly written.
     *
     * @return The number of bytes.
     * @ghidraAddress BRD3D: 0x10073f40
     * @ghidraAddress BRGlide: 0x1006d180
     */
    int StreamGetWriteSize();

    /**
     * The bytes of the stream.
     *
     * @return pBuffer.
     * @ghidraAddress BRD3D: 0x10073f50
     * @ghidraAddress BRGlide: 0x1006d190
     */
    unsigned char *StreamGetBufferPtr();
};

/**
 * The streams of the players of network play.
 *
 * @ghidraAddress BRD3D: 0x11826bd0
 * @ghidraAddress BRGlide: 0x11849f30
 */
extern NetStream g_aNetPlayerStreams[NET_PLAYER_STREAM_COUNT];
