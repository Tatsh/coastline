#include "assetpack.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <span>
#include <vector>

#include <fmt/format.h>

#include "data/branded-textures.h"
#include "paths.h"

namespace coastline {

namespace {

std::optional<std::vector<std::uint8_t>> ReadFile(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in), {});
}

// Write the bytes and read them back.
std::optional<std::string> WriteChecked(const std::filesystem::path &path,
                                        std::span<const std::uint8_t> bytes) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char *>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }
    auto written = ReadFile(path);
    if (!written || !std::ranges::equal(*written, bytes)) {
        return fmt::format("{} does not read back as written.", path.string());
    }
    return std::nullopt;
}

std::optional<std::string>
WriteJson(const std::filesystem::path &path, const nlohmann::ordered_json &json, int indent) {
    std::string text = json.dump(indent) + '\n';
    return WriteChecked(
        path, std::span(reinterpret_cast<const std::uint8_t *>(text.data()), text.size()));
}

// The model of the PC release whose primitives of the listed meshes sample the cartridge image in
// place of the PC image, from a file of the cartridge image beside the model. The PC image remains
// for the primitives of the other meshes.
std::optional<std::string> SampleInMeshes(const std::filesystem::path &defaults,
                                          const std::filesystem::path &boss,
                                          const nlohmann::json &entry,
                                          std::span<const std::uint8_t> cartridge) {
    auto modelPath = defaults / entry["model"].get<std::string>();
    auto pc = std::filesystem::path(entry["pc"].get<std::string>()).filename();
    auto uri = pc.stem().string() + "-n64.png";
    std::ifstream text(modelPath);
    auto model = nlohmann::ordered_json::parse(text, nullptr, false);
    text.close();
    if (model.is_discarded()) {
        return fmt::format("{} is not a glTF document.", modelPath.string());
    }
    auto &images = model["images"];
    int pcImage = -1;
    for (std::size_t i = 0; i < images.size(); ++i) {
        if (images[i]["uri"] == uri) {
            return std::nullopt;
        }
        if (images[i]["uri"] == pc.string()) {
            pcImage = static_cast<int>(i);
        }
    }
    if (pcImage < 0) {
        return fmt::format("{} does not have the image {}.", modelPath.string(), pc.string());
    }
    auto original = ReadFile(modelPath);
    if (!original) {
        return fmt::format("{} cannot be read.", modelPath.string());
    }
    auto bossModel = boss / entry["model"].get<std::string>();
    if (auto failure = WriteChecked(bossModel, *original)) {
        return failure;
    }
    if (auto failure = WriteChecked(modelPath.parent_path() / uri, cartridge)) {
        return failure;
    }
    images.push_back({{"uri", uri}});
    auto &textures = model["textures"];
    auto &materials = model["materials"];
    // A copy of each material of the PC image, sampling the cartridge image.
    std::map<std::size_t, std::size_t> copyOf;
    for (int mesh : entry["meshes"]) {
        for (auto &primitive : model["meshes"][static_cast<std::size_t>(mesh)]["primitives"]) {
            if (!primitive.contains("material")) {
                continue;
            }
            auto material = primitive["material"].get<std::size_t>();
            auto found = copyOf.find(material);
            if (found == copyOf.end()) {
                const auto &base = materials[material]["pbrMetallicRoughness"];
                if (!base.contains("baseColorTexture") ||
                    textures[base["baseColorTexture"]["index"].get<std::size_t>()]["source"] !=
                        pcImage) {
                    continue;
                }
                auto texture = textures[base["baseColorTexture"]["index"].get<std::size_t>()];
                texture["source"] = images.size() - 1;
                textures.push_back(std::move(texture));
                auto copy = materials[material];
                copy["pbrMetallicRoughness"]["baseColorTexture"]["index"] = textures.size() - 1;
                materials.push_back(std::move(copy));
                found = copyOf.emplace(material, materials.size() - 1).first;
            }
            primitive["material"] = found->second;
        }
    }
    return WriteJson(modelPath, model, 1);
}

} // namespace

std::optional<std::string> CurateAssetPacks(const std::filesystem::path &defaults) {
    auto boss = defaults.parent_path() / BOSS_PACK;
    auto list = nlohmann::json::parse(std::begin(BRANDED_TEXTURES), std::end(BRANDED_TEXTURES));
    for (const auto &entry : list["textures"]) {
        auto pcPath = defaults / entry["pc"].get<std::string>();
        auto pc = ReadFile(pcPath);
        auto cartridge = ReadFile(defaults / entry["cartridge"].get<std::string>());
        if (!pc || !cartridge || *pc == *cartridge) {
            continue;
        }
        std::optional<std::string> failure;
        if (entry.contains("meshes")) {
            failure = SampleInMeshes(defaults, boss, entry, *cartridge);
        } else {
            failure = WriteChecked(boss / entry["pc"].get<std::string>(), *pc);
            if (!failure) {
                failure = WriteChecked(pcPath, *cartridge);
            }
        }
        if (failure) {
            return failure;
        }
    }
    if (auto failure =
            WriteJson(defaults / "pack.json",
                      {{"version", PACK_SETTINGS_VERSION}, {"xmMusicGain", DEFAULT_XM_MUSIC_GAIN}},
                      2)) {
        return failure;
    }
    std::error_code error;
    if (std::filesystem::is_directory(boss, error)) {
        return WriteJson(boss / "pack.json", {{"version", PACK_SETTINGS_VERSION}}, 2);
    }
    return std::nullopt;
}

nlohmann::json PackSettings(const std::filesystem::path &data, std::string_view pack) {
    auto settings = nlohmann::json::object();
    for (const auto &directory : {DefaultAssets(data), PackDirectory(data, pack)}) {
        std::ifstream file(directory / "pack.json");
        auto layer = nlohmann::json::parse(file, nullptr, false);
        if (layer.is_object()) {
            settings.update(layer);
        }
    }
    return settings;
}

} // namespace coastline
