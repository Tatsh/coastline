// The music of the races on SDL. The PC release plays the tracks of its CD through MCI in music
// mode 1 and the music of the EAR library in mode 2. The SDL game plays the XM modules of the
// Nintendo 64 release by default, or the FLAC files the import made of the CD audio, and retains
// the tracks, the device users, and the commands of the PC release over them.
#include "racemusic.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

#include <SDL3/SDL.h>
#include <fmt/format.h>

#include "common/frameclock.h"
#include "common/settings.h"
#include "common/soundsystem.h"
#include "common/trackselect.h"
#include "flactrack.h"
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
constexpr const char *RACE_MUSIC_PROPERTY = "coastline.race-music";

// The module of a track of the music. g_adwTrackMusicRom of the console gives each track of the
// races the module of its place, race1 to race5, and each mirrored track that of its original. The
// race after a season retains the selected championship track and its module.
int RaceModule(int track, int race) {
    if (race >= TRACK_MIRROR_DESERT && race < TRACK_MIRROR_DESERT + RACE_MODULES) {
        race -= TRACK_MIRRORED;
    }
    if (race >= TRACK_DESERT && race < RACE_MODULES) {
        return 1 + race;
    }
    return track == MENU_MODULE_TRACK ? 0 : 1 + (track - FIRST_RACE_MODULE_TRACK) % RACE_MODULES;
}

} // namespace

/** The music of the races and its device. */
class RaceMusic {
public:
    RaceMusic() = default;

    static RaceMusic *Active() {
        return static_cast<RaceMusic *>(
            SDL_GetPointerProperty(SDL_GetGlobalProperties(), RACE_MUSIC_PROPERTY, nullptr));
    }

    bool Start(const std::filesystem::path &directory, RaceMusicSource source, int musicTrack);
    void Stop();
    void Shutdown();
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
    std::filesystem::path CdTrackPath(int track) const {
        return m_directory / fmt::format("track{:02}.flac", track);
    }
    void PlayModule(int track);
    void FillCd(std::vector<std::int16_t> &output, std::size_t frames);
    bool UpdateCdDevice();
    static void SDLCALL CdCallback(void *user, SDL_AudioStream *stream, int additional, int total);

    int m_currentTrack = 0;
    int m_trackMin = 0;
    int m_trackMax = 0;
    int m_users = 0;
    std::filesystem::path m_directory;
    int m_musicTrack = -1;
    RaceMusicSource m_source = RaceMusicSource::Modules;
    bool m_active = false;
    std::unique_ptr<N64Audio> m_modules;
    N64Audio *m_shared = nullptr;
    std::vector<std::uint8_t> m_module;
    std::optional<int> m_moduleChoice;
    int m_currentModule = 0;
    SDL_AudioStream *m_cdStream = nullptr;
    FlacTrack m_cdTrack;
    int m_cdPlaying = 0;
    std::vector<std::int16_t> m_cdSamples;
    unsigned char m_volume = 48;
    bool m_paused = false;
    bool m_cdDevicePaused = true;
};

bool RaceMusic::Start(const std::filesystem::path &directory,
                      RaceMusicSource source,
                      int musicTrack) {
    if (!g_dwPlayMusicMode) {
        return true;
    }
    if (++m_users != 1) {
        return true;
    }
    std::srand(GetHighResolutionTime());
    m_directory = directory;
    m_musicTrack = musicTrack;
    m_source = source;
    m_moduleChoice.reset();
    m_currentModule = 0;
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
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Cannot open music device: %s", SDL_GetError());
        } else {
            m_cdDevicePaused = true;
            UpdateCdDevice();
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
        SDL_LogWarn(
            SDL_LOG_CATEGORY_AUDIO, "CD music unavailable in %s.", directory.string().c_str());
        Stop();
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
    SDL_Log("Stopping the race music.");
    if (m_cdStream != nullptr) {
        SDL_DestroyAudioStream(m_cdStream);
        m_cdStream = nullptr;
    }
    // The console plays the module of the track on after a race, faded out, until the main menu
    // starts its own. The music of the CD gives way to that module.
    if (m_source == RaceMusicSource::CdAudio && m_active && m_shared != nullptr) {
        PlayModule(m_currentTrack);
    }
    // A race that ends from its pause menu leaves the device of the menus paused.
    if (m_shared != nullptr) {
        m_shared->Resume();
    }
    m_modules.reset();
    m_cdTrack = FlacTrack();
    m_cdSamples.clear();
    m_active = false;
    SDL_Log("Stopped the race music.");
}

void RaceMusic::Shutdown() {
    m_shared = nullptr;
    m_users = 0;
    m_active = false;
    if (m_cdStream != nullptr) {
        SDL_DestroyAudioStream(m_cdStream);
        m_cdStream = nullptr;
    }
    m_modules.reset();
    m_cdTrack = FlacTrack();
    m_cdSamples.clear();
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
        SDL_Log("Playing CD music %s.", CdTrackPath(track).string().c_str());
        const bool opened = m_cdTrack.Open(CdTrackPath(track));
        if (m_cdStream != nullptr) {
            SDL_UnlockAudioStream(m_cdStream);
            // MCI plays a new track after a pause of the race, as when the race restarts.
            m_paused = false;
            UpdateCdDevice();
        }
        return opened;
    }
    PlayModule(track);
    return true;
}

void RaceMusic::PlayModule(int track) {
    int module = m_moduleChoice.value_or(
        RaceModule(track, m_musicTrack < 0 ? static_cast<int>(g_dwSelectedTrackId) : m_musicTrack));
    const auto path = m_directory / (std::string(USA_MUSIC_MODULES[module].name) + ".xm");
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Cannot read music module %s.", path.string().c_str());
        return;
    }
    m_module.assign(std::istreambuf_iterator<char>(file), {});
    m_currentModule = module;
    SDL_Log("Playing the race music %s.", path.string().c_str());
    Modules()->Play(m_module);
    Modules()->Resume();
}

bool RaceMusic::Pause() {
    m_paused = true;
    if (!IsPlaying()) {
        return true;
    }
    if (m_source == RaceMusicSource::CdAudio) {
        return UpdateCdDevice();
    }
    return Modules()->Pause();
}

bool RaceMusic::Resume() {
    m_paused = false;
    if (!IsPlaying()) {
        return true;
    }
    if (m_source == RaceMusicSource::CdAudio) {
        return UpdateCdDevice();
    }
    return Modules()->Resume();
}

void RaceMusic::SetVolume(unsigned char volume) {
    if (m_cdStream != nullptr) {
        SDL_LockAudioStream(m_cdStream);
    }
    m_volume = volume;
    // The player of the modules scales its channels by g_bBgmVolume itself.
    if (IsPlaying() && m_cdStream != nullptr) {
        // The highest level of the music plays the CD at its own level.
        SDL_SetAudioStreamGain(
            m_cdStream,
            static_cast<float>(volume) /
                static_cast<float>(g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT - 1]));
    }
    if (m_cdStream != nullptr) {
        SDL_UnlockAudioStream(m_cdStream);
        UpdateCdDevice();
    }
}

bool RaceMusic::UpdateCdDevice() {
    const bool paused = m_paused || m_volume == 0;
    if (m_cdStream == nullptr || paused == m_cdDevicePaused) {
        return true;
    }
    const bool changed =
        paused ? SDL_PauseAudioStreamDevice(m_cdStream) : SDL_ResumeAudioStreamDevice(m_cdStream);
    if (changed) {
        m_cdDevicePaused = paused;
    }
    return changed;
}

// The music of the CD plays the tracks after the current one in turn, and the first after the
// last, as the window of the PC release does at each notice of MCI.
void RaceMusic::FillCd(std::vector<std::int16_t> &output, std::size_t frames) {
    std::size_t target = output.size() + frames * CD_CHANNELS;
    if (m_volume == 0) {
        output.resize(target, 0);
        return;
    }
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
    if (m_source == RaceMusicSource::Modules) {
        m_moduleChoice = std::clamp(m_currentModule + step, 0, RACE_MODULES);
        m_currentTrack = MENU_MODULE_TRACK + *m_moduleChoice;
    } else {
        m_currentTrack = std::clamp(CurrentTrack() + step, m_trackMin, m_trackMax);
    }
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

struct FrontEndMusic::State {
    State(N64Audio &player, const std::filesystem::path &path, RaceMusicSource choice)
        : audio(player), directory(path), source(choice) {
    }

    N64Audio &audio;
    std::filesystem::path directory;
    RaceMusicSource source;
    RaceMusic cd;
    bool cdStarted = false;
    unsigned char volume = 48;
    std::vector<std::uint8_t> module;
};

FrontEndMusic::FrontEndMusic(N64Audio &audio,
                             const std::filesystem::path &directory,
                             RaceMusicSource source)
    : m_state(std::make_unique<State>(audio, directory, source)) {
}

FrontEndMusic::~FrontEndMusic() {
    Stop();
}

bool FrontEndMusic::Play() {
    State &state = *m_state;
    if (state.source == RaceMusicSource::CdAudio) {
        if (!state.cdStarted) {
            state.cdStarted = state.cd.Start(state.directory, RaceMusicSource::CdAudio, -1);
        }
        if (state.cdStarted) {
            state.cd.SetVolume(state.volume);
        }
        if (state.cdStarted && state.cd.Play(FIRST_MUSIC_TRACK)) {
            state.audio.Stop();
            SDL_Log("Playing the menu music.");
            return true;
        }
        state.cd.Shutdown();
        state.cdStarted = false;
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "PC CD audio unavailable; using N64 menu music.");
    }
    if (state.module.empty()) {
        const auto path = state.directory / "menu.xm";
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Cannot read %s.", path.string().c_str());
            return false;
        }
        state.module.assign(std::istreambuf_iterator<char>(file), {});
    }
    SDL_Log("Playing the menu music.");
    state.audio.Play(state.module);
    state.audio.Resume();
    return true;
}

void FrontEndMusic::Stop() {
    State &state = *m_state;
    state.cd.Shutdown();
    state.cdStarted = false;
    state.audio.Stop();
}

void FrontEndMusic::SetVolume(unsigned char volume) {
    m_state->volume = volume;
    if (m_state->cdStarted) {
        m_state->cd.SetVolume(volume);
    }
}

void FrontEndMusic::SetSource(RaceMusicSource source) {
    if (m_state->source != source) {
        Stop();
        m_state->source = source;
        Play();
    }
}

RaceMusicContext::RaceMusicContext() : m_music(std::make_unique<RaceMusic>()) {
    assert(RaceMusic::Active() == nullptr);
    SDL_SetPointerProperty(SDL_GetGlobalProperties(), RACE_MUSIC_PROPERTY, m_music.get());
}

RaceMusicContext::~RaceMusicContext() {
    m_music->Shutdown();
    SDL_ClearProperty(SDL_GetGlobalProperties(), RACE_MUSIC_PROPERTY);
}

bool StartRaceMusic(const std::filesystem::path &directory,
                    RaceMusicSource source,
                    int musicTrack) {
    auto *music = RaceMusic::Active();
    return music && music->Start(directory, source, musicTrack);
}

void StopRaceMusic() {
    if (auto *music = RaceMusic::Active()) {
        music->Stop();
    }
}

void ShutdownRaceMusic() {
    if (auto *music = RaceMusic::Active()) {
        music->Shutdown();
    }
}

void RestartRaceMusic() {
    if (auto *music = RaceMusic::Active()) {
        music->Play(music->CurrentTrack());
    }
}

void ShareRaceMusicPlayer(N64Audio *audio) {
    if (auto *music = RaceMusic::Active()) {
        music->Share(audio);
    }
}

std::size_t RenderRaceMusic(std::size_t frames) {
    auto *music = RaceMusic::Active();
    return music ? music->Render(frames) : 0;
}

} // namespace coastline

bool SetCurrentMusicTrack(int nModeIndex) {
    auto *music = coastline::RaceMusic::Active();
    return !music || music->Play(nModeIndex);
}

bool PauseMusicPlayback(void) {
    auto *music = coastline::RaceMusic::Active();
    return !music || music->Pause();
}

bool UnpauseMusicPlayback(void) {
    auto *music = coastline::RaceMusic::Active();
    return !music || music->Resume();
}

int RandomizeMusicTrack(void) {
    auto *music = coastline::RaceMusic::Active();
    return music ? music->RandomTrack() : 0;
}

bool SelectPreviousMusicTrack(void) {
    auto *music = coastline::RaceMusic::Active();
    return !music || music->Step(-1);
}

bool SelectNextMusicTrack(void) {
    auto *music = coastline::RaceMusic::Active();
    return !music || music->Step(1);
}

bool DispatchMusicVolumeChange(unsigned char bVolume) {
    if (auto *music = coastline::RaceMusic::Active()) {
        music->SetVolume(bVolume);
    }
    return true;
}
