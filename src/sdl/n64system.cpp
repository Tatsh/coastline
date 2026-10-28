// The calls of the system of the console the front end of the Nintendo 64 release makes, for the
// SDL game. The Controller Pak and the Rumble Pak answer as a controller with no Pak, until the
// saves go to files. The SDL game blanks the screen and runs the audio of the console itself.
// ponytail: no Pak; a file of 32 KiB a controller behind __osContRamRead() and __osContRamWrite()
// takes the place of these once the saves are written.

#include <cstring>
#include <unordered_map>

#include <PR/os.h>
#include <PR/os_cont.h>
#include <SDL3/SDL.h>

#include "assetdma.h"
#include "audio.h"
#include "n64/mainmenu.h"
#include "n64/weatherselect.h"
#include "sdl/cartridgedata.h"
#include "sdl/input.h"
#include "sdl/music.h"
#include "sdl/n64frontend.h"
#include "seasons.h"
#include "vehicle.h"

void LogN64FrontEndData(void) {
    for (int i = 0; g_apWeatherMenuItems[i] != nullptr; ++i) {
        SDL_Log("Weather %d: %s, model 0x%06x",
                i,
                g_apWeatherMenuItems[i]->name,
                static_cast<unsigned>(g_apWeatherMenuItems[i]->romStart));
    }
    for (int i = 0; g_apMainMenuItems[i] != nullptr; ++i) {
        SDL_Log("Main menu %d: %s", i, g_apMainMenuItems[i]->name);
    }
    SDL_Log("Season 0: %s, %d races, car %s",
            g_SeasonTable[0].name,
            g_SeasonTable[0].raceCount,
            g_aCarModels[0].name);
}

u8 g_abMenuMusicModule[MENU_MUSIC_MODULE_SIZE];

namespace {
// The player of the audio of the console, shared with the PC game.
coastline::N64Audio *g_audio;
// The controllers of the SDL game, which the front end reads as the controllers of the console.
const coastline::InputBindings *g_input;
} // namespace

void SetN64FrontEndAudio(coastline::N64Audio *audio) {
    g_audio = audio;
}

void SetN64FrontEndInput(const coastline::InputBindings *input) {
    g_input = input;
}

namespace {

// The bytes of each module inflated last, by its address, for the player of the music.
std::unordered_map<const void *, std::size_t> g_moduleSizes;

// A big-endian word of the cartridge, or 0 past its end.
u32 RomWord(u32 romOffset) {
    auto rom = coastline::CartridgeData::Rom();
    if (romOffset + 4 > rom.size()) {
        return 0;
    }
    return static_cast<u32>(rom[romOffset]) << 24 | static_cast<u32>(rom[romOffset + 1]) << 16 |
           static_cast<u32>(rom[romOffset + 2]) << 8 | rom[romOffset + 3];
}

} // namespace

extern "C" {

// The reads of the cartridge, by its bytes. The front end has these under the names with N64
// appended, and the game code of the PC release keeps its own, which read nothing.
void DmaRead(void *dst, u32 romOffset, s32 size) {
    auto rom = coastline::CartridgeData::Rom();
    if (size > 0 && romOffset + static_cast<u32>(size) <= rom.size()) {
        std::memcpy(dst, rom.data() + romOffset, static_cast<std::size_t>(size));
    }
}

u32 DmaReadWord(u32 romOffset) {
    return RomWord(romOffset);
}

u32 DmaReadAssetSize(u32 romOffset) {
    return RomWord(romOffset + 4);
}

u32 DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
    auto data = coastline::ReadRomModule(coastline::CartridgeData::Rom(), romOffset);
    if (!data) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "The asset at 0x%06x does not inflate: %s",
                    static_cast<unsigned>(romOffset),
                    data.error().c_str());
        return 0;
    }
    if (dst == g_abMenuMusicModule && data->size() > sizeof(g_abMenuMusicModule)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "The menu music does not fit its buffer.");
        return 0;
    }
    std::memcpy(dst, data->data(), data->size());
    g_moduleSizes[dst] = data->size();
    if (job != nullptr) {
        job->dst = nullptr;
    }
    return static_cast<u32>(data->size());
}

s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern) {
    *pattern = 0;
    return 0;
}

s32 osPfsInitPak(OSMesgQueue *queue, OSPfs *pfs, int channel) {
    return PFS_ERR_NOPACK;
}

s32 osPfsRepairId(OSPfs *pfs) {
    return PFS_ERR_NOPACK;
}

s32 osPfsChecker(OSPfs *pfs) {
    return PFS_ERR_NOPACK;
}

s32 osPfsFindFile(
    OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name, s32 *file_no) {
    return PFS_ERR_NOPACK;
}

s32 osPfsReadWriteFile(
    OSPfs *pfs, s32 file_no, u8 flag, int offset, int size_in_bytes, u8 *data_buffer) {
    return PFS_ERR_NOPACK;
}

s32 osPfsFreeBlocks(OSPfs *pfs, s32 *bytes_not_used) {
    return PFS_ERR_NOPACK;
}

s32 osPfsAllocateFile(OSPfs *pfs,
                      u16 company_code,
                      u32 game_code,
                      u8 *game_name,
                      u8 *ext_name,
                      int size_in_bytes,
                      s32 *file_no) {
    return PFS_ERR_NOPACK;
}

s32 osPfsNumFiles(OSPfs *pfs, s32 *max_files, s32 *files_used) {
    return PFS_ERR_NOPACK;
}

s32 osPfsFileState(OSPfs *pfs, s32 file_no, OSPfsState *state) {
    return PFS_ERR_NOPACK;
}

s32 osPfsDeleteFile(OSPfs *pfs, u16 company_code, u32 game_code, u8 *game_name, u8 *ext_name) {
    return PFS_ERR_NOPACK;
}

s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel) {
    return PFS_ERR_NOPACK;
}

s32 osMotorStop(OSPfs *pfs) {
    return 0;
}

void osViBlack(u8 active) {
}

// The controllers are read from the bindings of the SDL game when the front end takes them.
s32 osContStartReadData(OSMesgQueue *queue) {
    return 0;
}

void osContGetReadData(OSContPad *data) {
    if (g_input == nullptr) {
        return;
    }
    auto ports = g_input->Poll();
    for (std::size_t i = 0; i < ports.size(); ++i) {
        data[i].button = ports[i].buttons;
        data[i].stick_x = ports[i].stickX;
        data[i].stick_y = ports[i].stickY;
        data[i].CONT_ERRNO = ports[i].connected ? 0 : CONT_NO_RESPONSE_ERROR;
    }
}

void InitAudioSystem(void *module, void *work) {
    auto size = g_moduleSizes.find(module);
    if (g_audio == nullptr || size == g_moduleSizes.end()) {
        return;
    }
    g_audio->Play({static_cast<const std::uint8_t *>(module), size->second});
}

} // extern "C"
