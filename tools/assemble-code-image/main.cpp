#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <argparse/argparse.hpp>
#include <nlohmann/json.hpp>

#include "elfobject.h"
#include "fileio.h"
#include "sha1.h"
#include "sha256.h"

namespace {

using Bytes = std::vector<std::uint8_t>;
using Path = std::filesystem::path;

Bytes readBytes(const Path &path) {
    auto result = Tools::Common::readFile(path);
    if (!result) {
        throw std::runtime_error(
            std::format("Cannot read {}: {}", path.string(), result.error().message));
    }
    return std::move(*result);
}

nlohmann::json readJson(const Path &path) {
    const auto bytes = readBytes(path);
    return nlohmann::json::parse(bytes.begin(), bytes.end());
}

std::map<std::uint32_t, std::uint32_t> functionInventory(const Path &path) {
    const auto bytes = readBytes(path);
    const std::string text(bytes.begin(), bytes.end());
    const std::regex expression(R"((\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+))");
    std::map<std::uint32_t, std::uint32_t> result;
    for (auto item = std::sregex_iterator(text.begin(), text.end(), expression);
         item != std::sregex_iterator();
         ++item) {
        result[std::stoul((*item)[2], nullptr, 16)] = std::stoul((*item)[3], nullptr, 16);
    }
    return result;
}

std::map<std::uint32_t, std::uint32_t> patchMap(const std::string &patches) {
    if (patches.size() % 16 != 0) {
        throw std::runtime_error("A relocation manifest has an invalid patch length.");
    }
    std::map<std::uint32_t, std::uint32_t> result;
    for (std::size_t index = 0; index < patches.size(); index += 16) {
        const auto position = std::stoul(patches.substr(index, 8), nullptr, 16);
        const auto bits = std::stoul(patches.substr(index + 8, 8), nullptr, 16);
        if (!result.emplace(position, bits).second) {
            throw std::runtime_error("A relocation manifest repeats a patch position.");
        }
    }
    return result;
}

void assemble(const Path &sourceRoot,
              const Path &build,
              const Path &reference,
              const Path &output,
              const std::string &region) {
    const auto matching = sourceRoot / "tools/matching";
    const auto baseline = readJson(matching / "code_image.json").at(region);
    const auto codeStart = baseline.at("code_start").get<std::uint32_t>();
    const auto codeEnd = baseline.at("code_end").get<std::uint32_t>();
    const auto imageBase = baseline.at("vram").get<std::uint32_t>();
    const auto imageSize = baseline.at("rom_size").get<std::size_t>();
    if (codeStart >= codeEnd || codeEnd > imageSize) {
        throw std::runtime_error("The regional code-image bounds are invalid.");
    }
    const auto inventory =
        functionInventory(sourceRoot / "tools/splat" / std::format("symbol_addrs.{}.txt", region));
    std::map<std::uint32_t, nlohmann::json> functions;
    for (const auto &part : {0, 1}) {
        const auto manifest =
            readJson(matching / std::format("code_relocations.{}.{}.json", region, part));
        if (manifest.at("region") != region) {
            throw std::runtime_error("A relocation manifest describes another region.");
        }
        for (const auto &entry : manifest.at("functions")) {
            const auto address = entry.at(0).get<std::uint32_t>();
            if (!functions.emplace(address, entry).second) {
                throw std::runtime_error("A relocation manifest repeats a function.");
            }
        }
    }
    if (functions.size() != inventory.size()) {
        throw std::runtime_error("The relocation manifest does not cover every function.");
    }
    Bytes image(imageSize);
    Bytes claimed(codeEnd - codeStart);
    std::map<Path, Tools::Common::ElfObject> objects;
    const auto absoluteBuild = std::filesystem::weakly_canonical(build);
    for (const auto &[address, entry] : functions) {
        const auto expectedSize = inventory.find(address);
        if (expectedSize == inventory.end() ||
            entry.at(3).get<std::uint32_t>() != expectedSize->second) {
            throw std::runtime_error(
                "The relocation manifest differs from the function inventory.");
        }
        const auto relative = Path(entry.at(1).get<std::string>());
        const auto objectPath = std::filesystem::weakly_canonical(build / relative);
        const auto withinBuild = objectPath.lexically_relative(absoluteBuild);
        if (relative.is_absolute() || withinBuild.empty() || *withinBuild.begin() == "..") {
            throw std::runtime_error("An object path escapes the build directory.");
        }
        const auto [object, inserted] = objects.try_emplace(objectPath);
        if (inserted) {
            object->second = Tools::Common::readElfObject(objectPath);
        }
        const auto &code = object->second.text;
        const auto &relocations = object->second.relocations;
        const auto offset = entry.at(2).get<std::size_t>();
        const auto size = expectedSize->second;
        if (offset > code.size() || offset % 4 || size == 0 || size % 4) {
            throw std::runtime_error("A function has an invalid object text range.");
        }
        const auto available = std::min<std::size_t>(size, code.size() - offset);
        Bytes data(size);
        std::copy_n(code.begin() + offset, available, data.begin());
        const auto patches = patchMap(entry.at(4).get<std::string>());
        std::set<std::uint32_t> actual;
        for (std::size_t position = 0; position < available; position += 4) {
            if (relocations.contains(offset + position)) {
                actual.insert(position);
            }
        }
        std::set<std::uint32_t> recorded;
        for (const auto &patch : patches) {
            recorded.insert(patch.first);
        }
        if (recorded != actual) {
            throw std::runtime_error("A function's relocations changed.");
        }
        for (const auto &[position, bits] : patches) {
            const auto mask = relocations.at(offset + position);
            if (bits & mask) {
                throw std::runtime_error("A relocation overlaps an instruction.");
            }
            const auto word = Tools::Common::readBigEndian32(data, position);
            const auto restored = (word & mask) | bits;
            for (std::size_t index = 0; index < 4; ++index) {
                data[position + index] = static_cast<std::uint8_t>(restored >> (24 - index * 8));
            }
        }
        const auto start = static_cast<std::int64_t>(address) - imageBase;
        if (start < 0 || static_cast<std::uint64_t>(start) + size > claimed.size() ||
            std::any_of(claimed.begin() + start,
                        claimed.begin() + start + size,
                        [](std::uint8_t value) { return value != 0; })) {
            throw std::runtime_error("A function has an invalid or overlapping ROM range.");
        }
        std::fill_n(claimed.begin() + start, size, 1);
        std::copy(data.begin(), data.end(), image.begin() + codeStart + start);
    }
    const auto codeHash = Tools::Common::sha256Hex(image);
    if (codeHash != baseline.at("sha256").get<std::string>()) {
        throw std::runtime_error("The assembled code image differs from the SHA-256 baseline.");
    }
    if (!reference.empty()) {
        auto original = readBytes(reference);
        const auto originalHash = baseline.at("sha1").get<std::string>();
        if (original.size() != image.size() || Tools::Common::sha1Hex(original) != originalHash) {
            throw std::runtime_error("The reference ROM differs from the verified original.");
        }
        std::copy(image.begin() + codeStart, image.begin() + codeEnd, original.begin() + codeStart);
        if (Tools::Common::sha1Hex(original) != originalHash) {
            throw std::runtime_error("The assembled ROM differs from the verified original.");
        }
        image = std::move(original);
    }
    std::filesystem::create_directories(output.parent_path());
    if (const auto written = Tools::Common::writeFile(output, image); !written) {
        throw std::runtime_error(
            std::format("Cannot write {}: {}", output.string(), written.error().message));
    }
    std::cout << std::format("Verified {} functions, code SHA-256 {}.\nWrote {}.\n",
                             functions.size(),
                             codeHash,
                             output.string());
}

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser("assemble-code-image");
    parser.add_argument("--region").required();
    parser.add_argument("--source-root").required();
    parser.add_argument("--build").required();
    parser.add_argument("--output").required();
    parser.add_argument("--reference").default_value(std::string{});
    try {
        parser.parse_args(argc, argv);
        const auto region = parser.get<std::string>("--region");
        if (region != "us" && region != "asia" && region != "eu" && region != "jp") {
            throw std::runtime_error("The region must be us, asia, eu, or jp.");
        }
        assemble(parser.get<std::string>("--source-root"),
                 parser.get<std::string>("--build"),
                 parser.get<std::string>("--reference"),
                 parser.get<std::string>("--output"),
                 region);
    } catch (const std::exception &error) {
        std::cerr << "assemble-code-image: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
