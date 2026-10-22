#include "encoder.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>

namespace Tools::EncodeText {

namespace {

/** The glyphs of the Japanese release from code 0x24, one character per code, in UTF-8. */
constexpr std::string_view kGlyphs =
    "！？。、ー☀☁☂❄☾゛゜"
    "あかさたなはまやらがざだばぱぁっいきしちにひみゆりぎじぢびぴぃゃ"
    "うくすつぬふむよるぐずづぶぷぅゅえけせてねへめわれげぜでべぺぇょ"
    "おこそとのほもをろごぞどぼぽぉん"
    "アカサタナハマヤラガザダバパァッイキシチニヒミユリギジヂビピィャ"
    "ウクスツヌフムヨルグズヅブプゥュエケセテネヘメワレゲゼデベペェョ"
    "オコソトノホモヲロゴゾドボポォン"
    "異常振動交換必要左右上下反転使用消空足読書込中選元他全前後発生";

/** The code of the first glyph of kGlyphs. */
constexpr unsigned kFirstGlyphCode = 0x24;

/** The length of the UTF-8 sequence that starts with a byte. */
std::size_t sequenceLength(unsigned char lead) {
    if (lead < 0x80) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    return 4;
}

/** The glyph code of a UTF-8 character, when the glyphs include it. */
std::optional<unsigned> glyphCode(std::string_view character) {
    unsigned code = kFirstGlyphCode;
    for (std::size_t i = 0; i < kGlyphs.size(); code++) {
        const auto length = sequenceLength(static_cast<unsigned char>(kGlyphs[i]));
        if (kGlyphs.substr(i, length) == character) {
            return code;
        }
        i += length;
    }
    return std::nullopt;
}

/** Whether a literal, from its opening quote to its closing quote, has a byte beyond ASCII. */
bool hasUtf8(std::string_view literal) {
    for (const char c : literal) {
        if (static_cast<unsigned char>(c) >= 0x80) {
            return true;
        }
    }
    return false;
}

} // namespace

std::expected<std::string, Common::Error> encode(std::string_view source) {
    std::string out;
    out.reserve(source.size());
    std::size_t line = 1;
    std::size_t i = 0;
    while (i < source.size()) {
        const char c = source[i];
        if (source.substr(i, 2) == "//") {
            const auto end = source.find('\n', i);
            const auto stop = end == std::string_view::npos ? source.size() : end;
            out.append(source.substr(i, stop - i));
            i = stop;
            continue;
        }
        if (source.substr(i, 2) == "/*") {
            const auto end = source.find("*/", i + 2);
            const auto stop = end == std::string_view::npos ? source.size() : end + 2;
            for (std::size_t j = i; j < stop; j++) {
                line += source[j] == '\n';
            }
            out.append(source.substr(i, stop - i));
            i = stop;
            continue;
        }
        if (c != '"' && c != '\'') {
            line += c == '\n';
            out.push_back(c);
            i++;
            continue;
        }
        // A literal ends at the next quote of its kind that no backslash escapes.
        std::size_t end = i + 1;
        while (end < source.size() && source[end] != c) {
            end += source[end] == '\\' ? 2 : 1;
        }
        const auto literal = source.substr(i, end + 1 - i);
        if (c == '\'' || !hasUtf8(literal)) {
            out.append(literal);
            i = end + 1;
            continue;
        }
        for (std::size_t j = 0; j < literal.size();) {
            const auto byte = static_cast<unsigned char>(literal[j]);
            if (literal[j] == '\\') {
                out.append(literal.substr(j, 2));
                j += 2;
                continue;
            }
            if (byte < 0x80) {
                out.push_back(literal[j]);
                j++;
                continue;
            }
            const auto length = sequenceLength(byte);
            const auto character = literal.substr(j, length);
            const auto code = glyphCode(character);
            if (!code) {
                return std::unexpected(Common::Error{
                    Common::ErrorCode::InvalidInput,
                    std::format(
                        "Line {} has {}, a character outside the glyph set.", line, character)});
            }
            // The glyph of ？ has the code of %. The text routines read a single % as the start of
            // a control code.
            out.append(*code == '%' ? "%%" : std::format("\\{:03o}", *code));
            j += length;
        }
        i = end + 1;
    }
    return out;
}

} // namespace Tools::EncodeText
