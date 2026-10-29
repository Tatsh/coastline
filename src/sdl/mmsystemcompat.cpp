#include "mmsystemcompat.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <FLAC/stream_decoder.h>
#include <SDL3/SDL.h>

#include "gamefiles.h"
#include "win32api.h"

namespace {

// The device of the CD audio, and the first audio track of the CD.
constexpr MCIDEVICEID kCdAudioDevice = 1;
constexpr int kFirstAudioTrack = 2;

std::string TrackPath(int track) {
    char name[32];
    std::snprintf(name, sizeof(name), "music/default/track%02d.flac", track);
    return name;
}

struct DecoderDeleter {
    void operator()(FLAC__StreamDecoder *decoder) const {
        FLAC__stream_decoder_delete(decoder);
    }
};

// The CD audio of the PC release, as FLAC files of its audio tracks played in order.
struct CdAudio {
    std::mutex mutex;
    SDL_AudioStream *stream = nullptr;
    std::unique_ptr<FLAC__StreamDecoder, DecoderDeleter> decoder;
    int trackCount = 0;
    int track = 0;
    int lastTrack = 0;
    bool playing = false;
    bool notify = false;
    HWND window = nullptr;
};

CdAudio g_cdAudio;

FLAC__StreamDecoderWriteStatus WriteFrames(const FLAC__StreamDecoder *,
                                           const FLAC__Frame *frame,
                                           const FLAC__int32 *const channels[],
                                           void *data) {
    auto &cd = *static_cast<CdAudio *>(data);
    unsigned count = frame->header.channels;
    unsigned shift = frame->header.bits_per_sample > 16 ? frame->header.bits_per_sample - 16 : 0;
    std::vector<std::int16_t> samples(static_cast<std::size_t>(frame->header.blocksize) * 2);
    for (unsigned i = 0; i < frame->header.blocksize; ++i) {
        samples[i * 2] = static_cast<std::int16_t>(channels[0][i] >> shift);
        samples[i * 2 + 1] = static_cast<std::int16_t>(channels[count > 1 ? 1 : 0][i] >> shift);
    }
    SDL_AudioSpec spec{SDL_AUDIO_S16, 2, static_cast<int>(frame->header.sample_rate)};
    SDL_SetAudioStreamFormat(cd.stream, &spec, nullptr);
    SDL_PutAudioStreamData(
        cd.stream, samples.data(), static_cast<int>(samples.size() * sizeof(std::int16_t)));
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void ReportError(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *) {
    SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                "A CD audio track is damaged: %s",
                FLAC__StreamDecoderErrorStatusString[status]);
}

// Start the decoder at a track. The caller holds the mutex.
bool OpenTrack(CdAudio &cd, int track) {
    cd.decoder.reset();
    auto path = coastline::FindGameFile(TrackPath(track).c_str());
    if (!path) {
        return false;
    }
    cd.decoder.reset(FLAC__stream_decoder_new());
    if (!cd.decoder ||
        FLAC__stream_decoder_init_file(
            cd.decoder.get(), path->string().c_str(), WriteFrames, nullptr, ReportError, &cd) !=
            FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        cd.decoder.reset();
        return false;
    }
    cd.track = track;
    return true;
}

// Decode the tracks into the stream while it needs more. The last track that ends notifies the
// window.
void SDLCALL FeedCdAudio(void *data, SDL_AudioStream *stream, int additional, int total) {
    auto &cd = *static_cast<CdAudio *>(data);
    std::lock_guard lock(cd.mutex);
    while (cd.playing && SDL_GetAudioStreamQueued(stream) < additional) {
        if (cd.decoder && FLAC__stream_decoder_process_single(cd.decoder.get()) &&
            FLAC__stream_decoder_get_state(cd.decoder.get()) !=
                FLAC__STREAM_DECODER_END_OF_STREAM) {
            continue;
        }
        if (cd.track < cd.lastTrack && OpenTrack(cd, cd.track + 1)) {
            continue;
        }
        cd.playing = false;
        cd.decoder.reset();
        if (cd.notify) {
            PostMessageA(cd.window, MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, kCdAudioDevice);
        }
    }
}

MCIERROR OpenCdAudio(MCI_OPEN_PARMSA *open) {
    CdAudio &cd = g_cdAudio;
    if (open->lpstrDeviceType == nullptr || std::strcmp(open->lpstrDeviceType, "cdaudio") != 0) {
        return MCIERR_DEVICE_NOT_READY;
    }
    int count = 0;
    while (coastline::FindGameFile(TrackPath(kFirstAudioTrack + count).c_str())) {
        ++count;
    }
    if (count == 0 || !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        return MCIERR_DEVICE_NOT_READY;
    }
    SDL_AudioSpec spec{SDL_AUDIO_S16, 2, 44100};
    cd.stream =
        SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FeedCdAudio, &cd);
    if (cd.stream == nullptr) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return MCIERR_DEVICE_NOT_READY;
    }
    cd.trackCount = count;
    open->wDeviceID = kCdAudioDevice;
    SDL_ResumeAudioStreamDevice(cd.stream);
    return 0;
}

MCIERROR SendCdAudioCommand(UINT message, DWORD_PTR flags, DWORD_PTR parameters) {
    CdAudio &cd = g_cdAudio;
    if (cd.stream == nullptr) {
        return MCIERR_DEVICE_NOT_READY;
    }
    if (message == MCI_CLOSE) {
        SDL_DestroyAudioStream(cd.stream);
        std::lock_guard lock(cd.mutex);
        cd.stream = nullptr;
        cd.decoder.reset();
        cd.playing = false;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 0;
    }
    std::lock_guard lock(cd.mutex);
    switch (message) {
    case MCI_STATUS: {
        auto *status = reinterpret_cast<MCI_STATUS_PARMS *>(parameters);
        status->dwReturn = status->dwItem == MCI_STATUS_NUMBER_OF_TRACKS ?
                               static_cast<DWORD_PTR>(kFirstAudioTrack - 1 + cd.trackCount) :
                               static_cast<DWORD_PTR>(cd.track);
        return 0;
    }
    case MCI_PLAY: {
        auto *play = reinterpret_cast<MCI_PLAY_PARMS *>(parameters);
        int last = kFirstAudioTrack - 1 + cd.trackCount;
        int first = (flags & MCI_FROM) ? static_cast<int>(play->dwFrom & 0xff) : cd.track;
        cd.lastTrack =
            (flags & MCI_TO) ? std::min(static_cast<int>(play->dwTo & 0xff), last) : last;
        SDL_ClearAudioStream(cd.stream);
        if (!OpenTrack(cd, std::max(first, kFirstAudioTrack))) {
            cd.playing = false;
            return MCIERR_DEVICE_NOT_READY;
        }
        cd.playing = true;
        cd.notify = (flags & MCI_NOTIFY) != 0;
        cd.window = reinterpret_cast<HWND>(play->dwCallback);
        return 0;
    }
    case MCI_PAUSE:
    case MCI_STOP:
        cd.playing = false;
        cd.decoder.reset();
        SDL_ClearAudioStream(cd.stream);
        return 0;
    default:
        return 0;
    }
}

struct MemoryFile {
    std::vector<char> bytes;
    std::size_t position = 0;
};

MemoryFile *FromHandle(HMMIO file) {
    return static_cast<MemoryFile *>(file);
}

bool ReadWord(MemoryFile &file, DWORD *value) {
    if (file.position > file.bytes.size() || file.bytes.size() - file.position < sizeof(*value)) {
        return false;
    }
    std::memcpy(value, file.bytes.data() + file.position, sizeof(*value));
    file.position += sizeof(*value);
    return true;
}

// Read the header of the chunk at the position, and the form type of a RIFF or list chunk.
bool ReadChunkHeader(MemoryFile &file, MMCKINFO *chunk) {
    if (!ReadWord(file, &chunk->ckid) || !ReadWord(file, &chunk->cksize)) {
        return false;
    }
    chunk->dwDataOffset = static_cast<DWORD>(file.position);
    chunk->fccType = 0;
    if (chunk->ckid == FOURCC_RIFF || chunk->ckid == FOURCC_LIST) {
        return ReadWord(file, &chunk->fccType);
    }
    return true;
}

} // namespace

extern "C" {

HMMIO mmioOpenA(LPSTR path, MMIOINFO *info, DWORD flags) {
    auto found = coastline::FindGameFile(path);
    if (!found) {
        return nullptr;
    }
    std::ifstream stream(*found, std::ios::binary);
    if (!stream) {
        return nullptr;
    }
    auto *file = new MemoryFile;
    file->bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return file;
}

UINT mmioDescend(HMMIO handle, MMCKINFO *chunk, const MMCKINFO *parent, UINT flags) {
    MemoryFile &file = *FromHandle(handle);
    std::size_t end = parent != nullptr ? parent->dwDataOffset + parent->cksize : file.bytes.size();
    if (!(flags & MMIO_FINDCHUNK)) {
        return ReadChunkHeader(file, chunk) ? 0 : MMIOERR_CHUNKNOTFOUND;
    }
    FOURCC wanted = chunk->ckid;
    while (file.position < end) {
        if (!ReadChunkHeader(file, chunk)) {
            return MMIOERR_CHUNKNOTFOUND;
        }
        if (chunk->ckid == wanted) {
            return 0;
        }
        file.position = chunk->dwDataOffset + ((chunk->cksize + 1) & ~DWORD{1});
    }
    return MMIOERR_CHUNKNOTFOUND;
}

UINT mmioAscend(HMMIO handle, MMCKINFO *chunk, UINT flags) {
    FromHandle(handle)->position = chunk->dwDataOffset + ((chunk->cksize + 1) & ~DWORD{1});
    return 0;
}

LONG mmioRead(HMMIO handle, HPSTR buffer, LONG size) {
    MemoryFile &file = *FromHandle(handle);
    if (size < 0 || file.position > file.bytes.size()) {
        return -1;
    }
    std::size_t count = std::min(static_cast<std::size_t>(size), file.bytes.size() - file.position);
    std::memcpy(buffer, file.bytes.data() + file.position, count);
    file.position += count;
    return static_cast<LONG>(count);
}

LONG mmioSeek(HMMIO handle, LONG offset, int origin) {
    MemoryFile &file = *FromHandle(handle);
    long long base = origin == SEEK_CUR ? static_cast<long long>(file.position) :
                     origin == SEEK_END ? static_cast<long long>(file.bytes.size()) :
                                          0;
    long long position = base + offset;
    if (position < 0 || position > static_cast<long long>(file.bytes.size())) {
        return -1;
    }
    file.position = static_cast<std::size_t>(position);
    return static_cast<LONG>(position);
}

UINT mmioGetInfo(HMMIO handle, MMIOINFO *info, UINT flags) {
    MemoryFile &file = *FromHandle(handle);
    *info = {};
    info->pchBuffer = file.bytes.data();
    info->pchNext = file.bytes.data() + std::min(file.position, file.bytes.size());
    info->pchEndRead = file.bytes.data() + file.bytes.size();
    info->lDiskOffset = static_cast<LONG>(file.bytes.size());
    return 0;
}

UINT mmioAdvance(HMMIO handle, MMIOINFO *info, UINT flags) {
    return 0;
}

UINT mmioSetInfo(HMMIO handle, const MMIOINFO *info, UINT flags) {
    MemoryFile &file = *FromHandle(handle);
    file.position = static_cast<std::size_t>(info->pchNext - file.bytes.data());
    return 0;
}

UINT mmioClose(HMMIO handle, UINT flags) {
    delete FromHandle(handle);
    return 0;
}

HGLOBAL GlobalAlloc(UINT flags, size_t size) {
    // A block of no bytes is still a block, as GlobalAlloc() returns one.
    return std::calloc(size != 0 ? size : 1, 1);
}

HGLOBAL GlobalFree(HGLOBAL memory) {
    std::free(memory);
    return nullptr;
}

} // extern "C"

extern "C" MCIERROR
mciSendCommandA(MCIDEVICEID device, UINT message, DWORD_PTR flags, DWORD_PTR parameters) {
    if (message == MCI_OPEN) {
        return OpenCdAudio(reinterpret_cast<MCI_OPEN_PARMSA *>(parameters));
    }
    if (device != kCdAudioDevice) {
        return MCIERR_DEVICE_NOT_READY;
    }
    return SendCdAudioCommand(message, flags, parameters);
}
