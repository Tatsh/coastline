#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>
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
using Candidate = std::tuple<int, int, std::string, std::size_t, std::size_t, std::string>;
using Group = std::tuple<std::string, std::size_t, std::size_t, std::string>;

struct Function {
    std::string name;
    std::uint32_t address;
    std::uint32_t size;
};

struct CodeRange {
    std::string name;
    std::size_t offset;
    std::size_t length;
};

Bytes readBytes(const Path &path) {
    const auto contents = Tools::Common::readFile(path);
    if (!contents) {
        throw std::runtime_error(
            std::format("Cannot read {}: {}", path.string(), contents.error().message));
    }
    return *contents;
}

nlohmann::json readJson(const Path &path) {
    const auto bytes = readBytes(path);
    return nlohmann::json::parse(bytes.begin(), bytes.end());
}

std::map<std::uint32_t, Function> inventory(const Path &path) {
    const auto bytes = readBytes(path);
    const std::string text(bytes.begin(), bytes.end());
    const std::regex expression(R"((\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+))");
    std::map<std::uint32_t, Function> result;
    for (auto item = std::sregex_iterator(text.begin(), text.end(), expression);
         item != std::sregex_iterator();
         ++item) {
        const auto address = static_cast<std::uint32_t>(std::stoul((*item)[2], nullptr, 16));
        result.emplace(address,
                       Function{(*item)[1],
                                address,
                                static_cast<std::uint32_t>(std::stoul((*item)[3], nullptr, 16))});
    }
    return result;
}

void append32(Bytes &output, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        output.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

std::vector<CodeRange> codeRanges(const Tools::Common::ElfObject &object) {
    std::vector<CodeRange> result;
    for (const auto &symbol : object.symbols) {
        if (symbol.function && symbol.size &&
            static_cast<std::uint64_t>(symbol.offset) + symbol.size <= object.text.size()) {
            result.push_back({symbol.name, symbol.offset, symbol.size});
        }
    }
    std::stable_sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
        return left.offset < right.offset;
    });
    if (result.empty()) {
        return result;
    }
    std::vector<CodeRange> gaps;
    if (result.front().offset) {
        gaps.push_back({"^.." + result.front().name, 0, result.front().offset});
    }
    for (std::size_t index = 1; index < result.size(); ++index) {
        const auto end = result[index - 1].offset + result[index - 1].length;
        if (result[index].offset > end) {
            gaps.push_back({result[index - 1].name + ".." + result[index].name,
                            end,
                            result[index].offset - end});
        }
    }
    const auto end = result.back().offset + result.back().length;
    if (end < object.text.size()) {
        auto tail = object.text.size() - (object.text.size() - end) % 4;
        while (tail >= end + 4 && Tools::Common::readBigEndian32(object.text, tail - 4) == 0) {
            tail -= 4;
        }
        if (tail > end) {
            gaps.push_back({result.back().name + "..$", end, tail - end});
        }
    }
    result.insert(result.end(), gaps.begin(), gaps.end());
    return result;
}

std::string digest(const Tools::Common::ElfObject &object, const CodeRange &range) {
    if (range.length % 4 || range.length > object.text.size() - range.offset) {
        throw std::runtime_error("An object has an invalid function text range.");
    }
    Bytes bytes;
    bytes.reserve(4 + range.length * 2);
    append32(bytes, range.length / 4);
    for (std::size_t index = 0; index < range.length; index += 4) {
        const auto position = range.offset + index;
        const auto mask =
            object.relocations.contains(position) ? object.relocations.at(position) : 0xffffffff;
        append32(bytes, mask);
        append32(bytes, Tools::Common::readBigEndian32(object.text, position) & mask);
    }
    return Tools::Common::sha256Hex(bytes);
}

void generate(const Path &sourceRoot,
              const Path &build,
              const Path &romPath,
              const Path &output,
              const std::string &region) {
    const auto matching = sourceRoot / "tools/matching";
    const auto baseline = readJson(matching / "code_image.json").at(region);
    const auto rom = readBytes(romPath);
    if (Tools::Common::sha1Hex(rom) != baseline.at("sha1").get<std::string>()) {
        throw std::runtime_error("The reference ROM differs from the verified original.");
    }
    const auto functions =
        inventory(sourceRoot / "tools/splat" / std::format("symbol_addrs.{}.txt", region));
    std::map<std::string, std::vector<std::pair<std::uint32_t, std::string>>> expected;
    const auto records = readJson(matching / (region + ".json")).at("functions");
    for (auto item = records.begin(); item != records.end(); ++item) {
        const auto address = static_cast<std::uint32_t>(std::stoul(item.key(), nullptr, 16));
        for (auto source = item.value().at("sources").begin();
             source != item.value().at("sources").end();
             ++source) {
            for (const auto &hash : source.value()) {
                expected[hash.get<std::string>()].emplace_back(address, source.key());
            }
        }
    }
    std::map<std::string, std::vector<std::uint32_t>> addressesByName;
    for (const auto &[address, function] : functions) {
        addressesByName[function.name].push_back(address);
    }
    const std::regex selectedObject(
        R"(CMakeFiles/coastline_(?:n64|common|zlib|libultra_\w+|common_libultra_\w+|main|vehiclestatus)\.dir/)");
    std::map<std::uint32_t, std::vector<Candidate>> choices;
    std::map<std::string, Tools::Common::ElfObject> objects;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(build / "src")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".obj") {
            continue;
        }
        const auto relative = entry.path().lexically_relative(build).generic_string();
        if (!std::regex_search(relative, selectedObject)) {
            continue;
        }
        auto object = Tools::Common::readElfObject(entry.path());
        std::map<std::int64_t, int> bases;
        for (const auto &symbol : object.symbols) {
            if (const auto names = addressesByName.find(symbol.name);
                names != addressesByName.end()) {
                for (const auto address : names->second) {
                    bases[static_cast<std::int64_t>(address) - symbol.offset]++;
                }
            }
        }
        for (const auto &range : codeRanges(object)) {
            const auto hash = digest(object, range);
            for (const auto &[address, source] : expected[hash]) {
                if (entry.path().filename() != Path(source).filename().string() + ".obj") {
                    continue;
                }
                const auto base = static_cast<std::int64_t>(address) - range.offset;
                const auto rank = range.name == functions.at(address).name ? 0 : 1;
                choices[address].emplace_back(
                    rank, -bases[base], relative, range.offset, range.length, range.name);
            }
        }
        objects.emplace(relative, std::move(object));
    }
    if (choices.size() != functions.size()) {
        throw std::runtime_error("Some functions have no matching IDO object code.");
    }
    std::map<Group, std::vector<std::uint32_t>> groups;
    for (auto &[address, candidates] : choices) {
        std::sort(candidates.begin(), candidates.end());
        const auto &[rank, base, relative, offset, length, name] = candidates.front();
        groups[{relative, offset, length, name}].push_back(address);
    }
    const auto codeStart = baseline.at("code_start").get<std::uint32_t>();
    const auto codeEnd = baseline.at("code_end").get<std::uint32_t>();
    const auto imageBase = baseline.at("vram").get<std::uint32_t>();
    Bytes image(baseline.at("rom_size").get<std::size_t>());
    Bytes covered(codeEnd - codeStart);
    std::vector<nlohmann::json> manifestEntries;
    for (const auto &[group, addresses] : groups) {
        const auto &[relative, offset, length, name] = group;
        const auto &object = objects.at(relative);
        const auto first = *std::min_element(addresses.begin(), addresses.end());
        for (const auto address : addresses) {
            const auto size = functions.at(address).size;
            const auto delta = static_cast<std::size_t>(address - first);
            if (delta > length || offset + delta > object.text.size()) {
                throw std::runtime_error("A function has an invalid object text range.");
            }
            const auto sourceOffset = offset + delta;
            const auto available =
                std::min<std::size_t>({size, length - delta, object.text.size() - sourceOffset});
            const auto start = static_cast<std::int64_t>(address) - imageBase;
            if (start < 0 || static_cast<std::uint64_t>(start) + size > covered.size() ||
                std::any_of(covered.begin() + start,
                            covered.begin() + start + size,
                            [](std::uint8_t value) { return value != 0; })) {
                throw std::runtime_error("A function has an invalid or overlapping ROM range.");
            }
            const auto romOffset = codeStart + start;
            if (std::any_of(rom.begin() + romOffset + available,
                            rom.begin() + romOffset + size,
                            [](std::uint8_t value) { return value != 0; })) {
                throw std::runtime_error("A function has nonzero code beyond its object range.");
            }
            std::string patches;
            Bytes data(size);
            std::copy_n(object.text.begin() + sourceOffset, available, data.begin());
            for (std::size_t position = 0; position < available; position += 4) {
                const auto objectPosition = sourceOffset + position;
                const auto mask = object.relocations.contains(objectPosition) ?
                                      object.relocations.at(objectPosition) :
                                      0xffffffff;
                const auto word = Tools::Common::readBigEndian32(data, position);
                const auto original = Tools::Common::readBigEndian32(rom, romOffset + position);
                if ((word & mask) != (original & mask)) {
                    throw std::runtime_error(
                        std::format("Compiled code differs at {:#x}.", address + position));
                }
                if (mask != 0xffffffff) {
                    patches += std::format("{:08x}{:08x}", position, original & ~mask);
                    const auto restored = (word & mask) | (original & ~mask);
                    for (std::size_t index = 0; index < 4; ++index) {
                        data[position + index] =
                            static_cast<std::uint8_t>(restored >> (24 - 8 * index));
                    }
                }
            }
            std::copy(data.begin(), data.end(), image.begin() + romOffset);
            std::fill_n(covered.begin() + start, size, 1);
            manifestEntries.push_back(
                nlohmann::json::array({address, relative, sourceOffset, size, patches}));
        }
    }
    if (manifestEntries.size() != functions.size() ||
        Tools::Common::sha256Hex(image) != baseline.at("sha256").get<std::string>()) {
        throw std::runtime_error("The generated code image differs from the SHA-256 baseline.");
    }
    if (!output.parent_path().empty()) {
        std::filesystem::create_directories(output.parent_path());
    }
    const auto middle = manifestEntries.size() / 2;
    for (std::size_t part = 0; part < 2; ++part) {
        const auto first = part == 0 ? manifestEntries.begin() : manifestEntries.begin() + middle;
        const auto last = part == 0 ? manifestEntries.begin() + middle : manifestEntries.end();
        const auto path =
            output.parent_path() / std::format("{}.{}.json", output.stem().string(), part);
        auto entries = nlohmann::json::array();
        for (auto item = first; item != last; ++item) {
            entries.push_back(*item);
        }
        const nlohmann::json manifest{{"functions", std::move(entries)}, {"region", region}};
        if (const auto written = Tools::Common::writeFile(path, manifest.dump(2) + '\n');
            !written) {
            throw std::runtime_error(
                std::format("Cannot write {}: {}", path.string(), written.error().message));
        }
        std::cout << std::format(
            "Recorded {} verified functions in {}.\n", last - first, path.string());
    }
}

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser("generate-code-relocations");
    parser.add_argument("--source-root").required();
    parser.add_argument("--region").required();
    parser.add_argument("--rom").required();
    parser.add_argument("--build").required();
    parser.add_argument("--output").required();
    try {
        parser.parse_args(argc, argv);
        const auto region = parser.get<std::string>("--region");
        if (region != "us" && region != "asia" && region != "eu" && region != "jp") {
            throw std::runtime_error("The region must be us, asia, eu, or jp.");
        }
        generate(parser.get<std::string>("--source-root"),
                 parser.get<std::string>("--build"),
                 parser.get<std::string>("--rom"),
                 parser.get<std::string>("--output"),
                 region);
    } catch (const std::exception &error) {
        std::cerr << "generate-code-relocations: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
