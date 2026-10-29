// The models of the console in the SDL game. A model inflates from the cartridge with big-endian
// fields and offsets from its start, which the console turns into pointers in place. Here the
// fields take the order of the host, and the offsets become the 32-bit pointers of the host.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_set>

#include "common/model.h"
#include "sdl/cartridgedata.h"

namespace {

static_assert(sizeof(ModelPart) == 20, "A part of a model is 20 bytes on the console.");
static_assert(offsetof(Model, parts) == 8, "The parts of a model start at its 9th byte.");

class ModelReader {
public:
    ModelReader(std::uint8_t *base, std::size_t size) : m_base(base), m_size(size) {
    }

    void Read() {
        if (m_size < offsetof(Model, parts)) {
            return;
        }
        u32 partCount = Swap32At(offsetof(Model, partCount));
        std::uint32_t animations = Swap32At(offsetof(Model, animations));
        for (u32 i = 0; i < partCount; ++i) {
            std::size_t part = offsetof(Model, parts) + i * sizeof(ModelPart);
            if (part + sizeof(ModelPart) > m_size) {
                break;
            }
            std::uint32_t displayList = Swap32At(part + offsetof(ModelPart, displayList));
            Swap16At(part + offsetof(ModelPart, flags));
            for (std::size_t axis = 0; axis < 3; ++axis) {
                Swap32At(part + offsetof(ModelPart, position) + axis * 4);
            }
            // A part with no display list has an offset of 0.
            WriteWord(part + offsetof(ModelPart, displayList),
                      displayList == 0 ? 0 : ReadDisplayList(displayList));
        }
        WriteWord(offsetof(Model, animations), animations == 0 ? 0 : ReadAnimations(animations));
    }

private:
    std::uint32_t Swap32At(std::size_t offset) {
        if (offset + 4 > m_size) {
            return 0;
        }
        std::uint32_t value;
        std::memcpy(&value, m_base + offset, 4);
        value = __builtin_bswap32(value);
        std::memcpy(m_base + offset, &value, 4);
        return value;
    }

    void Swap16At(std::size_t offset) {
        if (offset + 2 > m_size) {
            return;
        }
        std::uint16_t value;
        std::memcpy(&value, m_base + offset, 2);
        value = __builtin_bswap16(value);
        std::memcpy(m_base + offset, &value, 2);
    }

    void WriteWord(std::size_t offset, std::uint32_t value) {
        if (offset + 4 <= m_size) {
            std::memcpy(m_base + offset, &value, 4);
        }
    }

    // The 32-bit pointer of the host for an offset in the model or an address of the console.
    std::uint32_t Pointer(std::uint32_t value, bool bytes) {
        if (value < m_size) {
            return coastline::EncodePtr32(m_base + value);
        }
        return coastline::EncodePtr32(coastline::CartridgeData::Relocate(value, bytes));
    }

    void ReadVertices(std::uint32_t offset, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t vertex = offset + i * sizeof(Vtx);
            if (vertex + sizeof(Vtx) > m_size || !m_vertices.insert(vertex).second) {
                continue;
            }
            // The coordinates, the flag, and the texture coordinates are halfwords, and the colour
            // or the normal is bytes.
            for (std::size_t half = 0; half < 6; ++half) {
                Swap16At(vertex + half * 2);
            }
        }
    }

    std::uint32_t ReadDisplayList(std::uint32_t offset) {
        std::uint32_t pointer = Pointer(offset, false);
        if (offset >= m_size || !m_lists.insert(offset).second) {
            return pointer;
        }
        for (std::size_t command = offset; command + 8 <= m_size; command += 8) {
            std::uint32_t w0 = Swap32At(command);
            std::uint32_t w1 = Swap32At(command + 4);
            switch (w0 >> 24) {
            case G_VTX:
                if (w1 < m_size) {
                    ReadVertices(w1, w0 >> 10 & 0x3f);
                }
                WriteWord(command + 4, Pointer(w1, false));
                break;
            case G_SETTIMG:
                WriteWord(command + 4, Pointer(w1, true));
                break;
            case G_DL:
                WriteWord(command + 4, ReadDisplayList(w1));
                if (w0 >> 16 & 0xff) {
                    return pointer;
                }
                break;
            case G_ENDDL:
                return pointer;
            default:
                break;
            }
        }
        return pointer;
    }

    std::uint32_t ReadAnimations(std::uint32_t offset) {
        std::size_t count = Swap32At(offset);
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t entry = offset + offsetof(ModelAnimationList, entries) + i * 4;
            std::uint32_t animation = Swap32At(entry);
            WriteWord(entry, Pointer(animation, false));
            if (animation < m_size) {
                ReadAnimation(animation);
            }
        }
        return Pointer(offset, false);
    }

    void ReadAnimation(std::uint32_t offset) {
        std::size_t vertexCount = Swap32At(offset + offsetof(ModelAnimation, vertexCount));
        std::uint32_t vertices = Swap32At(offset + offsetof(ModelAnimation, vertices));
        if (vertices < m_size) {
            ReadVertices(vertices, vertexCount);
        }
        WriteWord(offset + offsetof(ModelAnimation, vertices), Pointer(vertices, false));
        std::uint32_t unused = Swap32At(offset + offsetof(ModelAnimation, unreconstructed8));
        WriteWord(offset + offsetof(ModelAnimation, unreconstructed8), Pointer(unused, false));
        std::size_t keyCount = Swap32At(offset + offsetof(ModelAnimation, keyCount));
        Swap16At(offset + offsetof(ModelAnimation, flags));
        Swap16At(offset + offsetof(ModelAnimation, startKey));
        Swap32At(offset + offsetof(ModelAnimation, startTime));
        Swap32At(offset + offsetof(ModelAnimation, endTime));
        Swap32At(offset + offsetof(ModelAnimation, time));
        for (std::size_t k = 0; k < keyCount; ++k) {
            std::size_t slot = offset + offsetof(ModelAnimation, keys) + k * 4;
            std::uint32_t key = Swap32At(slot);
            WriteWord(slot, Pointer(key, false));
            if (key >= m_size || !m_keys.insert(key).second) {
                continue;
            }
            Swap32At(key + offsetof(ModelKeyframe, time));
            // The halfwords of the positions of the vertices. Their normals after them are bytes.
            for (std::size_t half = 0; half < vertexCount * 3; ++half) {
                Swap16At(key + offsetof(ModelKeyframe, positions) + half * 2);
            }
        }
    }

    std::uint8_t *m_base;
    std::size_t m_size;
    std::unordered_set<std::size_t> m_vertices;
    std::unordered_set<std::uint32_t> m_lists;
    std::unordered_set<std::uint32_t> m_keys;
};

} // namespace

void ReadConsoleModel(Model *model, u32 size) {
    ModelReader(reinterpret_cast<std::uint8_t *>(model), size).Read();
}
