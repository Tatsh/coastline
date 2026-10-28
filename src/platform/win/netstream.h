#pragma once

#include "platform/win/packedvalue.h"

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
private:
    int nReadBit;           /*!< The bits read of the byte at nReadPos. */
    int nReadPos;           /*!< The byte read next. */
    int nWriteBit;          /*!< The bits written of the byte at nWritePos. */
    int nWritePos;          /*!< The byte written next, and the end of the data to read. */
    unsigned char *pBuffer; /*!< The bytes, abBuffer or a buffer attached. */
    unsigned char abBuffer[NET_STREAM_BUFFER_SIZE]; /*!< The bytes of a stream of its own. */

    /**
     * Move the reading to the next whole byte.
     *
     * @ghidraAddress BRD3D: 0x10073d20
     * @ghidraAddress BRGlide: 0x1006cf60
     */
    void StreamAlignReadByte();

    /**
     * Move the writing to the next whole byte.
     *
     * @ghidraAddress BRD3D: 0x10073f20
     * @ghidraAddress BRGlide: 0x1006d160
     */
    void StreamFlushWriteByte();

public:
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

/** A player of network play and the progress that ranks it. */
struct NetPlayerProgress {
    int nPlayer;     /*!< The player. */
    float fProgress; /*!< The progress of the player in the race. */
};

/**
 * The players of network play, sorted by their progress.
 *
 * @ghidraAddress BRD3D: 0x11826b50
 * @ghidraAddress BRGlide: 0x11849eb0
 */
extern NetPlayerProgress g_aNetPlayerProgressSort[NET_PLAYER_STREAM_COUNT];

/**
 * Order two NetPlayerProgress entries for qsort() by their progress, the greatest first.
 *
 * @param pA The first entry.
 * @param pB The second entry.
 * @return 1 when the first entry has less progress than the second, and 0 otherwise.
 * @ghidraAddress BRD3D: 0x10071b60
 * @ghidraAddress BRGlide: 0x1006aad0
 */
int CompareNetworkPlayerProgressDescending(const void *pA, const void *pB);

/** The largest size of a packet of a NetStream, in bytes. */
#define NET_PACKET_MAX_SIZE 256

/**
 * Write a packet of the time of a player: the player with the tag 0xc0, the time in 24 bits, and
 * a 16-bit value, when the packet fits.
 *
 * @param pStream The stream.
 * @param bPlayer The player, in the low 6 bits.
 * @param nTime The time, in its low 24 bits.
 * @param nValue The 16-bit value after the time.
 * @return Whether the packet fit and was written.
 * @ghidraAddress BRD3D: 0x10072030
 * @ghidraAddress BRGlide: 0x1006afa0
 */
bool WriteTimeSyncPacket(NetStream *pStream,
                         unsigned char bPlayer,
                         unsigned nTime,
                         unsigned nValue);

/**
 * The position of each network player in the race, from 0 to 15.
 *
 * @ghidraAddress BRD3D: 0x11826b08
 * @ghidraAddress BRGlide: 0x11849e68
 */
extern int g_adwNetPlayerRankPosition[NET_PLAYER_STREAM_COUNT];

/**
 * Write a packet of the positions of the players: the player with the tag 0x20 and the positions
 * of g_adwNetPlayerRankPosition, two to a byte with the first in the low 4 bits, when the packet
 * fits.
 *
 * @param pStream The stream.
 * @param bPlayer The player, in the low 5 bits.
 * @return Whether the packet fit and was written.
 * @ghidraAddress BRD3D: 0x10072110
 * @ghidraAddress BRGlide: 0x1006b080
 * @stub{BRGlide}
 */
bool WritePlayerRankingPacket(NetStream *pStream, unsigned char bPlayer);

/**
 * @ghidraAddress BRD3D: 0x10071f40
 */
bool WritePlayerStatusPacket(NetStream *pStream,
                             unsigned char bPlayer,
                             int nState,
                             int nStatusValue,
                             unsigned char bOptionA,
                             unsigned char bOptionB,
                             unsigned char bOptionC,
                             const char *szPlayerName,
                             unsigned dwOwner);

/**
 * @ghidraAddress BRD3D: 0x10072080
 */
bool WriteGameSettingsPacket(NetStream *pStream, unsigned char bPlayer);

/**
 * @ghidraAddress BRD3D: 0x10221280
 */
extern int g_bNetGameSettingUnk_10221280;

struct KeyframeState;

/**
 * @ghidraAddress BRD3D: 0x10006ec0
 */
void DeserializeVehicleState(KeyframeState *pState, NetStream *pStream);

/**
 * @ghidraAddress BRD3D: 0x10005130
 */
unsigned SendLocalVehicleStateUpdate(KeyframeState *state);

/**
 * @ghidraAddress BRD3D: 0x10220cf0
 */
extern KeyframeState g_LastSentKeyframe;
/**
 * @ghidraAddress BRD3D: 0x10220e60
 */
extern float g_afKeyframeAccumMax[7];

/**
 * @ghidraAddress BRD3D: 0x10094298
 */
extern int g_dwVehicleStateSendCounter;

/**
 * @ghidraAddress BRD3D: 0x10004c60
 */
unsigned SendKeyframePacket(KeyframeState *state);

/**
 * @ghidraAddress BRD3D: 0x10004e50
 */
unsigned SendDeltaKeyframePacket(KeyframeState *state, KeyframeState *base);

/**
 * @ghidraAddress BRD3D: 0x100061a0
 */
void EncodeKeyframeBitstreamFull(NetStream *stream, KeyframeState *state);

/**
 * @ghidraAddress BRD3D: 0x100054a0
 */
unsigned GetInterpolatedRemoteVehicleState(KeyframeState *state, int slotIndex);

/**
 * @ghidraAddress BRD3D: 0x10006830
 */
void EncodeKeyframeBitstreamDelta(NetStream *stream, KeyframeState *state, KeyframeState *base);

/**
 * @ghidraAddress BRD3D: 0x100073e0
 */
void DeserializeKeyframeDelta(KeyframeState *pState,
                              const KeyframeState *pBase,
                              NetStream *pStream);
#ifdef __cplusplus
}
#endif

/**
 * @ghidraAddress BRD3D: 0x11828d14
 */
extern unsigned g_dwNetRaceStartCountdownTime;
