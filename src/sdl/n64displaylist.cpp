// The display lists the front end of the Nintendo 64 release builds, made ready for the renderer of
// the PC release before each frame is drawn. The renderer of the PC release reads the matrices of
// its display lists as floats, its vertices as ConvertedModelVertex, and the indices of its
// triangles halved, as the PC release prepares the display lists of its models when it loads them.

#include "n64displaylist.h"

#include <array>
#include <cstring>
#include <unordered_set>

#include <PR/gbi.h>
#include <PR/gu.h>

#include "sdl/cartridgedata.h"
#include "win32/displaylist.h"
#include "win32/render.h"
#include "win32/texturescan.h"

namespace {

// The matrices and vertices of one frame, and those of the display lists of the cartridge, which
// are made ready once.
constexpr std::size_t kFrameMatrices = 2048;
constexpr std::size_t kFrameVertices = 32768;
std::array<float[4][4], kFrameMatrices> g_frameMatrices;
std::array<ConvertedModelVertex, kFrameVertices> g_frameVertices;
std::size_t g_frameMatrixCount;
std::size_t g_frameVertexCount;
std::unordered_set<const GbiCommand *> g_readyLists;

void (*g_pfnSubmitDisplayList)(GbiCommand *pDisplayList);

u32 SwapWord(u32 value) {
    return __builtin_bswap32(value);
}

u16 SwapHalf(u16 value) {
    return __builtin_bswap16(value);
}

// A matrix of the console, in s15.16 fixed point, as floats. A matrix of the cartridge has
// big-endian words.
float (*ConvertMatrix(const Mtx *source, bool cartridge))[4] {
    float (*target)[4];
    if (cartridge) {
        static std::array<float[4][4], kFrameMatrices> s_cartridgeMatrices;
        static std::size_t s_cartridgeMatrixCount;
        target = s_cartridgeMatrices[s_cartridgeMatrixCount++ % kFrameMatrices];
    } else {
        target = g_frameMatrices[g_frameMatrixCount++ % kFrameMatrices];
    }
    Mtx native = *source;
    if (CartridgeDataIsBigEndian(source)) {
        for (auto &row : native.m) {
            for (auto &word : row) {
                word = static_cast<s32>(SwapWord(static_cast<u32>(word)));
            }
        }
    }
    guMtxL2F(target, &native);
    return target;
}

// Vertices of the console as the converted vertices of the PC release. Vertices of the cartridge
// have big-endian halfwords.
ConvertedModelVertex *ConvertVertices(const Vtx *source, int count, bool cartridge) {
    ConvertedModelVertex *target;
    if (cartridge) {
        static std::array<ConvertedModelVertex, kFrameVertices> s_cartridgeVertices;
        static std::size_t s_cartridgeVertexCount;
        if (s_cartridgeVertexCount + count > kFrameVertices) {
            s_cartridgeVertexCount = 0;
        }
        target = &s_cartridgeVertices[s_cartridgeVertexCount];
        s_cartridgeVertexCount += count;
    } else {
        if (g_frameVertexCount + count > kFrameVertices) {
            g_frameVertexCount = 0;
        }
        target = &g_frameVertices[g_frameVertexCount];
        g_frameVertexCount += count;
    }
    bool bigEndian = CartridgeDataIsBigEndian(source);
    for (int i = 0; i < count; ++i) {
        Vtx vertex = source[i];
        if (bigEndian) {
            for (auto &coordinate : vertex.n.ob) {
                coordinate = static_cast<s16>(SwapHalf(static_cast<u16>(coordinate)));
            }
            for (auto &coordinate : vertex.n.tc) {
                coordinate = static_cast<s16>(SwapHalf(static_cast<u16>(coordinate)));
            }
        }
        target[i].position.x = vertex.n.ob[0];
        target[i].position.y = vertex.n.ob[1];
        target[i].position.z = vertex.n.ob[2];
        target[i].texture.x = vertex.n.tc[0];
        target[i].texture.y = vertex.n.tc[1];
        target[i].color.x = vertex.v.cn[0];
        target[i].color.y = vertex.v.cn[1];
        target[i].color.z = vertex.v.cn[2];
    }
    return target;
}

void PrepareDisplayList(GbiCommand *list) {
    bool cartridge = coastline::CartridgeData::IsCartridgeData(list);
    if (cartridge && !g_readyLists.insert(list).second) {
        return;
    }
    // The texture loads become the texture binds of the PC release, as for the display lists of
    // its models.
    PreprocessDisplayListTextures(list);
    for (GbiCommand *command = list;; ++command) {
        switch (command->w0 >> 24) {
        case G_ENDDL:
            return;
        case G_DL: {
            auto *target = static_cast<GbiCommand *>(coastline::DecodePtr32(command->w1));
            PrepareDisplayList(target);
            // A branch does not return.
            if (command->w0 >> 16 & 0xff) {
                return;
            }
            break;
        }
        case G_MTX:
            command->w1 = coastline::EncodePtr32(ConvertMatrix(
                static_cast<const Mtx *>(coastline::DecodePtr32(command->w1)), cartridge));
            break;
        case G_VTX: {
            int count = static_cast<int>(command->w0 >> 10 & 0x3f);
            int first = static_cast<int>(command->w0 >> 16 & 0xff) / 2;
            command->w1 = coastline::EncodePtr32(ConvertVertices(
                static_cast<const Vtx *>(coastline::DecodePtr32(command->w1)), count, cartridge));
            command->w0 = (command->w0 & 0xff00ffffu) | static_cast<u32>(first) << 16;
            break;
        }
        case G_TRI1:
            HalveGbiTriangleVertexIndices(command);
            break;
        case G_TRI2:
            HalveGbiTrianglePairVertexIndices(command);
            break;
        default:
            break;
        }
    }
}

void SubmitN64DisplayList(GbiCommand *list) {
    g_frameMatrixCount = 0;
    g_frameVertexCount = 0;
    PrepareDisplayList(list);
    g_pfnSubmitDisplayList(list);
}

} // namespace

bool CartridgeDataIsBigEndian(const void *host) {
    return coastline::CartridgeData::IsSegmentCopy(host);
}

void coastline::RenderN64DisplayLists() {
    if (g_pfnRenderBackendSubmitDisplayList != SubmitN64DisplayList) {
        g_pfnSubmitDisplayList = g_pfnRenderBackendSubmitDisplayList;
        g_pfnRenderBackendSubmitDisplayList = SubmitN64DisplayList;
    }
}
