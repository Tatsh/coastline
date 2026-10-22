#pragma once

#include <expected>
#include <string>
#include <string_view>

#include "error.h"

/** Conversion of the Japanese text of a source to the glyph codes of the Japanese release. */
namespace Tools::EncodeText {

/**
 * Rewrite every string literal of a C source that has UTF-8 characters beyond ASCII, writing each
 * of those characters as the octal escape of its glyph code. ASCII and escape sequences are copied
 * as written, and comments and character literals are copied unchanged.
 *
 * @param source The text of the source.
 * @return The rewritten source, or the line and the character the glyphs do not include.
 */
std::expected<std::string, Common::Error> encode(std::string_view source);

} // namespace Tools::EncodeText
