#include "gltfdocument.h"

#include <fstream>
#include <iterator>
#include <span>
#include <string_view>

namespace coastline {

namespace {

using Bytes = std::vector<std::uint8_t>;
using Result = std::expected<Bytes, std::string>;

Result ReadFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected(path.string() + " cannot be read.");
    }
    Bytes bytes(std::istreambuf_iterator<char>(file), {});
    if (file.bad()) {
        return std::unexpected("Cannot finish reading " + path.string() + ".");
    }
    return bytes;
}

Result ReadUri(const std::filesystem::path &directory, std::string_view uri) {
    if (!uri.starts_with("data:")) {
        return ReadFile(directory / std::string(uri));
    }
    auto comma = uri.find(',');
    if (comma == std::string_view::npos || !uri.substr(0, comma).ends_with(";base64")) {
        return std::unexpected("The data URI must use base64 encoding.");
    }
    auto encoded = uri.substr(comma + 1);
    constexpr std::string_view ALPHABET =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (encoded.size() % 4 != 0) {
        return std::unexpected("The base64 data is truncated.");
    }
    Bytes bytes;
    for (std::size_t at = 0; at < encoded.size(); at += 4) {
        unsigned value = 0;
        unsigned padding = 0;
        for (std::size_t digit = 0; digit < 4; ++digit) {
            char character = encoded[at + digit];
            auto index = ALPHABET.find(character);
            if (character == '=' && digit >= 2 && at + 4 == encoded.size()) {
                ++padding;
                index = 0;
            } else if (index == std::string_view::npos || padding > 0) {
                return std::unexpected("The base64 data is invalid.");
            }
            value = value << 6 | static_cast<unsigned>(index);
        }
        bytes.push_back(static_cast<std::uint8_t>(value >> 16));
        if (padding < 2) {
            bytes.push_back(static_cast<std::uint8_t>(value >> 8));
        }
        if (padding < 1) {
            bytes.push_back(static_cast<std::uint8_t>(value));
        }
    }
    return bytes;
}

std::uint32_t Little32(std::span<const std::uint8_t> bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) | static_cast<std::uint32_t>(bytes[at + 1]) << 8 |
           static_cast<std::uint32_t>(bytes[at + 2]) << 16 |
           static_cast<std::uint32_t>(bytes[at + 3]) << 24;
}

} // namespace

std::expected<GltfDocument, std::string> ReadGltfDocument(const std::filesystem::path &requested) {
    GltfDocument result;
    result.path = requested;
    if (requested.extension() == ".gltf" || requested.extension() == ".glb") {
        auto text = requested;
        text.replace_extension(".gltf");
        std::error_code error;
        result.path = std::filesystem::exists(text, error) ?
                          text :
                          std::filesystem::path(text).replace_extension(".glb");
    }
    auto file = ReadFile(result.path);
    if (!file) {
        return std::unexpected(file.error());
    }
    std::span<const std::uint8_t> jsonBytes = *file;
    std::span<const std::uint8_t> binary;
    bool glb = result.path.extension() == ".glb";
    if (glb) {
        if (file->size() < 20 || Little32(*file, 0) != 0x46546C67 || Little32(*file, 4) != 2 ||
            Little32(*file, 8) != file->size()) {
            return std::unexpected(result.path.string() + " has an invalid GLB header.");
        }
        bool foundJson = false;
        bool foundBinary = false;
        for (std::size_t at = 12; at < file->size();) {
            if (file->size() - at < 8) {
                return std::unexpected("The GLB chunk header is truncated.");
            }
            std::size_t length = Little32(*file, at);
            auto type = Little32(*file, at + 4);
            if (length % 4 != 0 || length > file->size() - at - 8) {
                return std::unexpected("The GLB chunk length is invalid.");
            }
            auto chunk = std::span(*file).subspan(at + 8, length);
            if (at == 12 && type != 0x4E4F534A) {
                return std::unexpected("The first GLB chunk must be JSON.");
            }
            if (type == 0x4E4F534A) {
                if (foundJson) {
                    return std::unexpected("The GLB has duplicate JSON chunks.");
                }
                jsonBytes = chunk;
                foundJson = true;
            } else if (type == 0x004E4942) {
                if (foundBinary) {
                    return std::unexpected("The GLB has duplicate BIN chunks.");
                }
                binary = chunk;
                foundBinary = true;
            }
            at += 8 + length;
        }
    }
    try {
        result.json = nlohmann::ordered_json::parse(jsonBytes.begin(), jsonBytes.end());
        if (!result.json.is_object() || result.json.at("asset").at("version") != "2.0") {
            return std::unexpected("The document must use glTF 2.0.");
        }
        std::vector<std::size_t> offsets;
        std::vector<std::size_t> lengths;
        for (const auto &entry : result.json.value("buffers", nlohmann::ordered_json::array())) {
            Result bytes =
                entry.contains("uri") ?
                    ReadUri(result.path.parent_path(), entry.at("uri").get<std::string>()) :
                    (glb && offsets.empty() ?
                         Result(Bytes(binary.begin(), binary.end())) :
                         Result(std::unexpected("A buffer requires a URI or GLB BIN chunk.")));
            if (!bytes) {
                return std::unexpected(bytes.error());
            }
            auto length = entry.at("byteLength").get<std::size_t>();
            if (length > bytes->size()) {
                return std::unexpected("A glTF buffer is truncated.");
            }
            while (result.buffer.size() % 4 != 0) {
                result.buffer.push_back(0);
            }
            offsets.push_back(result.buffer.size());
            lengths.push_back(length);
            result.buffer.insert(result.buffer.end(), bytes->begin(), bytes->begin() + length);
        }
        for (auto &view : result.json["bufferViews"]) {
            auto buffer = view.at("buffer").get<std::size_t>();
            auto offset = view.value("byteOffset", std::size_t{0});
            auto length = view.at("byteLength").get<std::size_t>();
            if (buffer >= lengths.size() || offset > lengths[buffer] ||
                length > lengths[buffer] - offset) {
                return std::unexpected("A glTF buffer view is outside its buffer.");
            }
            view["buffer"] = 0;
            view["byteOffset"] = offsets[buffer] + offset;
        }
        if (!offsets.empty()) {
            result.json["buffers"] =
                nlohmann::ordered_json::array({{{"byteLength", result.buffer.size()}}});
        }
    } catch (const nlohmann::json::exception &error) {
        return std::unexpected(result.path.string() + " is not a glTF document. " + error.what());
    }
    return result;
}

std::expected<std::vector<std::uint8_t>, std::string> GltfDocument::Image(std::size_t index) const {
    try {
        const auto &image = json.at("images").at(index);
        if (image.contains("uri")) {
            return ReadUri(path.parent_path(), image.at("uri").get<std::string>());
        }
        const auto &view = json.at("bufferViews").at(image.at("bufferView").get<std::size_t>());
        auto offset = view.at("byteOffset").get<std::size_t>();
        auto length = view.at("byteLength").get<std::size_t>();
        if (offset > buffer.size() || length > buffer.size() - offset) {
            return std::unexpected("The image is outside its buffer.");
        }
        return Bytes(buffer.begin() + offset, buffer.begin() + offset + length);
    } catch (const nlohmann::json::exception &error) {
        return std::unexpected(std::string("The glTF image is invalid. ") + error.what());
    }
}

} // namespace coastline
