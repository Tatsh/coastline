#include "platform/win/netstream.h"

#include <cmath>

#include "platform/win/directplay.h"
#include "platform/win/ghost.h"
#include "platform/win/settings.h"
#include "trackselect.h"

NetStream g_aNetPlayerStreams[NET_PLAYER_STREAM_COUNT];

static inline float &KeyframePeakField(KeyframeState &state, int index) {
    return index < 2 ? state.afLight[index] : state.afEffectFlags[index - 2];
}

static inline unsigned KeyframeFlag(float value) {
    return value < 0.0f || value > 0.0f;
}

void EncodeKeyframeBitstreamFull(NetStream *stream, KeyframeState *state) {
    stream->BitStreamWriteBits(
        static_cast<short>(ConvertFloatToFixedAngle(state->orientation.w)) >> 8, 8);
    stream->BitStreamWriteBits(
        static_cast<short>(ConvertFloatToFixedAngle(state->orientation.x)) >> 8, 8);
    stream->BitStreamWriteBits(
        static_cast<short>(ConvertFloatToFixedAngle(state->orientation.y)) >> 8, 8);
    stream->BitStreamWriteBits(
        static_cast<short>(ConvertFloatToFixedAngle(state->orientation.z)) >> 8, 8);
    stream->BitStreamWriteBits(
        static_cast<unsigned>(ConvertFloatToFixedAngleNoClamp(state->position.x)) >> 7, 17);
    stream->BitStreamWriteBits(
        static_cast<unsigned>(ConvertFloatToFixedAngleNoClamp(state->position.y)) >> 7, 17);
    stream->BitStreamWriteBits(
        static_cast<short>(ConvertFloatToFixedAngle16B(state->position.z)) >> 1, 15);
    stream->BitStreamWriteBits(static_cast<short>(FloatToSignedShortIndex(state->linearVelocity.x)),
                               16);
    stream->BitStreamWriteBits(static_cast<short>(FloatToSignedShortIndex(state->linearVelocity.y)),
                               16);
    stream->BitStreamWriteBits(
        static_cast<signed char>(ConvertFloatToSignedByte(state->angularVelocity.x)) >> 3, 5);
    stream->BitStreamWriteBits(
        static_cast<signed char>(ConvertFloatToSignedByte(state->angularVelocity.y)) >> 3, 5);
    stream->BitStreamWriteBits(
        static_cast<signed char>(ConvertFloatToSignedByte(state->angularVelocity.z)) >> 3, 5);
    stream->BitStreamWriteBits(
        static_cast<signed char>(ConvertFloatToSignedByte(state->fBodyRollAngle)) >> 4, 4);
    stream->BitStreamWriteBits(
        static_cast<signed char>(FloatToSignedByteIndex(state->fSteerAngle)) >> 2, 4);
    stream->BitStreamWriteBits(
        static_cast<unsigned char>(FloatToByteIndex(state->afWheelAngle[0])) >> 4, 4);
    for (int i = 0; i < 4; ++i) {
        stream->BitStreamWriteBits(KeyframeFlag(state->afWheelContact[i]), 1);
    }
    stream->BitStreamWriteBits(KeyframeFlag(state->fGearControl), 1);
    stream->BitStreamWriteBits(KeyframeFlag(state->fBraking), 1);
    stream->BitStreamWriteBits(KeyframeFlag(state->fReverse), 1);
    stream->BitStreamWriteBits(ConvertFloatToFixed24(state->fProgress), 24);
    stream->BitStreamWriteBits(static_cast<unsigned char>(FloatToEngineRpmIndex(state->fEngineRpm)),
                               6);
    stream->BitStreamWriteBits(QuantizeFloatToLightIndex(state->afLight[0]), 2);
    stream->BitStreamWriteBits(QuantizeFloatToLightIndex(state->afLight[1]), 2);
    for (int i = 0; i < 6; ++i) {
        stream->BitStreamWriteBits(KeyframeFlag(state->afEffectFlags[i]), 1);
    }
}

unsigned SendLocalVehicleStateUpdate(KeyframeState *state) {
    if (state->fProgress >= 4188888.0f && !(g_LastSentKeyframe.fProgress >= 4188888.0f)) {
        g_LastSentKeyframe = *state;
        return SendKeyframePacket(state);
    }
    g_nKeyframeAccumFrames = static_cast<int>(static_cast<unsigned>(g_nKeyframeAccumFrames) + 1u);
    if (g_nKeyframeAccumFrames < 3) {
        for (int i = 0; i < 7; ++i) {
            if (!(g_afKeyframeAccumMax[i] >= KeyframePeakField(*state, i))) {
                g_afKeyframeAccumMax[i] = KeyframePeakField(*state, i);
            }
        }
        return 1;
    }
    for (int i = 0; i < 7; ++i) {
        if (!(KeyframePeakField(*state, i) >= g_afKeyframeAccumMax[i])) {
            KeyframePeakField(*state, i) = g_afKeyframeAccumMax[i];
        }
    }
    g_dwVehicleStateSendCounter =
        static_cast<int>(static_cast<unsigned>(g_dwVehicleStateSendCounter) + 1u);
    for (float &value : g_afKeyframeAccumMax) {
        value = 0.0f;
    }
    g_nKeyframeAccumFrames = 0;
    if ((static_cast<unsigned>(g_dwVehicleStateSendCounter) & 3u) == 0) {
        g_LastSentKeyframe = *state;
        return SendKeyframePacket(state);
    }
    unsigned result = SendDeltaKeyframePacket(state, &g_LastSentKeyframe);
    MaybeSendAllPlayersReadyPacket();
    TickNetworkPauseRequest();
    return result;
}

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

bool WritePlayerStatusPacket(NetStream *pStream,
                             unsigned char bPlayer,
                             int nState,
                             int nStatusValue,
                             unsigned char bOptionA,
                             unsigned char bOptionB,
                             unsigned char bOptionC,
                             const char *szPlayerName,
                             unsigned dwOwner) {
    int state = nState & 0x3f;
    int size = pStream->StreamGetWriteSize() + 10;
    if (state <= 2) {
        size += 24;
    }
    if (state == 4) {
        size += 3;
    }
    if (size > NET_PACKET_MAX_SIZE) {
        return false;
    }
    pStream->StreamWriteByte(bPlayer);
    pStream->StreamWriteByte(static_cast<unsigned char>(nState));
    pStream->StreamWriteByte(static_cast<unsigned char>(nStatusValue));
    pStream->StreamWriteByte(bOptionA);
    pStream->StreamWriteByte(bOptionB);
    pStream->StreamWriteByte(bOptionC);
    pStream->StreamWriteInt32(dwOwner);
    if (state <= 2) {
        bool ended = false;
        for (int i = 0; i < 24; i++) {
            if (ended) {
                pStream->StreamWriteByte(0);
            } else {
                pStream->StreamWriteByte(static_cast<unsigned char>(szPlayerName[i]));
                if (szPlayerName[i] == 0) {
                    ended = true;
                }
            }
        }
    }
    if (state == 4) {
        pStream->StreamWriteInt24(g_dwNetRaceStartCountdownTime);
    }
    return true;
}

int g_bNetGameSettingUnk_10221280;

bool WriteGameSettingsPacket(NetStream *pStream, unsigned char bPlayer) {
    if (pStream->StreamGetWriteSize() + 9 > NET_PACKET_MAX_SIZE) {
        return false;
    }
    pStream->StreamWriteByte(bPlayer | 0xe0);
    pStream->StreamWriteByte(static_cast<unsigned char>(g_bNetMaxPlayers));
    pStream->StreamWriteByte(static_cast<unsigned char>(g_dwSelectedTrackId));
    pStream->StreamWriteByte(static_cast<unsigned char>(g_dwChosenWeather));
    pStream->StreamWriteInt16(g_wNetAllowedCarsMask);
    pStream->StreamWriteByte(static_cast<unsigned char>(g_bNetGameSettingUnk_10221280));
    pStream->StreamWriteByte(static_cast<unsigned char>(g_bNetTireSetting));
    pStream->StreamWriteByte(static_cast<unsigned char>(g_bNetSuspensionSetting));
    return true;
}

static inline void ReadNetworkQuaternion(KeyframeState *pState, NetStream *pStream) {
    pState->orientation.w =
        ConvertFixedAngleToFloat(static_cast<short>(pStream->BitStreamReadBits(8) << 8));
    pState->orientation.x =
        ConvertFixedAngleToFloat(static_cast<short>(pStream->BitStreamReadBits(8) << 8));
    pState->orientation.y =
        ConvertFixedAngleToFloat(static_cast<short>(pStream->BitStreamReadBits(8) << 8));
    pState->orientation.z =
        ConvertFixedAngleToFloat(static_cast<short>(pStream->BitStreamReadBits(8) << 8));
}

static inline void ReadNetworkLightsAndEffects(KeyframeState *pState, NetStream *pStream) {
    pState->fEngineRpm =
        EngineRpmIndexToFloat(static_cast<unsigned char>(pStream->BitStreamReadBits(6)));
    pState->afLight[0] = GetLightLevelScale(static_cast<char>(pStream->BitStreamReadBits(2)));
    pState->afLight[1] = GetLightLevelScale(static_cast<char>(pStream->BitStreamReadBits(2)));
    for (int i = 0; i < 6; i++) {
        pState->afEffectFlags[i] = pStream->BitStreamReadBits(1) != 0 ? 128.0f : 0.0f;
    }
}

void DeserializeVehicleState(KeyframeState *pState, NetStream *pStream) {
    ReadNetworkQuaternion(pState, pStream);
    pState->position.x = ConvertUintToScaledFloat(pStream->BitStreamReadBits(17) << 7);
    pState->position.y = ConvertUintToScaledFloat(pStream->BitStreamReadBits(17) << 7);
    pState->position.z = FixedToRadians(static_cast<short>(pStream->BitStreamReadBits(15) << 1));
    pState->linearVelocity.x =
        SignedShortToFloat(static_cast<short>(pStream->BitStreamReadBits(16)));
    pState->linearVelocity.y =
        SignedShortToFloat(static_cast<short>(pStream->BitStreamReadBits(16)));
    pState->linearVelocity.z = 0.0f;
    pState->angularVelocity.x =
        ConvertCharToScaledFloat(static_cast<signed char>(pStream->BitStreamReadBits(5) << 3));
    pState->angularVelocity.y =
        ConvertCharToScaledFloat(static_cast<signed char>(pStream->BitStreamReadBits(5) << 3));
    pState->angularVelocity.z =
        ConvertCharToScaledFloat(static_cast<signed char>(pStream->BitStreamReadBits(5) << 3));
    pState->fBodyRollAngle =
        ConvertCharToScaledFloat(static_cast<signed char>(pStream->BitStreamReadBits(4) << 4));
    pState->fSteerAngle =
        SignedByteToFloat(static_cast<unsigned char>(pStream->BitStreamReadBits(4) << 2));
    pState->afWheelAngle[0] =
        ByteToFloat(static_cast<unsigned char>(pStream->BitStreamReadBits(4) << 4));
    pState->afWheelAngle[1] = pState->afWheelAngle[0];
    float rearAngle = pState->afWheelAngle[0] + 35.0f;
    if (rearAngle >= 360.0f) {
        rearAngle -= 360.0f;
    }
    pState->afWheelAngle[2] = rearAngle;
    pState->afWheelAngle[3] = rearAngle;
    for (int i = 0; i < 4; i++) {
        pState->afWheelContact[i] = static_cast<float>(pStream->BitStreamReadBits(1));
    }
    pState->fGearControl = pStream->BitStreamReadBits(1) != 0 ? 128.0f : 0.0f;
    pState->fBraking = pStream->BitStreamReadBits(1) != 0 ? 1.0f : 0.0f;
    pState->fReverse = pStream->BitStreamReadBits(1) != 0 ? 1.0f : 0.0f;
    pState->fProgress = Signed24BitToFloat(pStream->BitStreamReadBits(24));
    ReadNetworkLightsAndEffects(pState, pStream);
}

static inline unsigned MergeNetworkDelta(
    unsigned base, unsigned delta, unsigned baseMask, unsigned lowMask, unsigned step) {
    unsigned mode = delta & (step * 3);
    unsigned high = base & baseMask;
    if (mode == step || mode == step * 2) {
        high += mode;
    } else if (mode == step * 3) {
        high -= step;
    }
    return high | (delta & lowMask);
}

void DeserializeKeyframeDelta(KeyframeState *pState,
                              const KeyframeState *pBase,
                              NetStream *pStream) {
    ReadNetworkQuaternion(pState, pStream);
    unsigned base = static_cast<unsigned>(ConvertFloatToFixedAngleNoClamp(pBase->position.x)) >> 7;
    unsigned delta = pStream->BitStreamReadBits(14);
    pState->position.x =
        ConvertUintToScaledFloat(MergeNetworkDelta(base, delta, 0x1f000, 0xfff, 0x1000) << 7);
    base = static_cast<unsigned>(ConvertFloatToFixedAngleNoClamp(pBase->position.y)) >> 7;
    delta = pStream->BitStreamReadBits(14);
    pState->position.y =
        ConvertUintToScaledFloat(MergeNetworkDelta(base, delta, 0x1f000, 0xfff, 0x1000) << 7);
    base = static_cast<short>(ConvertFloatToFixedAngle16B(pBase->position.z)) >> 1;
    delta = pStream->BitStreamReadBits(11);
    pState->position.z = FixedToRadians(
        static_cast<short>(MergeNetworkDelta(base, delta, 0x7e00, 0x1ff, 0x200) << 1));
    base = ConvertFloatToFixed24(pBase->fProgress);
    delta = pStream->BitStreamReadBits(9);
    pState->fProgress = Signed24BitToFloat(MergeNetworkDelta(base, delta, 0xffff80, 0x7f, 0x80));
    ReadNetworkLightsAndEffects(pState, pStream);
}
