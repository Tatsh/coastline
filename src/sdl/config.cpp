#include "config.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <locale>
#include <ranges>
#include <sstream>

#include "savefile.h"

namespace coastline {

namespace {

bool ValidUtf8(std::string_view text) {
    for (std::size_t at = 0; at < text.size();) {
        const auto first = static_cast<unsigned char>(text[at++]);
        if (first < 0x80) {
            if (first == 0 || (first < 0x20 && first != '\t' && first != '\r' && first != '\n')) {
                return false;
            }
            continue;
        }
        const int following = first >= 0xc2 && first <= 0xdf ? 1 :
                              first >= 0xe0 && first <= 0xef ? 2 :
                              first >= 0xf0 && first <= 0xf4 ? 3 :
                                                               0;
        if (following == 0 || static_cast<std::size_t>(following) > text.size() - at) {
            return false;
        }
        std::uint32_t point = first & (0x3fu >> following);
        for (int count = 0; count < following; ++count) {
            const auto byte = static_cast<unsigned char>(text[at++]);
            if ((byte & 0xc0) != 0x80) {
                return false;
            }
            point = point << 6 | (byte & 0x3f);
        }
        if ((following == 1 && point < 0x80) || (following == 2 && point < 0x800) ||
            (following == 3 && point < 0x10000) || point > 0x10ffff ||
            (point >= 0xd800 && point <= 0xdfff)) {
            return false;
        }
    }
    return true;
}

std::string_view Trim(std::string_view text) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

// The whole text as a number, read in the classic locale. Floating-point std::from_chars needs
// macOS 26.
std::optional<float> ParseNumber(std::string_view text) {
    std::istringstream stream{std::string(text)};
    stream.imbue(std::locale::classic());
    float result = 0.0f;
    stream >> std::noskipws >> result;
    if (stream.fail() || stream.peek() != std::char_traits<char>::eof()) {
        return std::nullopt;
    }
    return result;
}

} // namespace

bool GameConfig::CaseInsensitiveLess::operator()(std::string_view left,
                                                 std::string_view right) const {
    return std::ranges::lexicographical_compare(left, right, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) <
               std::tolower(static_cast<unsigned char>(b));
    });
}

GameConfig GameConfig::Load(const std::filesystem::path &path) {
    const auto file = ReadSaveFile(path, 16 * 1024 * 1024);
    if (!file) {
        return {};
    }
    return Parse({reinterpret_cast<const char *>(file->data()), file->size()});
}

GameConfig GameConfig::Parse(std::string_view text) {
    GameConfig config;
    if (!ValidUtf8(text)) {
        return {};
    }
    if (text.starts_with("\xef\xbb\xbf")) {
        text.remove_prefix(3);
    }
    std::string section;
    for (auto range : text | std::views::split('\n')) {
        auto line = Trim(std::string_view(range.begin(), range.end()));
        if (line.empty() || line.front() == ';' || line.front() == '#') {
            continue;
        }
        if (line.front() == '[') {
            auto end = line.find(']');
            if (end == std::string_view::npos) {
                return {};
            }
            const auto tail = Trim(line.substr(end + 1));
            if (!tail.empty() && tail.front() != ';' && tail.front() != '#') {
                return {};
            }
            section = std::string(Trim(
                line.substr(1, end == std::string_view::npos ? std::string_view::npos : end - 1)));
            continue;
        }
        auto equals = line.find('=');
        if (equals == std::string_view::npos) {
            return {};
        }
        if (Trim(line.substr(0, equals)).empty()) {
            return {};
        }
        config.Set(
            section, Trim(line.substr(0, equals)), std::string(Trim(line.substr(equals + 1))));
    }
    return config;
}

bool GameConfig::Save(const std::filesystem::path &path) const {
    std::ostringstream file;
    bool first = true;
    for (const auto &[name, values] : m_sections) {
        if (!first) {
            file << "\r\n";
        }
        first = false;
        file << '[' << name << "]\r\n";
        for (const auto &[key, value] : values) {
            file << key << '=' << value << "\r\n";
        }
    }
    return WriteSaveFile(path, file.str());
}

std::optional<std::string> GameConfig::Get(std::string_view section, std::string_view key) const {
    auto found = m_sections.find(section);
    if (found == m_sections.end()) {
        return std::nullopt;
    }
    auto value = found->second.find(key);
    if (value == found->second.end()) {
        return std::nullopt;
    }
    return value->second;
}

long GameConfig::GetInteger(std::string_view section, std::string_view key, long fallback) const {
    auto value = Get(section, key);
    if (!value) {
        return fallback;
    }
    long result = 0;
    auto [end, error] = std::from_chars(value->data(), value->data() + value->size(), result);
    return error == std::errc() && end == value->data() + value->size() ? result : fallback;
}

int GameConfig::GetIndex(std::string_view section,
                         std::string_view key,
                         std::size_t count,
                         int fallback) const {
    long value = GetInteger(section, key, -1);
    return value >= 0 && static_cast<unsigned long>(value) < count ? static_cast<int>(value) :
                                                                     fallback;
}

float GameConfig::GetFloat(std::string_view section, std::string_view key, float fallback) const {
    auto value = Get(section, key);
    if (!value) {
        return fallback;
    }
    return ParseNumber(*value).value_or(fallback);
}

bool GameConfig::GetBoolean(std::string_view section, std::string_view key, bool fallback) const {
    auto value = Get(section, key);
    if (!value) {
        return fallback;
    }
    std::string lower(*value);
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower == "true" || lower == "yes" || lower == "on") {
        return true;
    }
    if (lower == "false" || lower == "no" || lower == "off") {
        return false;
    }
    return GetInteger(section, key, fallback ? 1 : 0) != 0;
}

void GameConfig::Set(std::string_view section, std::string_view key, std::string value) {
    auto found = m_sections.find(section);
    if (found == m_sections.end()) {
        found = m_sections.emplace(std::string(section), Section{}).first;
    }
    found->second.insert_or_assign(std::string(key), std::move(value));
}

std::array<int, 2> ExpandInternalResolution(int width, int height, float aspect) {
    constexpr int LARGEST = 16384;
    width = std::clamp(width, 320, LARGEST);
    height = std::clamp(height, 240, LARGEST);
    if (std::isfinite(aspect) && aspect > 0) {
        // Float ratios can exceed an exact pixel boundary by a rounding error.
        auto pixels = [](double value) {
            return static_cast<int>(std::min(std::ceil(value * (1.0 - 1e-6)), 16384.0));
        };
        if (static_cast<double>(height) * aspect >= width) {
            width = std::max(width, pixels(static_cast<double>(height) * aspect));
        } else {
            height = std::max(height, pixels(static_cast<double>(width) / aspect));
        }
    }
    return {width, height};
}

std::optional<float> ParseAspectRatio(std::string_view text) {
    text = Trim(text);
    std::string lower(text);
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return std::tolower(c); });
    if (lower.empty() || lower == "auto") {
        return 0.0f;
    }
    std::optional<float> ratio;
    auto colon = text.find(':');
    if (colon == std::string_view::npos) {
        ratio = ParseNumber(text);
    } else {
        auto width = ParseNumber(Trim(text.substr(0, colon)));
        auto height = ParseNumber(Trim(text.substr(colon + 1)));
        if (width && height && *height > 0.0f) {
            ratio = *width / *height;
        }
    }
    if (!ratio || !std::isfinite(*ratio) || !(*ratio > 0.0f)) {
        return std::nullopt;
    }
    return ratio;
}

} // namespace coastline
