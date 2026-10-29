#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "ear/mfc.h"
#include "ear/playbackimplementations.h"
#include "ear/registry.h"
#include "error.h"
#include "wavechunks.h"

namespace {
// @ghidraAddress EAR: 0x1002e638
const double g_earAdpcmHalfBufferScale = 0.5;

// @ghidraAddress EAR: 0x1002e490
const int g_aiImaAdpcmIndexDeltas[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

// @ghidraAddress EAR: 0x1002e4d0
const int g_aiImaAdpcmStepSizes[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,   21,    23,
    25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,   73,    80,
    88,    97,    107,   118,   130,   143,   157,   173,   190,   209,   230,  253,   279,
    307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,  876,   963,
    1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749, 3024,  3327,
    3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

// The bytes of the header of each channel at the start of a block.
constexpr int IMA_ADPCM_HEADER_BYTES = 4;
constexpr int IMA_ADPCM_LAST_STEP_INDEX = 88;

static inline std::uint32_t TruncatePlaybackInteger(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    return value >= -0x1p63L && value < 0x1p63L ?
               static_cast<std::uint32_t>(static_cast<std::int64_t>(value)) :
               0;
}
} // namespace

ImaAdpcmWaveStream::ImaAdpcmWaveStream() {
    blockBuffer = nullptr;
    adpcmDataBytes = 0;
    blockBufferBytes = 0;
    blockHalfBytes = 0;
    channelStates = nullptr;
    blocksPerHalf = 0;
    dataWordsPerBlock = 0;
    decodedSample = 0;
    decodedDifference = 0;
    decodedNibble = 0;
    memorySource = 0;
}

void ImaAdpcmWaveStream::ReleaseEarPlaybackResources() {
    CloseRegisteredEarPlayback();
    if (channelStates) {
        ::operator delete(channelStates);
        channelStates = nullptr;
    }
}

int ImaAdpcmWaveStream::OpenImaAdpcmWaveStream(const void *source) {
    DWORD offset = 0;
    if (memorySource) {
        char header[16];
        std::memmove(header, source, sizeof(header));
        if (std::memcmp(header, "RIFF", 4) || std::memcmp(header + 8, "WAVEfmt ", 8)) {
            SetEarLastError(0x0f);
            goto fail;
        }
        // The rest of the information is left as the stack holds it, as the original does.
        MMIOINFO info;
        info.pIOProc = nullptr;
        info.fccIOProc = FOURCC_MEM;
        info.pchBuffer = static_cast<HPSTR>(const_cast<void *>(source));
        std::memcpy(&info.cchBuffer, header + 4, sizeof(info.cchBuffer));
        info.adwInfo[0] = 0;
        file = mmioOpenA(nullptr, &info, MMIO_READWRITE);
    } else {
        file = mmioOpenA(static_cast<LPSTR>(const_cast<void *>(source)), nullptr, 0);
    }
    if (!file) {
        SetEarLastError(0x0d);
        goto fail;
    }
    if (mmioDescend(file, &g_RiffWaveChunk, nullptr, 0)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (g_RiffWaveChunk.ckid != FOURCC_RIFF ||
        g_RiffWaveChunk.fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
        SetEarLastError(0x0f);
        goto fail;
    }
    if (mmioDescend(file, &g_WaveFormatChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (g_WaveFormatChunk.cksize != sizeof(adpcmFormat)) {
        // The error is recorded, but the chunk is read whatever its size.
        SetEarLastError(0x0f);
    }
    if (mmioRead(file,
                 static_cast<HPSTR>(static_cast<void *>(&adpcmFormat)),
                 std::bit_cast<LONG>(g_WaveFormatChunk.cksize)) !=
        std::bit_cast<LONG>(g_WaveFormatChunk.cksize)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (adpcmFormat.wfx.wFormatTag != WAVE_FORMAT_IMA_ADPCM) {
        SetEarLastError(0x0f);
        goto fail;
    }
    if (adpcmFormat.wfx.nChannels > 2) {
        SetEarLastError(0x17);
        goto fail;
    }
    if (adpcmFormat.wfx.wBitsPerSample != 4) {
        SetEarLastError(0x17);
        goto fail;
    }
    if (mmioAscend(file, &g_WaveFormatChunk, 0)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    if (mmioDescend(file, &g_WaveDataChunk, &g_RiffWaveChunk, MMIO_FINDCHUNK)) {
        SetEarLastError(0x0e);
        goto fail;
    }
    adpcmDataBytes = g_WaveDataChunk.cksize;
    dataBytes = adpcmDataBytes / adpcmFormat.wfx.nBlockAlign * adpcmFormat.wSamplesPerBlock *
                adpcmFormat.wfx.nChannels * 2;
    // The samples are decoded to 16-bit PCM of the same channels and rate.
    format.nChannels = adpcmFormat.wfx.nChannels;
    format.nSamplesPerSec = adpcmFormat.wfx.nSamplesPerSec;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;
    if (!CheckWavePlaybackDeviceAccess()) {
        SetEarLastError(6);
        goto fail;
    }
    if (memorySource) {
        DisablePlaybackStreaming();
    }
    if (IsEarPlaybackStreaming() && dataBytes > g_earMaximumStreamingBufferBytes) {
        blockBufferBytes = static_cast<DWORD>(
            static_cast<int>(g_earMaximumStreamingBufferBytes / 4) / adpcmFormat.wfx.nBlockAlign);
        if (blockBufferBytes % 2) {
            --blockBufferBytes;
        }
        blockBufferBytes *= adpcmFormat.wfx.nBlockAlign;
        blockHalfBytes = TruncatePlaybackInteger(static_cast<long double>(blockBufferBytes) *
                                                 g_earAdpcmHalfBufferScale);
    } else {
        blockBufferBytes = adpcmDataBytes;
        blockHalfBytes = adpcmDataBytes;
        DisablePlaybackStreaming();
    }
    blocksPerHalf = static_cast<std::uint16_t>(blockHalfBytes / adpcmFormat.wfx.nBlockAlign);
    dataWordsPerBlock = static_cast<DWORD>(
        (adpcmFormat.wfx.nBlockAlign - adpcmFormat.wfx.nChannels * IMA_ADPCM_HEADER_BYTES) / 4);
    SetPlaybackBufferBytes(blocksPerHalf * 2 * adpcmFormat.wSamplesPerBlock *
                           adpcmFormat.wfx.nChannels);
    if (IsEarPlaybackStreaming()) {
        SetPlaybackBufferBytes(GetPlaybackBufferBytes() * 2);
    }
    if (IsEarPlaybackStreaming()) {
        SetStreamHalfBytes(TruncatePlaybackInteger(
            static_cast<long double>(GetPlaybackBufferBytes()) * g_earAdpcmHalfBufferScale));
    } else {
        SetStreamHalfBytes(GetPlaybackBufferBytes());
    }
    dataStartOffset = mmioSeek(file, 0, SEEK_CUR);
    if (dataStartOffset == -1) {
        SetEarLastError(0x0e);
        goto fail;
    }
    sampleBuffer = static_cast<char *>(std::malloc(GetPlaybackBufferBytes()));
    if (!sampleBuffer) {
        SetEarLastError(0x10);
        goto fail;
    }
    std::memset(sampleBuffer, 0, GetPlaybackBufferBytes());
    if (memorySource) {
        // The blocks are read in place, after a header of a fixed size.
        blockBuffer = static_cast<std::uint8_t *>(const_cast<void *>(source)) + 0x3c;
    } else {
        blockBuffer = static_cast<std::uint8_t *>(std::malloc(blockBufferBytes));
        if (!blockBuffer) {
            SetEarLastError(0x10);
            goto fail;
        }
        std::memset(blockBuffer, 0, blockBufferBytes);
        if (HasPlaybackStartPosition() && IsEarPlaybackStreaming()) {
            offset = TruncatePlaybackInteger(static_cast<long double>(adpcmDataBytes) *
                                             GetPlaybackStartPosition());
            offset = offset / adpcmFormat.wfx.nBlockAlign * adpcmFormat.wfx.nBlockAlign;
            readOffset = offset;
            if (mmioSeek(file,
                         std::bit_cast<LONG>(static_cast<DWORD>(dataStartOffset) + readOffset),
                         SEEK_SET) == -1) {
                SetEarLastError(0x0e);
                goto fail;
            }
            ClearPlaybackStartPosition();
        }
        readOffset +=
            static_cast<DWORD>(mmioRead(file,
                                        static_cast<HPSTR>(static_cast<void *>(blockBuffer)),
                                        std::bit_cast<LONG>(blockBufferBytes)));
        if (readOffset != blockBufferBytes + offset) {
            if (readOffset == offset - 1) {
                SetEarLastError(0x0e);
                goto fail;
            }
            if (IsPlaybackLooping()) {
                if (mmioSeek(file, dataStartOffset, SEEK_SET) == -1) {
                    SetEarLastError(0x0e);
                    goto fail;
                }
                offset = readOffset - offset;
                readOffset = static_cast<DWORD>(
                    mmioRead(file,
                             static_cast<HPSTR>(static_cast<void *>(blockBuffer + offset)),
                             std::bit_cast<LONG>(blockBufferBytes - offset)));
                if (readOffset != blockBufferBytes - offset) {
                    SetEarLastError(0x0e);
                    goto fail;
                }
            } else {
                DisablePlaybackStreaming();
            }
        }
    }
    if (!IsEarPlaybackStreaming()) {
        mmioClose(file, 0);
        file = nullptr;
    }
    return 1;

fail:
    if (sampleBuffer) {
        std::free(sampleBuffer);
    }
    sampleBuffer = nullptr;
    if (!memorySource && blockBuffer) {
        std::free(blockBuffer);
    }
    blockBuffer = nullptr;
    if (file) {
        mmioClose(file, 0);
        file = nullptr;
    }
    return 0;
}

int ImaAdpcmWaveStream::InitializeEarPlayback(const void *source, int streaming, int looping) {
    SetPlaybackStreamingAndLooping(streaming, looping);
    memorySource = (GetPlaybackSourceFlags() & 0x6000) == 0x2000;
    if (!source) {
        SetEarLastError(0x0d);
        return 0;
    }
    if (!OpenImaAdpcmWaveStream(source)) {
        return 0;
    }
    channelStates = static_cast<ImaAdpcmChannelState *>(
        MfcAllocateWithNewHandler(adpcmFormat.wfx.nChannels * sizeof(ImaAdpcmChannelState)));
    // Both halves of a streamed buffer are decoded before the playback starts.
    DecodeImaAdpcmStreamBlocks();
    FlipStreamHalfIndex();
    if (IsEarPlaybackStreaming()) {
        DecodeImaAdpcmStreamBlocks();
        FlipStreamHalfIndex();
    }
    SetPlaybackBaseFrequency(TruncatePlaybackInteger(
        static_cast<long double>(format.nSamplesPerSec) * GetPlaybackBaseFrequencyScale()));
    SetPlaybackHandle(AllocateEarEventId());
    int count = g_PlaybackRegistry.GetCount();
    if (static_cast<UINT>(count) == g_PlaybackRegistry.GetHashTableSize()) {
        SetEarLastError(0x1a);
        SetPlaybackHandle(0);
    }
    if (!HasPlaybackHandle()) {
        if (sampleBuffer) {
            std::free(sampleBuffer);
            sampleBuffer = nullptr;
        }
        if (!memorySource && blockBuffer) {
            std::free(blockBuffer);
            blockBuffer = nullptr;
        }
        return 0;
    }
    g_PlaybackRegistry.SetDwordPointerMapValue(GetPlaybackHandle(), this);
    return 1;
}

void ImaAdpcmWaveStream::CloseRegisteredEarPlayback() {
    PcmWaveStream::CloseRegisteredEarPlayback();
    if (!memorySource && blockBuffer) {
        std::free(blockBuffer);
        blockBuffer = nullptr;
    }
}

void ImaAdpcmWaveStream::RefillEarPlaybackStream() {
    writeOffset = blockHalfBytes * !GetStreamHalfIndex();
    std::memset(blockBuffer + writeOffset, 0, blockHalfBytes);
    std::memset(
        sampleBuffer + GetStreamHalfBytes() * !GetStreamHalfIndex(), 0, GetStreamHalfBytes());
    if (!file) {
        CompleteEarPlayback();
        FlipStreamHalfIndex();
        return;
    }
    DWORD remaining = adpcmDataBytes - readOffset;
    DWORD requested = remaining < blockHalfBytes ? remaining : blockHalfBytes;
    DWORD read = static_cast<DWORD>(
        mmioRead(file,
                 static_cast<HPSTR>(static_cast<void *>(blockBuffer + writeOffset)),
                 std::bit_cast<LONG>(requested)));
    if (read != blockHalfBytes) {
        if (read == 0xffffffff) {
            SetEarLastError(0x0e);
            CompleteEarPlayback();
            return;
        }
        if (read == requested) {
            if (IsPlaybackLooping()) {
                if (mmioSeek(file, dataStartOffset, SEEK_SET) == -1) {
                    SetEarLastError(0x0e);
                    CompleteEarPlayback();
                    return;
                }
                readOffset = 0;
                readOffset += static_cast<DWORD>(mmioRead(
                    file,
                    static_cast<HPSTR>(static_cast<void *>(blockBuffer + writeOffset + read)),
                    std::bit_cast<LONG>(blockHalfBytes - read)));
                if (readOffset != blockHalfBytes - read) {
                    SetEarLastError(0x0e);
                    CompleteEarPlayback();
                    return;
                }
                read = 0;
            } else {
                if (!(GetPlaybackSourceFlags() & 0x6000)) {
                    mmioClose(file, 0);
                }
                file = nullptr;
            }
        }
    }
    readOffset += read;
    DecodeImaAdpcmStreamBlocks();
    PcmWaveStream::CopyEarPlaybackSamplesToOutput();
    FlipStreamHalfIndex();
}

void ImaAdpcmWaveStream::CompleteEarPlayback() {
    if (!sampleBuffer || !HasPlaybackHandle()) {
        return;
    }
    if (PcmWaveStream::CheckEarPlaybackLoopCompletion()) {
        DisablePlaybackLooping();
        MarkComplete();
    }
}

std::uint32_t ImaAdpcmWaveStream::GetEarPlaybackMediaLength() {
    return adpcmDataBytes;
}

void ImaAdpcmWaveStream::DecodeImaAdpcmStreamBlocks() {
    std::uint8_t *input = blockBuffer + blockHalfBytes * !GetStreamHalfIndex();
    auto *output = static_cast<std::int16_t *>(
        static_cast<void *>(sampleBuffer + GetStreamHalfBytes() * !GetStreamHalfIndex()));
    std::int16_t channel;
    std::int16_t position;
    // The original unrolls the eight codes of each word; each code runs the same steps.
    auto decodeNibble = [&](std::uint8_t nibble) {
        ImaAdpcmChannelState &state = channelStates[channel];
        decodedNibble = nibble;
        decodedDifference = 0;
        // The step index of a header is not clamped, so a bad header reads past the table.
        if (decodedNibble & 4) {
            decodedDifference += g_aiImaAdpcmStepSizes[state.stepIndex];
        }
        if (decodedNibble & 2) {
            decodedDifference += g_aiImaAdpcmStepSizes[state.stepIndex] >> 1;
        }
        if (decodedNibble & 1) {
            decodedDifference += g_aiImaAdpcmStepSizes[state.stepIndex] >> 2;
        }
        decodedDifference += g_aiImaAdpcmStepSizes[state.stepIndex] >> 3;
        if (decodedNibble & 8) {
            decodedDifference = -decodedDifference;
        }
        int lower = state.predictor + decodedDifference > -0x8000 ?
                        state.predictor + decodedDifference :
                        -0x8000;
        decodedSample = lower < 0x7fff ? lower : 0x7fff;
        output[position] = static_cast<std::int16_t>(decodedSample);
        position = static_cast<std::int16_t>(position + adpcmFormat.wfx.nChannels);
        int upper =
            state.stepIndex + g_aiImaAdpcmIndexDeltas[decodedNibble] < IMA_ADPCM_LAST_STEP_INDEX ?
                state.stepIndex + g_aiImaAdpcmIndexDeltas[decodedNibble] :
                IMA_ADPCM_LAST_STEP_INDEX;
        state.stepIndex = static_cast<std::int16_t>(upper > 0 ? upper : 0);
        state.predictor = static_cast<std::int16_t>(decodedSample);
    };
    for (int block = 0; block < blocksPerHalf; ++block) {
        for (int header = 0; header < adpcmFormat.wfx.nChannels; ++header) {
            channelStates[header].predictor = static_cast<std::int16_t>(input[0] | input[1] << 8);
            input += 2;
            channelStates[header].stepIndex = input[0];
            input += 2;
            *output++ = channelStates[header].predictor;
        }
        channel = 0;
        position = 0;
        for (DWORD word = 0; word < dataWordsPerBlock; ++word) {
            for (int byte = 0; byte < 4; ++byte) {
                decodeNibble(*input & 0xf);
                decodeNibble(*input >> 4 & 0xf);
                ++input;
            }
            if (channel == adpcmFormat.wfx.nChannels - 1) {
                output += adpcmFormat.wfx.nChannels * 8;
                position = 0;
                channel = 0;
            } else {
                ++channel;
                position = channel;
            }
        }
    }
}
