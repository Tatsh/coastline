#include "decalcar.h"

#include <array>
#include <fstream>

#include <SDL3/SDL.h>

#include "gltfdocument.h"
#include "indexedimage.h"
#include "n64model.h"

namespace coastline {

namespace {

// Where the console loads a car, and where the header of the model lists its textures, the
// texture of each material, and the masks of the decals.
constexpr std::uint32_t CAR_BASE = 0x803c8000;
constexpr std::size_t TEXTURES_AT = 0x14;
constexpr std::size_t MATERIAL_TEXTURES_AT = 0x110;
constexpr std::size_t DECAL_MASKS_AT = 0x11c;
constexpr std::size_t TEXTURE_SIZE = 0x24;
constexpr std::size_t PALETTE_BYTES = 0x20;

std::uint32_t Big32(const std::vector<u8> &bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) << 24 | bytes[at + 1] << 16 | bytes[at + 2] << 8 |
           bytes[at + 3];
}

// The offset in the bytes of a pointer of the loaded model, or 0 for none.
std::size_t Offset(const std::vector<u8> &bytes, std::size_t at) {
    std::uint32_t address = Big32(bytes, at);
    return address < CAR_BASE ? 0 : address - CAR_BASE;
}

} // namespace

std::optional<DecalCar> DecalCar::Load(const std::filesystem::path &directory) {
    DecalCar car;
    car.m_directory = directory;
    auto source = ReadGltfDocument(directory / "model.gltf");
    if (!source) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", source.error().c_str());
        return std::nullopt;
    }
    const auto &path = source->path;
    auto bytes = ModelFromGltf(source->json, source->buffer);
    if (!bytes) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Invalid cartridge car %s (%s).",
                     path.string().c_str(),
                     bytes.error().c_str());
        return std::nullopt;
    }
    car.m_bytes = std::move(*bytes);
    car.m_images = source->json.value("images", nlohmann::ordered_json::array());
    for (std::size_t index = 0; index < car.m_images.size(); ++index) {
        auto image = source->Image(index);
        if (!image) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", image.error().c_str());
            return std::nullopt;
        }
        car.m_imageBytes.push_back(std::move(*image));
        car.m_images[index].erase("bufferView");
        car.m_images[index].erase("mimeType");
        car.m_images[index]["uri"] = "image-" + std::to_string(index) + ".png";
    }
    // Every texture and mask the paint shop reads lies inside the model.
    if (car.m_bytes.size() < DECAL_MASKS_AT + 4) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION, "Truncated car data in %s.", path.string().c_str());
        return std::nullopt;
    }
    for (int material = 0; material < CAR_MATERIALS; ++material) {
        DecalTexture texture = car.Texture(material);
        auto texels = static_cast<std::size_t>(texture.width * texture.height / 2);
        if (texture.width == 0 || texture.texels + texels > car.m_bytes.size() ||
            texture.palette == 0 || texture.palette + PALETTE_BYTES > car.m_bytes.size()) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Material %d in %s requires a 16-color texture.",
                         material,
                         path.string().c_str());
            return std::nullopt;
        }
    }
    return car;
}

DecalTexture DecalCar::Texture(int material) const {
    std::size_t table = Offset(m_bytes, TEXTURES_AT);
    std::size_t at =
        table + m_bytes[MATERIAL_TEXTURES_AT + static_cast<std::size_t>(material)] * TEXTURE_SIZE;
    if (table == 0 || at + TEXTURE_SIZE > m_bytes.size()) {
        return {};
    }
    return {Offset(m_bytes, at),
            Offset(m_bytes, at + 4),
            m_bytes[at + 12] << 8 | m_bytes[at + 13],
            m_bytes[at + 14] << 8 | m_bytes[at + 15]};
}

u8 *DecalCar::Texels(int material) {
    return &m_bytes[Texture(material).texels];
}

const u8 *DecalCar::End() const {
    return m_bytes.data() + m_bytes.size();
}

const u8 *DecalCar::Mask(int slot) const {
    std::size_t masks = Offset(m_bytes, DECAL_MASKS_AT);
    if (masks == 0 || masks + DECAL_SLOTS * 4 > m_bytes.size()) {
        return nullptr;
    }
    std::size_t mask = Offset(m_bytes, masks + static_cast<std::size_t>(slot) * 4);
    return mask == 0 ? nullptr : &m_bytes[mask];
}

u16 DecalCar::PaletteColor(int material, int entry) const {
    std::size_t at = Texture(material).palette + static_cast<std::size_t>(entry) * 2;
    return static_cast<u16>(m_bytes[at] << 8 | m_bytes[at + 1]);
}

void DecalCar::SetPaletteColor(int material, int entry, u16 color) {
    std::size_t at = Texture(material).palette + static_cast<std::size_t>(entry) * 2;
    m_bytes[at] = static_cast<u8>(color >> 8);
    m_bytes[at + 1] = static_cast<u8>(color);
}

std::optional<std::filesystem::path>
DecalCar::WriteImages(const std::filesystem::path &directory) const {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    for (std::size_t index = 0; index < m_images.size(); ++index) {
        const auto &image = m_images[index];
        auto uri = image.value("uri", std::string());
        auto to = directory / uri;
        int material = image.value("extras", nlohmann::ordered_json::object())
                           .value("coastlinePaint", nlohmann::ordered_json::object())
                           .value("material", -1);
        if (material < 0 || material >= CAR_MATERIALS) {
            std::ofstream output(to, std::ios::binary | std::ios::trunc);
            const auto &bytes = m_imageBytes[index];
            output.write(reinterpret_cast<const char *>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
            if (!output) {
                return std::nullopt;
            }
            continue;
        }
        DecalTexture texture = Texture(material);
        std::vector<std::uint8_t> texels(
            static_cast<std::size_t>((texture.width + 1) / 2 * texture.height));
        std::array<std::uint8_t, PALETTE_BYTES> palette{};
        if (material < DECAL_SLOTS) {
            // The image of a decal is its texels in rows, without the swap of the odd rows.
            for (int y = 0; y < texture.height; ++y) {
                for (int x = 0; x < texture.width; ++x) {
                    u8 pair = m_bytes[texture.texels +
                                      static_cast<std::size_t>(TexelOffset(x, y, texture.width))];
                    u8 texel = (x & 1) != 0 ? pair & 0xF : pair >> 4;
                    auto at = static_cast<std::size_t>(y * ((texture.width + 1) / 2) + x / 2);
                    texels[at] |= static_cast<std::uint8_t>((x & 1) != 0 ? texel : texel << 4);
                }
            }
        } else if (!LoadIndexed4Png(m_imageBytes[index], texels, palette)) {
            // The lights retain the texels of the import and take the colours of the model.
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Cannot read image %zu in %s.",
                         index,
                         m_directory.string().c_str());
            return std::nullopt;
        }
        std::copy_n(m_bytes.begin() + static_cast<std::ptrdiff_t>(texture.palette),
                    PALETTE_BYTES,
                    palette.begin());
        if (!SaveIndexed4Png(to, texels, palette, texture.width, texture.height)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot write %s.", to.string().c_str());
            return std::nullopt;
        }
    }
    nlohmann::ordered_json document{{"asset", {{"version", "2.0"}}}, {"images", m_images}};
    auto path = directory / "model.gltf";
    std::ofstream(path, std::ios::trunc) << document.dump(1) << '\n';
    return path;
}

const std::filesystem::path &DecalCar::Directory() const {
    return m_directory;
}

} // namespace coastline
