#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
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
using Json = nlohmann::json;

struct Range {
    std::string name;
    std::size_t offset;
    std::size_t length;
};

struct Candidate {
    std::vector<std::string> keys;
    std::uint32_t address;
    std::size_t size;
};

struct Options {
    Path root;
    Path ido;
    Path bin;
    Path encoder;
    Path baseline;
    Path rom;
    std::string region;
    bool verbose;
};

struct Context {
    Options options;
    Path scratch;
    std::size_t idoCalls = 0;
    std::size_t assemblerCalls = 0;
    std::string lastError;
};

struct ScratchCleanup {
    Path path;
    ~ScratchCleanup() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

Bytes readBytes(const Path &path) {
    const auto result = Tools::Common::readFile(path);
    if (!result) {
        throw std::runtime_error(
            std::format("Cannot read {}: {}", path.string(), result.error().message));
    }
    return *result;
}

std::string readText(const Path &path) {
    const auto bytes = readBytes(path);
    return {bytes.begin(), bytes.end()};
}

Json readJson(const Path &path) {
    const auto bytes = readBytes(path);
    return Json::parse(bytes.begin(), bytes.end());
}

std::string quote(const std::string &value) {
    std::string quoted = "'";
    for (const char character : value) {
        quoted += character == '\'' ? "'\\''" : std::string(1, character);
    }
    return quoted + "'";
}

int run(Context &context, const std::vector<std::string> &args) {
    std::string command;
    for (const auto &argument : args) {
        command += quote(argument) + ' ';
    }
    if (context.options.verbose) {
        std::cerr << "+ " << command << '\n';
    }
    const auto log = context.scratch / "compiler.log";
    command = "cd " + quote(context.scratch.string()) + " && " + command;
    command += " >" + quote(log.string()) + " 2>&1";
    const int status = std::system(command.c_str());
    context.lastError = readText(log);
    return status;
}

Path binutil(const Context &context, const std::string &name) {
    for (const auto *prefix : {"mips64-elf-", "mips64-libdragon-elf-"}) {
        const auto path = context.options.bin / (prefix + name);
        if (std::filesystem::is_regular_file(path)) {
            return path;
        }
    }
    throw std::runtime_error(
        std::format("Cannot find {} under {}.", name, context.options.bin.string()));
}

std::map<std::string, std::pair<std::uint32_t, std::uint32_t>> inventory(const Options &options) {
    const auto text =
        readText(options.root / "tools/splat" / std::format("symbol_addrs.{}.txt", options.region));
    const std::regex pattern(R"((\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+))");
    std::map<std::string, std::pair<std::uint32_t, std::uint32_t>> result;
    for (auto item = std::sregex_iterator(text.begin(), text.end(), pattern);
         item != std::sregex_iterator();
         ++item) {
        result[(*item)[1]] = {static_cast<std::uint32_t>(std::stoul((*item)[2], nullptr, 16)),
                              static_cast<std::uint32_t>(std::stoul((*item)[3], nullptr, 16))};
    }
    return result;
}

std::vector<Range> ranges(const Tools::Common::ElfObject &object) {
    std::vector<Range> result;
    for (const auto &symbol : object.symbols) {
        if (symbol.function && symbol.size &&
            static_cast<std::uint64_t>(symbol.offset) + symbol.size <= object.text.size()) {
            result.push_back({symbol.name, symbol.offset, symbol.size});
        }
    }
    std::stable_sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        return a.offset < b.offset;
    });
    if (result.empty()) {
        return result;
    }
    std::vector<Range> gaps;
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

void append32(Bytes &bytes, std::uint32_t word) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        bytes.push_back(static_cast<std::uint8_t>(word >> shift));
    }
}

std::string digest(const Tools::Common::ElfObject &object, const Range &range) {
    Bytes data;
    append32(data, range.length / 4);
    for (std::size_t index = 0; index < range.length; index += 4) {
        const auto offset = range.offset + index;
        const auto mask =
            object.relocations.contains(offset) ? object.relocations.at(offset) : 0xffffffff;
        append32(data, mask);
        append32(data, Tools::Common::readBigEndian32(object.text, offset) & mask);
    }
    return Tools::Common::sha256Hex(data);
}

std::uint32_t registerFields(std::uint32_t word) {
    switch (word >> 26) {
    case 0:
        return 0x03fff800;
    case 1:
        return 0x03e00000;
    case 2:
    case 3:
        return 0;
    case 0x11:
        return 0x001fffc0;
    default:
        return 0x03ff0000;
    }
}

std::pair<bool, std::size_t> matches(const Tools::Common::ElfObject &object,
                                     const Range &range,
                                     std::uint32_t address,
                                     std::size_t size,
                                     const Bytes &rom) {
    const auto start = static_cast<std::int64_t>(address) - 0x80200000 + 0x1000;
    if (start < 0 || static_cast<std::uint64_t>(start) + size > rom.size()) {
        throw std::runtime_error("A function range exceeds the reference ROM.");
    }
    auto length = range.length;
    while (length > size && length >= 4 &&
           Tools::Common::readBigEndian32(object.text, range.offset + length - 4) == 0) {
        length -= 4;
    }
    std::size_t matched = 0;
    for (std::size_t index = 0; index < std::min(length, size); index += 4) {
        const auto offset = range.offset + index;
        const auto mask =
            object.relocations.contains(offset) ? object.relocations.at(offset) : 0xffffffff;
        const auto compiled = Tools::Common::readBigEndian32(object.text, offset);
        const auto original = Tools::Common::readBigEndian32(rom, start + index);
        if ((compiled & mask) == (original & mask)) {
            matched += 4;
        }
    }
    return {length == size && matched == size, matched};
}

std::vector<std::string> defines(const std::string &region) {
    const std::map<std::string, int> values{{"us", 1}, {"jp", 2}, {"eu", 3}, {"asia", 4}};
    return {"-DTGR_PLATFORM_N64=1",
            "-DTGR_PLATFORM_WIN=2",
            "-DTGR_PLATFORM=1",
            "-DTGR_REGION_US=1",
            "-DTGR_REGION_JP=2",
            "-DTGR_REGION_EU=3",
            "-DTGR_REGION_ASIA=4",
            std::format("-DTGR_REGION={}", values.at(region)),
            "-DTGR_RENDERER_D3D=1",
            "-DTGR_RENDERER_GLIDE=2",
            "-DTGR_RENDERER=0",
            "-D_LANGUAGE_C",
            "-DF3DEX_GBI",
            "-D_MIPS_SZLONG=32",
            "-D__sgi",
            "-DNDEBUG"};
}

std::string globRegex(const std::string &glob) {
    std::string pattern = "^";
    for (const char character : glob) {
        if (character == '*') {
            pattern += ".*";
        } else if (character == '?') {
            pattern += '.';
        } else if (std::string(".()[]{}+^$|\\").find(character) != std::string::npos) {
            pattern += '\\';
            pattern += character;
        } else {
            pattern += character;
        }
    }
    return pattern + '$';
}

std::vector<std::vector<std::string>> flagSets(const Options &options, const Path &source) {
    const auto relative = source.lexically_relative(options.root).generic_string();
    std::istringstream input(readText(options.root / "tools/ido_flags.txt"));
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto split = line.find_first_of(" \t");
        if (split == std::string::npos ||
            !std::regex_match(relative, std::regex(globRegex(line.substr(0, split))))) {
            continue;
        }
        std::vector<std::vector<std::string>> result;
        std::istringstream alternatives(line.substr(split + 1));
        std::string alternative;
        while (std::getline(alternatives, alternative, '|')) {
            std::istringstream words(alternative);
            std::vector<std::string> flags;
            for (std::string word; words >> word;) {
                flags.push_back(word);
            }
            result.push_back(std::move(flags));
        }
        return result;
    }
    return {{"-O2"}};
}

bool compile(Context &context,
             const Path &source,
             std::vector<std::string> flags,
             const Path &object) {
    const auto root = context.options.root;
    Path input = source;
    if (context.options.region == "jp") {
        const auto bytes = readBytes(source);
        if (std::any_of(bytes.begin(), bytes.end(), [](auto byte) { return byte > 127; })) {
            if (context.options.encoder.empty()) {
                throw std::runtime_error("Japanese text requires --encoder.");
            }
            input = context.scratch / source.filename();
            if (run(context, {context.options.encoder.string(), source.string(), input.string()})) {
                throw std::runtime_error(context.lastError);
            }
        }
    }
    if (std::none_of(flags.begin(), flags.end(), [](const auto &flag) {
            return flag.starts_with("-mips");
        })) {
        flags.insert(flags.begin(), "-mips2");
    }
    std::vector<std::string> command{(context.options.ido / "cc").string(),
                                     "-c",
                                     "-nostdinc",
                                     "-non_shared",
                                     "-G",
                                     "0",
                                     "-Xcpluscomm"};
    command.insert(command.end(), flags.begin(), flags.end());
    command.push_back("-Wab,-r4300_mul");
    for (const auto &include : {root / "tools/ido-include",
                                root / "src",
                                root / "src/n64",
                                root / "src/common/libultra/include",
                                root / "src/n64/libultra/include",
                                root / "3rdparty/zlib-1.0.4",
                                source.parent_path()}) {
        command.push_back("-I" + include.string());
    }
    const auto macros = defines(context.options.region);
    command.insert(command.end(), macros.begin(), macros.end());
    command.insert(command.end(), {"-o", object.string(), input.string()});
    ++context.idoCalls;
    return run(context, command) == 0;
}

bool assemble(Context &context, const Path &source, const Path &object) {
    const auto root = context.options.root;
    std::vector<std::string> command{binutil(context, "gcc").string(),
                                     "-c",
                                     "-march=vr4300",
                                     "-mabi=32",
                                     "-mno-abicalls",
                                     "-fno-pic",
                                     "-G0",
                                     "-I" + (root / "src/common/libultra/include").string(),
                                     "-I" + (root / "src").string()};
    const auto macros = defines(context.options.region);
    command.insert(command.end(), macros.begin(), macros.end());
    command.insert(command.end(),
                   {"-x", "assembler-with-cpp", "-o", object.string(), source.string()});
    ++context.assemblerCalls;
    return run(context, command) == 0;
}

std::vector<Path> sources(const Options &options, const Json *expected) {
    std::vector<Path> result;
    if (expected) {
        std::set<std::string> names;
        for (auto item = expected->begin(); item != expected->end(); ++item) {
            for (auto source = item.value().at("sources").begin();
                 source != item.value().at("sources").end();
                 ++source) {
                names.insert(source.key());
            }
        }
        for (const auto &name : names) {
            result.push_back(options.root / name);
        }
        return result;
    }
    for (const auto &entry : std::filesystem::recursive_directory_iterator(options.root / "src")) {
        if (!entry.is_regular_file() ||
            (entry.path().extension() != ".c" && entry.path().extension() != ".s")) {
            continue;
        }
        const auto relative = entry.path().lexically_relative(options.root);
        bool excluded = false;
        for (const auto &part : relative) {
            if (part == "platform" || part == "asm") {
                excluded = true;
            }
        }
        if (!excluded && entry.path().filename() != "divdi3.c") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    for (const auto *name : {"adler32.c",
                             "infblock.c",
                             "infcodes.c",
                             "inffast.c",
                             "inflate.c",
                             "inftrees.c",
                             "infutil.c",
                             "uncompr.c",
                             "zutil.c"}) {
        result.push_back(options.root / "3rdparty/zlib-1.0.4" / name);
    }
    return result;
}

Candidate candidate(const Range &range,
                    const std::map<std::string, std::pair<std::uint32_t, std::uint32_t>> &table,
                    const std::vector<std::pair<std::uint32_t, std::string>> &byAddress) {
    std::uint32_t start = 0;
    std::uint32_t end = 0;
    const auto separator = range.name.find("..");
    if (separator == std::string::npos) {
        if (const auto found = table.find(range.name); found != table.end()) {
            return {{std::format("0x{:08x}", found->second.first)},
                    found->second.first,
                    found->second.second};
        }
        return {};
    }
    const auto before = range.name.substr(0, separator);
    const auto after = range.name.substr(separator + 2);
    if (before == "^" && table.contains(after)) {
        end = table.at(after).first;
        start = end - range.length;
    } else if (after == "$" && table.contains(before)) {
        start = table.at(before).first + table.at(before).second;
        end = start + range.length;
    } else if (table.contains(before) && table.contains(after)) {
        start = table.at(before).first + table.at(before).second;
        end = table.at(after).first;
    } else {
        return {};
    }
    std::vector<std::string> result;
    for (const auto &[address, name] : byAddress) {
        if (start <= address && address < end) {
            result.push_back(std::format("0x{:08x}", address));
        }
    }
    return {result, start, end - start};
}

int verify(Context &context) {
    const auto &options = context.options;
    const std::map<std::string, std::string> romNames{
        {"us", "usa.z64"}, {"jp", "jpn.z64"}, {"eu", "eur.z64"}, {"asia", "asia.z64"}};
    const auto expectedSha = readJson(options.root / "tools/matching/code_image.json")
                                 .at(options.region)
                                 .at("sha1")
                                 .get<std::string>();
    std::optional<Bytes> rom;
    std::optional<Json> expected;
    if (!options.rom.empty()) {
        rom = readBytes(options.rom);
        if (Tools::Common::sha1Hex(*rom) != expectedSha) {
            throw std::runtime_error(std::format("{} is not the verified {} image.",
                                                 options.rom.string(),
                                                 romNames.at(options.region)));
        }
    } else {
        const auto baseline = options.baseline.empty() ?
                                  options.root / "tools/matching" / (options.region + ".json") :
                                  options.baseline;
        const auto file = readJson(baseline);
        if (file.at("region") != options.region || file.at("rom_sha1") != expectedSha) {
            throw std::runtime_error("The matching baseline describes another ROM.");
        }
        expected = file.at("functions");
    }
    const auto table = inventory(options);
    std::vector<std::pair<std::uint32_t, std::string>> byAddress;
    for (const auto &[name, address] : table) {
        byAddress.emplace_back(address.first, name);
    }
    std::sort(byAddress.begin(), byAddress.end());
    Json entries = Json::object();
    std::set<std::string> seen;
    std::map<std::string, std::tuple<std::size_t, std::size_t, std::string>> closest;
    const auto object = context.scratch / "out.o";
    for (const auto &source : sources(options, expected ? &*expected : nullptr)) {
        const auto relative = source.lexically_relative(options.root).generic_string();
        bool compiled = false;
        auto flags = source.extension() == ".s" ? std::vector<std::vector<std::string>>{{"as"}} :
                                                  flagSets(options, source);
        for (const auto &alternative : flags) {
            if (!(alternative == std::vector<std::string>{"as"} ?
                      assemble(context, source, object) :
                      compile(context, source, alternative, object))) {
                continue;
            }
            compiled = true;
            const auto elf = Tools::Common::readElfObject(object);
            for (const auto &range : ranges(elf)) {
                const auto selection = candidate(range, table, byAddress);
                const auto &keys = selection.keys;
                if (keys.empty()) {
                    continue;
                }
                for (const auto &key : keys) {
                    seen.insert(key);
                }
                const auto fingerprint = digest(elf, range);
                if (expected) {
                    for (const auto &key : keys) {
                        if (expected->contains(key) &&
                            (*expected)[key]["sources"].contains(relative)) {
                            const auto &hashes = (*expected)[key]["sources"][relative];
                            if (std::find(hashes.begin(), hashes.end(), fingerprint) !=
                                hashes.end()) {
                                entries[key]["sources"][relative] = true;
                            }
                        }
                    }
                } else {
                    const auto [matched, count] =
                        matches(elf, range, selection.address, selection.size, *rom);
                    for (const auto &key : keys) {
                        if (count > std::get<0>(closest[key])) {
                            closest[key] = {count, selection.size, relative};
                        }
                    }
                    if (matched) {
                        for (const auto &key : keys) {
                            auto &hashes = entries[key]["sources"][relative];
                            if (std::find(hashes.begin(), hashes.end(), fingerprint) ==
                                hashes.end()) {
                                hashes.push_back(fingerprint);
                            }
                        }
                    }
                }
            }
        }
        if (!compiled && expected) {
            throw std::runtime_error(
                std::format("{} does not compile with IDO 5.3.\n{}", relative, context.lastError));
        }
    }
    std::cerr << std::format("{} IDO compiler invocations; {} assembler invocations.\n",
                             context.idoCalls,
                             context.assemblerCalls);
    if (!expected) {
        std::size_t missing = 0;
        for (const auto &[name, address] : table) {
            const auto key = std::format("0x{:08x}", address.first);
            if (!entries.contains(key)) {
                const auto [count, size, source] = closest[key];
                std::cerr << std::format(
                    "{} in {}: {}/{} bytes match.\n", key, source, count, size);
                ++missing;
            }
        }
        if (missing) {
            std::cerr << std::format("{} reconstructed routines do not match the ROM.\n", missing);
            return 1;
        }
        for (auto &entry : entries.items()) {
            for (auto &source : entry.value()["sources"].items()) {
                std::sort(source.value().begin(), source.value().end());
            }
        }
        std::cout << Json{{"region", options.region},
                          {"rom_sha1", expectedSha},
                          {"functions", entries}}
                         .dump(2)
                  << '\n';
        return 0;
    }
    std::size_t missing = 0;
    for (auto entry = expected->begin(); entry != expected->end(); ++entry) {
        for (auto source = entry.value()["sources"].begin();
             source != entry.value()["sources"].end();
             ++source) {
            if (!entries.contains(entry.key()) ||
                !entries[entry.key()]["sources"].contains(source.key())) {
                std::cerr << std::format(
                    "{} in {}: compiled code differs from the ROM-matched baseline.\n",
                    entry.key(),
                    source.key());
                ++missing;
            }
        }
    }
    for (const auto &key : seen) {
        if (!expected->contains(key)) {
            std::cerr << std::format("{}: reconstructed routine is absent from the baseline.\n",
                                     key);
            ++missing;
        }
    }
    std::cout << std::format("{}/{} ROM-matched routines verified for {}.\n",
                             entries.size(),
                             expected->size(),
                             options.region);
    return missing ? 1 : 0;
}

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser("check-matching");
    parser.add_argument("--source-root").default_value(std::string("."));
    parser.add_argument("--region").required();
    parser.add_argument("--baseline");
    parser.add_argument("--rom");
    parser.add_argument("--encoder");
    parser.add_argument("--ido-root");
    parser.add_argument("--binutils-root");
    parser.add_argument("--verbose").default_value(false).implicit_value(true);
    try {
        parser.parse_args(argc, argv);
        const auto region = parser.get<std::string>("--region");
        if (region != "us" && region != "jp" && region != "eu" && region != "asia") {
            throw std::runtime_error("The region must be us, jp, eu, or asia.");
        }
        const auto root = std::filesystem::absolute(parser.get<std::string>("--source-root"));
        const auto ido =
            parser.present("--ido-root").value_or((root / ".wiswa-ci/ido/5.3").string());
        const auto bin = parser.present("--binutils-root")
                             .value_or((root / ".wiswa-ci/toolchains/n64/opt/libdragon").string());
        const auto encoder = parser.present("--encoder").value_or("");
        Context context{{root,
                         std::filesystem::absolute(ido),
                         std::filesystem::absolute(bin) / "bin",
                         encoder.empty() ? Path{} : std::filesystem::absolute(encoder),
                         parser.present("--baseline").value_or(""),
                         parser.present("--rom").value_or(""),
                         region,
                         parser.get<bool>("--verbose")},
                        {},
                        0,
                        0,
                        {}};
        if (!std::filesystem::is_regular_file(context.options.ido / "cc")) {
            throw std::runtime_error("IDO 5.3 compiler is missing.");
        }
        std::filesystem::create_directories(root / ".wiswa-ci");
        std::random_device random;
        bool created = false;
        for (int attempt = 0; attempt < 16; ++attempt) {
            context.scratch = root / ".wiswa-ci" / std::format("check-matching-{:08x}", random());
            if (std::filesystem::create_directory(context.scratch)) {
                created = true;
                break;
            }
        }
        if (!created) {
            throw std::runtime_error("Cannot create a matching scratch directory.");
        }
        const ScratchCleanup cleanup{context.scratch};
        return verify(context);
    } catch (const std::exception &error) {
        std::cerr << "check-matching: " << error.what() << '\n';
        return 1;
    }
}
