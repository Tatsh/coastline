#include "decalcar.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <memory>

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
    const auto name = directory.filename();
    if (name == "cupra" || name == "helmet" || name == "beach-ball") {
        if (!car.LoadImagePanels()) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Cannot prepare editable images in %s.",
                         path.string().c_str());
            return std::nullopt;
        }
        return car;
    }
    // Every texture and mask the paint shop reads lies inside the model.
    if (car.m_bytes.size() < DECAL_MASKS_AT + 4) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION, "Truncated car data in %s.", path.string().c_str());
        return std::nullopt;
    }
    for (int material = 0; material < CAR_MATERIALS; ++material) {
        DecalTexture texture = car.Texture(material);
        if (texture.palette == 0 && car.m_bytes[MATERIAL_TEXTURES_AT + material] == 0) {
            continue;
        }
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
    if (!car.HasMaterial(CAR_MATERIAL_PAINT)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Missing editable paint palette in %s.",
                     path.string().c_str());
        return std::nullopt;
    }
    for (const auto &image : car.m_images) {
        const auto extras = image.value("extras", nlohmann::ordered_json::object());
        const int material =
            extras.value("coastlinePaint", nlohmann::ordered_json::object()).value("material", -1);
        DecalTexture texture;
        bool decal = false;
        if (material >= 0 && material < CAR_MATERIALS) {
            texture = car.Texture(material);
            if (extras.contains("coastlineImage")) {
                const auto &layout = extras.at("coastlineImage");
                texture.texels = layout.at("texels").get<std::size_t>();
                texture.width = layout.at("width").get<int>();
                texture.height = layout.at("height").get<int>();
                if (texture.width <= 0 || texture.height <= 0 || texture.width > 16384 ||
                    texture.height > 16384 || texture.texels > car.m_bytes.size() ||
                    static_cast<std::size_t>((texture.width + 1) / 2) * texture.height >
                        car.m_bytes.size() - texture.texels) {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "Image exceeds car texture data in %s.",
                                 path.string().c_str());
                    return std::nullopt;
                }
                for (int slot = 0; slot < DECAL_SLOTS; ++slot) {
                    if (texture.texels == car.Texture(slot).texels) {
                        decal = true;
                    }
                }
            } else {
                decal = material < DECAL_SLOTS;
            }
        }
        car.m_imageTextures.push_back(texture);
        car.m_imageDecals.push_back(decal);
    }
    return car;
}

bool DecalCar::LoadImagePanels() {
    if (m_images.size() < 3 || m_images.size() > 20) {
        return false;
    }
    m_imagePanels = true;
    m_bytes.assign(1, 0);
    // Entry zero tints the source image; the other entries apply ink colours.
    constexpr std::array<u16, 16> colors{0xfffe,
                                         0x4211,
                                         0xffff,
                                         0xf801,
                                         0x7801,
                                         0x07c1,
                                         0x03c1,
                                         0xffc1,
                                         0x7bc1,
                                         0x003f,
                                         0x001f,
                                         0xf83f,
                                         0x781f,
                                         0xfc01,
                                         0x8201,
                                         0x0001};
    for (u16 color : colors) {
        m_bytes.push_back(static_cast<u8>(color >> 8));
        m_bytes.push_back(static_cast<u8>(color));
    }
    for (const auto &encoded : m_imageBytes) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> image(
            SDL_LoadPNG_IO(SDL_IOFromConstMem(encoded.data(), encoded.size()), true),
            SDL_DestroySurface);
        if (!image || image->w <= 0 || image->h <= 0 || image->w > 80 || image->h > 125 ||
            (image->w + 1) / 2 * image->h > 2048) {
            return false;
        }
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> rgba(
            SDL_ConvertSurface(image.get(), SDL_PIXELFORMAT_RGBA32), SDL_DestroySurface);
        if (!rgba) {
            return false;
        }
        const auto width = static_cast<std::size_t>(image->w);
        const auto height = static_cast<std::size_t>(image->h);
        std::vector<u8> pixels(width * height * 4);
        for (std::size_t row = 0; row < height; ++row) {
            const auto *source = static_cast<const u8 *>(rgba->pixels) + row * rgba->pitch;
            std::copy_n(source, width * 4, pixels.begin() + row * width * 4);
        }
        auto hash = [&](u8 value) { m_imageKey = (m_imageKey ^ value) * 16777619u; };
        hash(static_cast<u8>(width));
        hash(static_cast<u8>(height));
        for (u8 value : pixels) {
            hash(value);
        }
        m_basePixels.push_back(std::move(pixels));
        m_imageTextures.push_back({m_bytes.size(), 1, image->w, image->h});
        m_bytes.resize(m_bytes.size() + (width + 1) / 2 * height);
    }
    return m_bytes.size() + 7 <= 0x3e00;
}

bool DecalCar::ImagePanels() const {
    return m_imagePanels;
}

int DecalCar::PanelCount() const {
    return m_imagePanels ? static_cast<int>(m_imageTextures.size()) : DECAL_SLOTS;
}

std::vector<u8> DecalCar::PanelPixels(int panel) const {
    if (!m_imagePanels || panel < 0 || panel >= PanelCount()) {
        return {};
    }
    auto pixels = m_basePixels[static_cast<std::size_t>(panel)];
    const auto texture = Texture(panel);
    const auto tint = PaletteColor(panel, 0);
    auto channel = [](u16 color, int shift) {
        const auto value = static_cast<unsigned>(color >> shift & 31);
        return static_cast<u8>(value << 3 | value >> 2);
    };
    for (int y = 0; y < texture.height; ++y) {
        for (int x = 0; x < texture.width; ++x) {
            const auto pair = m_bytes[texture.texels + PixelOffset(x, y, texture.width)];
            const auto ink = static_cast<u8>((x & 1) ? pair & 15 : pair >> 4);
            const auto color = PaletteColor(panel, ink);
            const auto pixel = static_cast<std::size_t>(y * texture.width + x) * 4;
            for (int component = 0; component < 3; ++component) {
                const int shift = 11 - component * 5;
                pixels[pixel + component] =
                    ink == 0 ? static_cast<u8>(
                                   (pixels[pixel + component] * channel(tint, shift) + 127) / 255) :
                               channel(color, shift);
            }
            if (ink != 0) {
                pixels[pixel + 3] = (color & 1) ? 255 : 0;
            }
        }
    }
    return pixels;
}

std::vector<u8> DecalCar::NoteHeader() const {
    if (!m_imagePanels) {
        return {};
    }
    return {'C',
            'P',
            'N',
            1,
            static_cast<u8>(m_imageKey >> 24),
            static_cast<u8>(m_imageKey >> 16),
            static_cast<u8>(m_imageKey >> 8),
            static_cast<u8>(m_imageKey)};
}

bool DecalCar::ValidNote(std::span<const u8> note) const {
    const auto header = NoteHeader();
    std::size_t size = header.size() + PALETTE_BYTES;
    const bool console = DecalNote() == "TOP GEAR DECALS";
    for (int panel = 0; panel < PanelCount(); ++panel) {
        if (!HasMaterial(panel)) {
            continue;
        }
        const auto texture = Texture(panel);
        const auto width = static_cast<std::size_t>(texture.width);
        auto height = static_cast<std::size_t>(texture.height);
        if (console && panel < 2) {
            ++height;
        }
        size += ((width + 1) / 2) * height;
        if (size > note.size()) {
            return false;
        }
    }
    return note.size() >= size && std::equal(header.begin(), header.end(), note.begin());
}

std::size_t DecalCar::PixelOffset(int x, int y, int width) const {
    return static_cast<std::size_t>(m_imagePanels ? y * ((width + 1) / 2) + x / 2 :
                                                    TexelOffset(x, y, width));
}

DecalTexture DecalCar::Texture(int material) const {
    if (m_imagePanels) {
        return material >= 0 && material < PanelCount() ?
                   m_imageTextures[static_cast<std::size_t>(material)] :
                   DecalTexture{};
    }
    if (material < 0 || material >= CAR_MATERIALS) {
        return {};
    }
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
    return HasMaterial(material) ? &m_bytes[Texture(material).texels] : nullptr;
}

bool DecalCar::HasMaterial(int material) const {
    const auto texture = Texture(material);
    return texture.width > 0 && texture.height > 0 && texture.palette != 0;
}

std::string DecalCar::DecalNote() const {
    if (m_imagePanels) {
        return m_directory.filename() == "cupra"  ? "COASTLINE CP" :
               m_directory.filename() == "helmet" ? "COASTLINE HE" :
                                                    "COASTLINE BB";
    }
    return m_directory.filename() == "milk-truck" ? "COASTLINE MT" : "TOP GEAR DECALS";
}

const u8 *DecalCar::End() const {
    return m_bytes.data() + m_bytes.size();
}

std::vector<u8> DecalCar::CopyEditableData() const {
    return m_bytes;
}

bool DecalCar::RestoreEditableData(std::span<const u8> data) {
    if (data.size() != m_bytes.size()) {
        return false;
    }
    std::copy(data.begin(), data.end(), m_bytes.begin());
    return true;
}

const u8 *DecalCar::Mask(int slot) const {
    if (m_imagePanels) {
        return nullptr;
    }
    std::size_t masks = Offset(m_bytes, DECAL_MASKS_AT);
    if (masks == 0 || masks + DECAL_SLOTS * 4 > m_bytes.size()) {
        return nullptr;
    }
    std::size_t mask = Offset(m_bytes, masks + static_cast<std::size_t>(slot) * 4);
    return mask == 0 ? nullptr : &m_bytes[mask];
}

u16 DecalCar::PaletteColor(int material, int entry) const {
    if (!HasMaterial(material) || entry < 0 || entry >= 16) {
        return 0;
    }
    std::size_t at = Texture(material).palette + static_cast<std::size_t>(entry) * 2;
    return static_cast<u16>(m_bytes[at] << 8 | m_bytes[at + 1]);
}

void DecalCar::SetPaletteColor(int material, int entry, u16 color) {
    if (!HasMaterial(material) || entry < 0 || entry >= 16) {
        return;
    }
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
        if (m_imagePanels) {
            const auto texture = Texture(static_cast<int>(index));
            if (!SaveRgba8Png(
                    to, PanelPixels(static_cast<int>(index)), texture.width, texture.height)) {
                return std::nullopt;
            }
            continue;
        }
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
        const DecalTexture &texture = m_imageTextures[index];
        std::vector<std::uint8_t> texels(
            static_cast<std::size_t>((texture.width + 1) / 2 * texture.height));
        std::array<std::uint8_t, PALETTE_BYTES> palette{};
        if (m_imageDecals[index]) {
            int rowWidth = texture.width;
            for (int slot = 0; slot < DECAL_SLOTS; ++slot) {
                const auto panel = Texture(slot);
                if (panel.width > 0 && panel.texels == texture.texels) {
                    rowWidth = panel.width;
                    break;
                }
            }
            // The image of a decal is its texels in rows, without the swap of the odd rows.
            for (int y = 0; y < texture.height; ++y) {
                for (int x = 0; x < texture.width; ++x) {
                    u8 pair = m_bytes[texture.texels +
                                      static_cast<std::size_t>(TexelOffset(x, y, rowWidth))];
                    u8 texel = (x & 1) != 0 ? pair & 0xF : pair >> 4;
                    auto at = static_cast<std::size_t>(y * ((texture.width + 1) / 2) + x / 2);
                    texels[at] |= static_cast<std::uint8_t>((x & 1) != 0 ? texel : texel << 4);
                }
            }
        } else if (!LoadIndexed4Png(m_imageBytes[index], texels, palette)) {
            // The lights retain the texels of the import and take the colours of the model.
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Cannot read image %llu in %s.",
                         static_cast<unsigned long long>(index),
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
