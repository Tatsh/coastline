#include <cassert>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include <zlib.h>

#include "common/frontendstate.h"
#include "common/season.h"

Vehicle g_aVehicles[MAX_VEHICLES];
SdlFrontEndState g_frontEnd{};

extern "C" SdlFrontEndState *GetSdlFrontEndState(void) {
    return &g_frontEnd;
}

namespace {

std::vector<unsigned char> Read(const char *path) {
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return {std::istreambuf_iterator<char>(file), {}};
}

void CheckPayload(const std::vector<unsigned char> &file, const PlayerProfile &profile) {
    constexpr auto PAYLOAD_AT = sizeof(g_szSeasonSaveMagic) + sizeof(u32);
    assert(file.size() == PAYLOAD_AT + SEASON_SAVE_PAYLOAD_SIZE + sizeof(SeasonSaveSettings) +
                              SEASON_SAVE_SLOT_NAME_SIZE);
    assert(std::memcmp(file.data(), g_szSeasonSaveMagic, sizeof(g_szSeasonSaveMagic)) == 0);
    assert(std::memcmp(file.data() + PAYLOAD_AT, &profile, sizeof(profile)) == 0);
    for (std::size_t at = PAYLOAD_AT + sizeof(profile); at < PAYLOAD_AT + SEASON_SAVE_PAYLOAD_SIZE;
         ++at) {
        assert(file[at] == 0);
    }
    u32 checksum;
    std::memcpy(&checksum, file.data() + sizeof(g_szSeasonSaveMagic), sizeof(checksum));
    assert(checksum ==
           adler32(adler32(0, nullptr, 0), file.data() + PAYLOAD_AT, SEASON_SAVE_PAYLOAD_SIZE));
}

} // namespace

int main() {
    g_dwSelectedTransmission = 1;
    g_dwTireMenuIndex = 2;
    g_dwSuspensionMenuIndex = 3;
    g_dwCarMenuIndex = 4;
    g_dwHandlingTypeMenuIndex = 5;
    const auto original = std::filesystem::current_path();
    const auto directory =
        std::filesystem::temp_directory_path() /
        ("coastline-season-files-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    std::filesystem::current_path(directory);

    // The final profile in the array exposes an oversized read to AddressSanitizer.
    auto &profile = g_aSeasonSaveBuffers[1];
    profile = {};
    profile.season = 1;
    profile.round = 2;
    profile.championshipFlags = 3;
    profile.lapRecords[5] = 73.125f;
    profile.raceRecords[11] = 239.75f;
    profile.unlocks = 0x4ff;
    g_aVehicles[0].SetProfile(&profile);
    g_aVehicles[1].SetProfile(&g_SeasonProgressPrev);
    std::strcpy(g_szSeasonSaveFileName, "RallySeason2.brf");
    std::strcpy(g_szSeasonSaveSlotName, "Saved season");
    assert(SaveSeasonSaveFile());
    const auto saved = Read(g_szSeasonSaveFileName);
    CheckPayload(saved, profile);

    profile.lapRecords[5] = 68.5f;
    std::strcpy(g_szSeasonSaveSlotName, "Temporary label");
    SaveAutoSaveSeason();
    CheckPayload(Read("AutoSave.brf"), profile);
    const auto slot = Read("RallySeason2.brf");
    CheckPayload(slot, profile);
    assert(std::strcmp(reinterpret_cast<const char *>(slot.data() + slot.size() -
                                                      SEASON_SAVE_SLOT_NAME_SIZE),
                       "Saved season") == 0);
    assert(std::strcmp(g_szSeasonSaveSlotName, "Temporary label") == 0);
    assert(std::strcmp(g_szSeasonSaveFileName, "AutoSave.brf") == 0);

    std::strcpy(g_szSeasonSaveFileName, "RallySeason2.brf");
    assert(LoadSeasonSaveFile(SEASON_LOAD_FILE, false));
    assert(profile.lapRecords[5] == 68.5f && profile.raceRecords[11] == 239.75f);
    assert(profile.season == 1 && profile.round == 2 && profile.unlocks == 0x4ff);
    assert(std::strcmp(g_szSeasonSaveSlotName, "Saved season") == 0);
    assert(g_LoadedSeasonSettings.transmission == 1 && g_LoadedSeasonSettings.tires == 2 &&
           g_LoadedSeasonSettings.suspension == 3 && g_LoadedSeasonSettings.car == 4 &&
           g_LoadedSeasonSettings.handling == 5);

    const auto truncated = saved.size() - sizeof(SeasonSaveSettings) - SEASON_SAVE_SLOT_NAME_SIZE;
    for (std::size_t size : {truncated, truncated + 4, saved.size() - 1}) {
        std::filesystem::resize_file(g_szSeasonSaveFileName, size);
        const auto before = profile;
        assert(!LoadSeasonSaveFile(SEASON_LOAD_FILE, false));
        assert(std::memcmp(&profile, &before, sizeof(profile)) == 0);
        assert(std::strcmp(g_szSeasonSaveSlotName, "Saved season") == 0);
        std::ofstream file(g_szSeasonSaveFileName, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char *>(slot.data()), slot.size());
        assert(file.good());
    }

    g_aVehicles[0].SetProfile(nullptr);
    std::strcpy(g_szSeasonSaveFileName, "missing-profile.brf");
    assert(!SaveSeasonSaveFile());
    assert(!std::filesystem::exists(g_szSeasonSaveFileName));
    std::filesystem::current_path(original);
    std::filesystem::remove_all(directory);
    std::cout << "Season file checks passed.\n";
}
