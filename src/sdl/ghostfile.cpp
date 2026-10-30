#include "ghostfile.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <zlib.h>

namespace coastline {

namespace {

constexpr std::uint8_t MAGIC[4] = {'R', 'G', 'h', 'o'};
constexpr std::size_t INFO_SIZE = 16;
constexpr std::size_t FRAME_SIZE = 24;
constexpr std::size_t SETTINGS = 6;
constexpr std::size_t SLOT_NAME_SIZE = 128;
constexpr std::size_t TRAILER_SIZE = SETTINGS * 4 + SLOT_NAME_SIZE;
// A layout word below this begins the layout with the best times, and others are the size.
constexpr std::uint32_t SIZED_LAYOUT = 12;

constexpr const char *RECORDING_FIELDS[8] = {"track",
                                             "car",
                                             "steeringMode",
                                             "transmission",
                                             "tires",
                                             "suspension",
                                             "controllerLayout",
                                             "weather"};
constexpr const char *SETTING_FIELDS[SETTINGS] = {
    "transmission", "tires", "suspension", "car", "handling", "laps"};

std::uint32_t Read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset] | bytes[offset + 1] << 8 |
                                      bytes[offset + 2] << 16 |
                                      static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint16_t Read16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | bytes[offset + 1] << 8);
}

void Write32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void Write16(std::vector<std::uint8_t> &bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

// A field of bits of a word, sign-extended when signed.
std::int64_t Bits(std::uint32_t word, int first, int count, bool isSigned = false) {
    std::uint32_t value = (word >> first) & ((1u << count) - 1);
    if (isSigned && (value >> (count - 1)) != 0) {
        return static_cast<std::int64_t>(value) - (std::int64_t{1} << count);
    }
    return value;
}

std::uint32_t Place(std::int64_t value, int first, int count) {
    return (static_cast<std::uint32_t>(value) & ((1u << count) - 1)) << first;
}

nlohmann::ordered_json FrameToJson(std::span<const std::uint8_t> frame) {
    nlohmann::ordered_json json;
    // Each half of the orientation lends its lowest bit to a flag of the effects.
    nlohmann::ordered_json orientation = nlohmann::ordered_json::array();
    nlohmann::ordered_json effects = nlohmann::ordered_json::array();
    std::uint32_t x = Read32(frame, 8);
    std::uint32_t y = Read32(frame, 12);
    effects.push_back(Bits(x, 1, 1));
    for (int i = 0; i < 4; ++i) {
        std::uint16_t half = Read16(frame, static_cast<std::size_t>(i) * 2);
        orientation.push_back(Bits(half, 1, 15, true));
        effects.push_back(Bits(half, 0, 1));
    }
    effects.push_back(Bits(x, 2, 1));
    json["orientation"] = orientation;
    json["effects"] = effects;
    json["position"] = {
        Bits(x, 3, 21, true), Bits(y, 2, 22, true), static_cast<std::int16_t>(Read16(frame, 16))};
    json["gear"] = Bits(x, 0, 1);
    json["reverse"] = Bits(y, 0, 1);
    json["braking"] = Bits(y, 1, 1);
    json["wheelAngle"] = Bits(x, 24, 8);
    json["engineRpm"] = Bits(y, 24, 6);
    json["bodyRoll"] = static_cast<std::int8_t>(frame[18]);
    json["steer"] = Bits(frame[19], 0, 6);
    json["lights"] = {Bits(frame[19], 6, 2), Bits(y, 30, 2)};
    nlohmann::ordered_json wheels = nlohmann::ordered_json::array();
    for (int pair = 0; pair < 2; ++pair) {
        std::uint8_t byte = frame[20 + static_cast<std::size_t>(pair)];
        wheels.push_back({Bits(byte, 7, 1), Bits(byte, 4, 3)});
        wheels.push_back({Bits(byte, 3, 1), Bits(byte, 0, 3)});
    }
    json["wheels"] = wheels;
    if (frame[22] != 0 || frame[23] != 0) {
        json["reserved"] = {frame[22], frame[23]};
    }
    return json;
}

void FrameFromJson(std::vector<std::uint8_t> &bytes, const nlohmann::ordered_json &json) {
    const auto &orientation = json.at("orientation");
    const auto &effects = json.at("effects");
    const auto &position = json.at("position");
    const auto &lights = json.at("lights");
    const auto &wheels = json.at("wheels");
    for (std::size_t i = 0; i < 4; ++i) {
        Write16(bytes,
                static_cast<std::uint16_t>(Place(orientation.at(i).get<std::int64_t>(), 1, 15) |
                                           Place(effects.at(i + 1).get<std::int64_t>(), 0, 1)));
    }
    Write32(bytes,
            Place(json.at("gear").get<std::int64_t>(), 0, 1) |
                Place(effects.at(0).get<std::int64_t>(), 1, 1) |
                Place(effects.at(5).get<std::int64_t>(), 2, 1) |
                Place(position.at(0).get<std::int64_t>(), 3, 21) |
                Place(json.at("wheelAngle").get<std::int64_t>(), 24, 8));
    Write32(bytes,
            Place(json.at("reverse").get<std::int64_t>(), 0, 1) |
                Place(json.at("braking").get<std::int64_t>(), 1, 1) |
                Place(position.at(1).get<std::int64_t>(), 2, 22) |
                Place(json.at("engineRpm").get<std::int64_t>(), 24, 6) |
                Place(lights.at(1).get<std::int64_t>(), 30, 2));
    Write16(bytes, static_cast<std::uint16_t>(position.at(2).get<std::int64_t>()));
    bytes.push_back(static_cast<std::uint8_t>(json.at("bodyRoll").get<std::int64_t>()));
    bytes.push_back(static_cast<std::uint8_t>(Place(json.at("steer").get<std::int64_t>(), 0, 6) |
                                              Place(lights.at(0).get<std::int64_t>(), 6, 2)));
    for (std::size_t pair = 0; pair < 2; ++pair) {
        const auto &first = wheels.at(pair * 2);
        const auto &second = wheels.at(pair * 2 + 1);
        bytes.push_back(static_cast<std::uint8_t>(Place(first.at(0).get<std::int64_t>(), 7, 1) |
                                                  Place(first.at(1).get<std::int64_t>(), 4, 3) |
                                                  Place(second.at(0).get<std::int64_t>(), 3, 1) |
                                                  Place(second.at(1).get<std::int64_t>(), 0, 3)));
    }
    auto reserved = json.value("reserved", nlohmann::ordered_json::array({0, 0}));
    bytes.push_back(reserved.at(0).get<std::uint8_t>());
    bytes.push_back(reserved.at(1).get<std::uint8_t>());
}

std::string Hex(std::span<const std::uint8_t> bytes) {
    std::string text;
    for (std::uint8_t byte : bytes) {
        text += fmt::format("{:02x}", byte);
    }
    return text;
}

std::vector<std::uint8_t> FromHex(const std::string &text) {
    std::vector<std::uint8_t> bytes;
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>(std::stoul(text.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

} // namespace

std::optional<nlohmann::ordered_json> GhostToJson(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 12 || !std::equal(std::begin(MAGIC), std::end(MAGIC), bytes.begin())) {
        return std::nullopt;
    }
    std::uint32_t layout = Read32(bytes, 4);
    std::size_t offset = 8;
    std::uint32_t remaining = layout;
    nlohmann::ordered_json json;
    json["format"] = "coastline-ghost";
    json["version"] = 1;
    if (layout < SIZED_LAYOUT) {
        if (layout != 0 || bytes.size() < 12) {
            return std::nullopt;
        }
        remaining = Read32(bytes, offset);
        offset += 4;
    }
    if (remaining < 4 + INFO_SIZE + (layout < SIZED_LAYOUT ? 8 : 0)) {
        return std::nullopt;
    }
    std::size_t payload = offset;
    std::uint32_t checksum = Read32(bytes, offset);
    offset += 4;
    std::size_t timed = offset;
    if (layout < SIZED_LAYOUT) {
        // The times are kept as the bits of their floats, which JSON numbers cannot all write.
        json["bestLapTimeBits"] = Read32(bytes, offset);
        json["bestRaceTimeBits"] = Read32(bytes, offset + 4);
        offset += 8;
    }
    std::size_t end = payload + remaining;
    if (end + TRAILER_SIZE != bytes.size() || (end - offset - INFO_SIZE) % FRAME_SIZE != 0) {
        return std::nullopt;
    }
    uLong expected = adler32(0, nullptr, 0);
    expected = adler32(expected, bytes.data() + timed, static_cast<uInt>(end - timed));
    if (expected != checksum) {
        return std::nullopt;
    }
    nlohmann::ordered_json recording;
    for (std::size_t i = 0; i < 8; ++i) {
        recording[RECORDING_FIELDS[i]] = i == 6 ? static_cast<std::int64_t>(bytes[offset + i]) :
                                                  static_cast<std::int8_t>(bytes[offset + i]);
    }
    recording["reserved"] = {Read32(bytes, offset + 8), Read32(bytes, offset + 12)};
    json["recording"] = recording;
    offset += INFO_SIZE;
    nlohmann::ordered_json settings;
    for (std::size_t i = 0; i < SETTINGS; ++i) {
        settings[SETTING_FIELDS[i]] = static_cast<std::int32_t>(Read32(bytes, end + i * 4));
    }
    json["settings"] = settings;
    auto slot = bytes.subspan(end + SETTINGS * 4, SLOT_NAME_SIZE);
    auto terminator = std::ranges::find(slot, 0);
    if (std::any_of(slot.begin(), terminator, [](std::uint8_t c) { return c >= 0x80; })) {
        return std::nullopt;
    }
    json["slotName"] = std::string(slot.begin(), terminator);
    // The bytes after the terminator of the name, which the original leaves as they were, to the
    // last that is not zero.
    auto last = std::find_if(slot.rbegin(), slot.rend(), [](std::uint8_t c) { return c != 0; });
    if (terminator != slot.end() && last.base() > terminator + 1) {
        json["slotNameTail"] = Hex(std::span(terminator + 1, last.base()));
    }
    nlohmann::ordered_json frames = nlohmann::ordered_json::array();
    for (; offset < end; offset += FRAME_SIZE) {
        frames.push_back(FrameToJson(bytes.subspan(offset, FRAME_SIZE)));
    }
    json["frames"] = frames;
    return json;
}

std::optional<std::vector<std::uint8_t>> GhostFromJson(const nlohmann::ordered_json &json) {
    try {
        if (json.at("format") != "coastline-ghost" || json.at("version") != 1) {
            return std::nullopt;
        }
        bool timed = json.contains("bestLapTimeBits");
        std::vector<std::uint8_t> body;
        if (timed) {
            Write32(body, json.at("bestLapTimeBits").get<std::uint32_t>());
            Write32(body, json.at("bestRaceTimeBits").get<std::uint32_t>());
        }
        const auto &recording = json.at("recording");
        for (const char *field : RECORDING_FIELDS) {
            body.push_back(static_cast<std::uint8_t>(recording.at(field).get<std::int64_t>()));
        }
        Write32(body, recording.at("reserved").at(0).get<std::uint32_t>());
        Write32(body, recording.at("reserved").at(1).get<std::uint32_t>());
        for (const auto &frame : json.at("frames")) {
            FrameFromJson(body, frame);
        }
        uLong checksum = adler32(0, nullptr, 0);
        checksum = adler32(checksum, body.data(), static_cast<uInt>(body.size()));
        std::vector<std::uint8_t> bytes(std::begin(MAGIC), std::end(MAGIC));
        auto remaining = static_cast<std::uint32_t>(body.size() + 4);
        if (timed) {
            Write32(bytes, 0);
        }
        Write32(bytes, remaining);
        Write32(bytes, static_cast<std::uint32_t>(checksum));
        bytes.insert(bytes.end(), body.begin(), body.end());
        for (const char *field : SETTING_FIELDS) {
            Write32(bytes,
                    static_cast<std::uint32_t>(json.at("settings").at(field).get<std::int32_t>()));
        }
        std::string name = json.at("slotName").get<std::string>();
        std::vector<std::uint8_t> slot(name.begin(), name.end());
        slot.push_back(0);
        if (json.contains("slotNameTail")) {
            auto tail = FromHex(json.at("slotNameTail").get<std::string>());
            slot.insert(slot.end(), tail.begin(), tail.end());
        }
        if (slot.size() > SLOT_NAME_SIZE) {
            return std::nullopt;
        }
        slot.resize(SLOT_NAME_SIZE, 0);
        bytes.insert(bytes.end(), slot.begin(), slot.end());
        return bytes;
    } catch (const nlohmann::json::exception &) {
        return std::nullopt;
    }
}

std::string FormatGhostJson(const nlohmann::ordered_json &json) {
    nlohmann::ordered_json header = json;
    header.erase("frames");
    std::string text = header.dump(2);
    text.erase(text.find_last_not_of("\n}") + 1);
    text += ",\n  \"frames\": [\n";
    const auto &frames = json.at("frames");
    for (std::size_t i = 0; i < frames.size(); ++i) {
        text += "    " + frames[i].dump() + (i + 1 < frames.size() ? ",\n" : "\n");
    }
    text += "  ]\n}\n";
    return text;
}

} // namespace coastline

// The engine reads a recording through a FILE, and the file built from the JSON goes to a
// temporary file that is removed when it is closed.
extern "C" FILE *OpenGhostRecording(const char *pszPath) {
    FILE *original = std::fopen(pszPath, "rb");
    if (original != nullptr) {
        return original;
    }
    std::ifstream text(std::filesystem::path(pszPath).replace_extension(".json"));
    if (!text) {
        return nullptr;
    }
    auto json = nlohmann::ordered_json::parse(text, nullptr, false);
    if (json.is_discarded()) {
        return nullptr;
    }
    auto bytes = coastline::GhostFromJson(json);
    if (!bytes) {
        return nullptr;
    }
    FILE *built = std::tmpfile();
    if (built == nullptr) {
        return nullptr;
    }
    if (std::fwrite(bytes->data(), 1, bytes->size(), built) != bytes->size()) {
        std::fclose(built);
        return nullptr;
    }
    std::rewind(built);
    return built;
}
