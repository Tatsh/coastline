// The music of the races on SDL. The PC release plays the tracks of its CD through MCI in music
// mode 1 and the music of the EAR library in mode 2. The SDL game plays the XM modules of the
// Nintendo 64 release by default, or the FLAC files the import made of the CD audio, and retains
// the tracks, the device users, and the commands of the PC release over them.
#include "racemusic.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

#include <FLAC/stream_decoder.h>
#include <SDL3/SDL.h>
#include <fmt/format.h>

#include "common/frameclock.h"
#include "common/settings.h"
#include "common/soundsystem.h"
#include "common/trackselect.h"
#include "importer.h"
#include "music.h"

namespace coastline {

namespace {

// The first track of the music, the first after the track of the data of the CD, and the last
// track of the CD of the PC release.
constexpr int FIRST_MUSIC_TRACK = 2;
constexpr int LAST_MUSIC_TRACK = 13;
// The track of the modules that plays the music of the menus. The other tracks take the modules
// of the races in turn.
constexpr int MENU_MODULE_TRACK = 2;
constexpr int FIRST_RACE_MODULE_TRACK = 3;
constexpr int RACE_MODULES = 5;
// The CD audio, 16-bit stereo at 44,100 Hz.
constexpr int CD_RATE = 44100;
constexpr int CD_CHANNELS = 2;

// The module of a track of the music. g_adwTrackMusicRom of the console gives each track of the
// races the module of its place, race1 to race5, and each mirrored track that of its original. The
// race after a season has a sound sample there rather than a module, and takes the module of the
// track of the music in turn.
int RaceModule(int track) {
    int race = static_cast<int>(g_dwSelectedTrackId);
    if (race >= TRACK_MIRROR_DESERT && race < TRACK_MIRROR_DESERT + RACE_MODULES) {
        race -= TRACK_MIRRORED;
    }
    if (race >= TRACK_DESERT && race < RACE_MODULES) {
        return 1 + race;
    }
    return track == MENU_MODULE_TRACK ? 0 : 1 + (track - FIRST_RACE_MODULE_TRACK) % RACE_MODULES;
}

struct DecoderDeleter {
    void operator()(FLAC__StreamDecoder *decoder) const {
        FLAC__stream_decoder_delete(decoder);
    }
};

/** A FLAC file decoded on request to 16-bit stereo frames. */
class FlacTrack {
public:
    bool Open(const std::filesystem::path &path) {
        m_decoder.reset(FLAC__stream_decoder_new());
        if (!m_decoder) {
            return false;
        }
        auto status = FLAC__stream_decoder_init_file(
            m_decoder.get(), path.string().c_str(), Write, nullptr, Error, this);
        if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                        "%s cannot be played: %s",
                        path.string().c_str(),
                        FLAC__StreamDecoderInitStatusString[status]);
            m_decoder.reset();
            return false;
        }
        return true;
    }

    /**
     * Decode frames onto the end of the output.
     *
     * @return False once the file ends or fails.
     */
    bool Decode(std::vector<std::int16_t> &output, std::size_t frames) {
        m_output = &output;
        std::size_t target = output.size() + frames * CD_CHANNELS;
        while (m_decoder && output.size() < target) {
            if (!FLAC__stream_decoder_process_single(m_decoder.get()) ||
                FLAC__stream_decoder_get_state(m_decoder.get()) ==
                    FLAC__STREAM_DECODER_END_OF_STREAM) {
                m_decoder.reset();
            }
        }
        return m_decoder != nullptr;
    }

private:
    static FLAC__StreamDecoderWriteStatus Write(const FLAC__StreamDecoder *,
                                                const FLAC__Frame *frame,
                                                const FLAC__int32 *const buffer[],
                                                void *user) {
        auto *track = static_cast<FlacTrack *>(user);
        unsigned bits = frame->header.bits_per_sample;
        for (unsigned i = 0; i < frame->header.blocksize; ++i) {
            for (unsigned channel = 0; channel < CD_CHANNELS; ++channel) {
                FLAC__int32 sample = buffer[channel < frame->header.channels ? channel : 0][i];
                track->m_output->push_back(static_cast<std::int16_t>(
                    bits > 16 ? sample >> (bits - 16) : sample << (16 - bits)));
            }
        }
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    static void Error(const FLAC__StreamDecoder *, FLAC__StreamDecoderErrorStatus status, void *) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "The music file is damaged: %s",
                    FLAC__StreamDecoderErrorStatusString[status]);
    }

    std::unique_ptr<FLAC__StreamDecoder, DecoderDeleter> m_decoder;
    std::vector<std::int16_t> *m_output = nullptr;
};

/** The music of the races and its device. */
class RaceMusic {
public:
    static RaceMusic &Get() {
        static RaceMusic music;
        return music;
    }

    bool Start(const std::filesystem::path &directory, RaceMusicSource source);
    void Stop();
    bool IsPlaying() const {
        return g_dwPlayMusicMode != 0 && m_users != 0 && m_active;
    }
    bool Play(int track);
    bool Pause();
    bool Resume();
    void SetVolume(unsigned char volume);
    int CurrentTrack() const {
        if (g_dwPlayMusicMode == 0 || m_users == 0) {
            return 0;
        }
        return m_active ? m_currentTrack : 0;
    }
    std::size_t Render(std::size_t frames);
    int RandomTrack() const;
    bool Step(int step);
    void Share(N64Audio *audio) {
        m_shared = audio;
    }

private:
    // The modules play through the player of the menus when it is shared, and else through a
    // player of the races.
    N64Audio *Modules() const {
        return m_shared != nullptr ? m_shared : m_modules.get();
    }
    RaceMusic() = default;
    std::filesystem::path CdTrackPath(int track) const {
        return m_directory / fmt::format("track{:02}.flac", track);
    }
    void FillCd(std::vector<std::int16_t> &output, std::size_t frames);
    static void SDLCALL CdCallback(void *user, SDL_AudioStream *stream, int additional, int total);

    int m_currentTrack = 0;
    int m_trackMin = 0;
    int m_trackMax = 0;
    int m_users = 0;
    std::filesystem::path m_directory;
    RaceMusicSource m_source = RaceMusicSource::Modules;
    bool m_active = false;
    std::unique_ptr<N64Audio> m_modules;
    N64Audio *m_shared = nullptr;
    std::vector<std::uint8_t> m_module;
    SDL_AudioStream *m_cdStream = nullptr;
    FlacTrack m_cdTrack;
    int m_cdPlaying = 0;
    std::vector<std::int16_t> m_cdSamples;
};

bool RaceMusic::Start(const std::filesystem::path &directory, RaceMusicSource source) {
    if (!g_dwPlayMusicMode) {
        return true;
    }
    if (++m_users != 1) {
        return true;
    }
    std::srand(GetHighResolutionTime());
    m_directory = directory;
    m_source = source;
    m_currentTrack = FIRST_MUSIC_TRACK;
    m_trackMin = FIRST_MUSIC_TRACK;
    m_trackMax = FIRST_MUSIC_TRACK - 1;
    if (source == RaceMusicSource::CdAudio) {
        // The tracks run from the first to the last file the import wrote in turn.
        std::error_code error;
        while (m_trackMax < LAST_MUSIC_TRACK &&
               std::filesystem::exists(CdTrackPath(m_trackMax + 1), error)) {
            ++m_trackMax;
        }
        SDL_AudioSpec spec{SDL_AUDIO_S16, CD_CHANNELS, CD_RATE};
        m_cdStream =
            SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, CdCallback, this);
        if (m_cdStream == nullptr) {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_AUDIO, "The music device did not open: %s", SDL_GetError());
        } else {
            SDL_ResumeAudioStreamDevice(m_cdStream);
        }
    } else {
        m_trackMax = LAST_MUSIC_TRACK;
        if (m_shared == nullptr) {
            m_modules = std::make_unique<N64Audio>();
            m_modules->OpenDevice();
        }
    }
    m_active = m_trackMax >= m_trackMin;
    if (!m_active) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "%s has no music, and the races are silent.",
                    directory.string().c_str());
    }
    return m_active;
}

void RaceMusic::Stop() {
    if (g_dwPlayMusicMode == 0 || m_users == 0) {
        return;
    }
    if (--m_users != 0) {
        return;
    }
    SDL_Log("The music of the race stops.");
    if (m_cdStream != nullptr) {
        SDL_DestroyAudioStream(m_cdStream);
        m_cdStream = nullptr;
    }
    // A race that ends from its pause menu leaves the device of the menus paused.
    if (m_shared != nullptr) {
        m_shared->Stop();
        m_shared->Resume();
    }
    m_modules.reset();
    m_cdTrack = FlacTrack();
    m_cdSamples.clear();
    m_active = false;
}

bool RaceMusic::Play(int track) {
    if (g_dwPlayMusicMode == 0 || m_users == 0) {
        return true;
    }
    if (track < m_trackMin) {
        track = m_trackMin;
    }
    if (track > m_trackMax) {
        track = m_trackMax;
    }
    m_currentTrack = track;
    if (!m_active) {
        return true;
    }
    if (m_source == RaceMusicSource::CdAudio) {
        if (m_cdStream != nullptr) {
            SDL_LockAudioStream(m_cdStream);
        }
        m_cdSamples.clear();
        m_cdPlaying = track;
        SDL_Log("The race plays track %d of the CD.", track);
        m_cdTrack.Open(CdTrackPath(track));
        if (m_cdStream != nullptr) {
            SDL_UnlockAudioStream(m_cdStream);
            // MCI plays a new track after a pause of the race, as when the race restarts.
            SDL_ResumeAudioStreamDevice(m_cdStream);
        }
        return true;
    }
    int module = RaceModule(track);
    std::ifstream file(m_directory / (std::string(USA_MUSIC_MODULES[module].name) + ".xm"),
                       std::ios::binary);
    if (!file) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "The music module %s cannot be read.",
                    USA_MUSIC_MODULES[module].name);
        return true;
    }
    m_module.assign(std::istreambuf_iterator<char>(file), {});
    SDL_Log("The race plays the music module %s.", USA_MUSIC_MODULES[module].name);
    Modules()->Play(m_module);
    Modules()->Resume();
    return true;
}

bool RaceMusic::Pause() {
    if (!IsPlaying()) {
        return true;
    }
    if (m_cdStream != nullptr) {
        return SDL_PauseAudioStreamDevice(m_cdStream);
    }
    return Modules()->Pause();
}

bool RaceMusic::Resume() {
    if (!IsPlaying()) {
        return true;
    }
    if (m_cdStream != nullptr) {
        return SDL_ResumeAudioStreamDevice(m_cdStream);
    }
    return Modules()->Resume();
}

void RaceMusic::SetVolume(unsigned char volume) {
    // The player of the modules scales its channels by g_bBgmVolume itself.
    if (IsPlaying() && m_cdStream != nullptr) {
        // The highest level of the music plays the CD at its own level.
        SDL_SetAudioStreamGain(
            m_cdStream,
            static_cast<float>(volume) /
                static_cast<float>(g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT - 1]));
    }
}

// The music of the CD plays the tracks after the current one in turn, and the first after the
// last, as the window of the PC release does at each notice of MCI.
void RaceMusic::FillCd(std::vector<std::int16_t> &output, std::size_t frames) {
    std::size_t target = output.size() + frames * CD_CHANNELS;
    for (int attempts = 0; output.size() < target && attempts <= LAST_MUSIC_TRACK; ++attempts) {
        if (m_cdTrack.Decode(output, (target - output.size()) / CD_CHANNELS)) {
            continue;
        }
        m_cdPlaying = m_cdPlaying >= m_trackMax ? m_trackMin : m_cdPlaying + 1;
        if (!m_cdTrack.Open(CdTrackPath(m_cdPlaying))) {
            break;
        }
    }
    output.resize(target);
}

void SDLCALL RaceMusic::CdCallback(void *user, SDL_AudioStream *stream, int additional, int) {
    auto *music = static_cast<RaceMusic *>(user);
    if (additional <= 0 || music->m_cdPlaying == 0) {
        return;
    }
    music->m_cdSamples.clear();
    music->FillCd(music->m_cdSamples,
                  static_cast<std::size_t>(additional) / (sizeof(std::int16_t) * CD_CHANNELS) + 1);
    SDL_PutAudioStreamData(stream,
                           music->m_cdSamples.data(),
                           static_cast<int>(music->m_cdSamples.size() * sizeof(std::int16_t)));
}

// Pick a track from the fourth on, as RandomizeMusicTrack() of the PC release does, other than the
// one that plays when there are more than six.
int RaceMusic::RandomTrack() const {
    constexpr int FIRST = 3;
    constexpr int LEFT_OUT = 5;
    constexpr int REPEAT_TRACKS = 6;
    constexpr int RAND_RANGE = 0x8000;
    int track;
    do {
        // The rand() of the C library of Visual C++ returns 15 bits.
        track = (std::rand() & (RAND_RANGE - 1)) * (m_trackMax - LEFT_OUT) / RAND_RANGE + FIRST;
        track = std::min(std::max(track, FIRST), m_trackMax);
    } while (m_trackMax > REPEAT_TRACKS && track == m_currentTrack);
    return track;
}

// SelectPreviousMusicTrack() and SelectNextMusicTrack() of the PC release.
bool RaceMusic::Step(int step) {
    if (g_dwPlayMusicMode == 0 || m_users == 0) {
        return true;
    }
    m_currentTrack = std::clamp(CurrentTrack() + step, m_trackMin, m_trackMax);
    return Play(m_currentTrack);
}

std::size_t RaceMusic::Render(std::size_t frames) {
    std::vector<std::int16_t> samples;
    if (m_source == RaceMusicSource::CdAudio) {
        FillCd(samples, frames);
    } else if (Modules() != nullptr) {
        samples = Modules()->Render(frames);
    }
    return static_cast<std::size_t>(
        std::ranges::count_if(samples, [](std::int16_t sample) { return sample != 0; }));
}

} // namespace

bool StartRaceMusic(const std::filesystem::path &directory, RaceMusicSource source) {
    return RaceMusic::Get().Start(directory, source);
}

void StopRaceMusic() {
    RaceMusic::Get().Stop();
}

void RestartRaceMusic() {
    RaceMusic &music = RaceMusic::Get();
    music.Play(music.CurrentTrack());
}

void ShareRaceMusicPlayer(N64Audio *audio) {
    RaceMusic::Get().Share(audio);
}

std::size_t RenderRaceMusic(std::size_t frames) {
    return RaceMusic::Get().Render(frames);
}

} // namespace coastline

bool SetCurrentMusicTrack(int nModeIndex) {
    return coastline::RaceMusic::Get().Play(nModeIndex);
}

bool PauseMusicPlayback(void) {
    return coastline::RaceMusic::Get().Pause();
}

bool UnpauseMusicPlayback(void) {
    return coastline::RaceMusic::Get().Resume();
}

int RandomizeMusicTrack(void) {
    return coastline::RaceMusic::Get().RandomTrack();
}

bool SelectPreviousMusicTrack(void) {
    return coastline::RaceMusic::Get().Step(-1);
}

bool SelectNextMusicTrack(void) {
    return coastline::RaceMusic::Get().Step(1);
}

bool DispatchMusicVolumeChange(unsigned char bVolume) {
    coastline::RaceMusic::Get().SetVolume(bVolume);
    return true;
}
