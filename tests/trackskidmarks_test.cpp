#include <array>
#include <cassert>
#include <cstring>
#include <random>
#include <vector>

#include "common/trackskidmarks.cpp"

int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];
int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];
s32 g_dwNumActiveVehicles;
u8 g_abInstanceVisibilityMask[256];
ConvertedModelVertex *g_pTrackQuadVertexStart;
ConvertedModelVertex *g_pTrackQuadVertexWritePtr;
int g_dwChosenWeather;
unsigned char g_abPtr32Anchor[1];

float RaycastTerrainHeight(Vec3f *) {
    return 0.0f;
}

namespace {
struct Quad {
    unsigned slot;
    unsigned long long revision;
    std::array<Vec3f, 4> corners;
};

void Collect(unsigned slot,
             unsigned long long revision,
             const Vec3f *oldRight,
             const Vec3f *newRight,
             const Vec3f *oldLeft,
             const Vec3f *newLeft,
             void *context) {
    static_cast<std::vector<Quad> *>(context)->push_back(
        {slot, revision, {*oldRight, *newRight, *oldLeft, *newLeft}});
}

void CompareDrawOrder() {
    std::mt19937 random(0x51d);
    std::array<ConvertedModelVertex, TRACK_QUAD_VERTICES> vertices{};
    std::array<GbiCommand, TRACK_QUAD_VERTICES> commands{};
    for (int trial = 0; trial < 300; ++trial) {
        g_dwNumActiveVehicles = 1 + random() % 8;
        for (int trail = 0; trail < SKID_MARK_TRACK_COUNT; ++trail) {
            g_anSkidMarkHead[trail] = random() % SEGMENT_COUNT;
            g_anSkidMarkTail[trail] = random() % SEGMENT_COUNT;
            for (auto &segment : g_aSkidMarkSegments[trail]) {
                segment.left = {static_cast<float>(random() % 100),
                                static_cast<float>(random() % 100),
                                static_cast<float>(random() % 100)};
                segment.right = {static_cast<float>(random() % 100),
                                 static_cast<float>(random() % 100),
                                 static_cast<float>(random() % 100)};
                segment.flags = random() % 256 | (random() % 4 ? ACTIVE : 0);
                segment.revision = random();
            }
        }
        for (auto &mask : g_abInstanceVisibilityMask) {
            mask = random();
        }
        int taken = trial % 3 == 0 ? 0 : random() % TRACK_QUAD_VERTICES;
        g_pTrackQuadVertexStart = vertices.data();
        g_pTrackQuadVertexWritePtr = vertices.data() + taken;
        GbiCommand *end = DrawSkidTrails(commands.data());
        std::vector<Quad> quads;
        int visited = taken;
        VisitSkidMarkQuads(Collect, &quads, g_abInstanceVisibilityMask, &visited);
        assert(visited == g_pTrackQuadVertexWritePtr - vertices.data());
        assert(end - commands.data() == static_cast<std::ptrdiff_t>(quads.size() * 2));
        for (const auto &quad : quads) {
            unsigned trail = quad.slot / SEGMENT_COUNT;
            unsigned index = quad.slot % SEGMENT_COUNT;
            const auto &segment = g_aSkidMarkSegments[trail][index];
            const auto &old = g_aSkidMarkSegments[trail][PreviousSegment(index)];
            assert(quad.revision ==
                   (static_cast<unsigned long long>(old.revision) << 32 | segment.revision));
            for (const auto &corner : quad.corners) {
                assert(std::memcmp(&corner, &vertices[taken++].position, sizeof(Vec3f)) == 0);
            }
        }
    }
}

void CheckRecordingRevisions() {
    std::memset(g_anSkidMarkHead, 0, sizeof(g_anSkidMarkHead));
    std::memset(g_anSkidMarkTail, 0, sizeof(g_anSkidMarkTail));
    std::memset(g_aSkidMarkSegments, 0, sizeof(g_aSkidMarkSegments));
    Vehicle vehicle{};
    std::array<RigidBody, 4> wheels{};
    for (unsigned wheel = 0; wheel < 4; ++wheel) {
        wheels[wheel].contactFrames = 1;
        vehicle.GetBody().wheels[wheel] = &wheels[wheel];
        vehicle.GetWheelFrames()[wheel].side.x = 1.0f;
    }
    g_dwChosenWeather = 2;
    g_dwNumActiveVehicles = 1;
    std::memset(g_abInstanceVisibilityMask, 0, sizeof(g_abInstanceVisibilityMask));
    std::array<Quad, 4 * SEGMENT_COUNT> cached{};
    unsigned rewrites = 0;
    unsigned wraps = 0;
    for (int frame = 0; frame < 1100; ++frame) {
        for (auto &wheel : vehicle.GetWheelFrames()) {
            wheel.position.x = frame * (frame % 3 ? 0.1f : 6.0f);
        }
        auto before = g_aSkidMarkSegments[0][PreviousSegment(g_anSkidMarkHead[0])].revision;
        int previous = PreviousSegment(g_anSkidMarkHead[0]);
        RecordTrackSkidMarks(&vehicle, 0);
        int written = PreviousSegment(g_anSkidMarkHead[0]);
        assert(g_aSkidMarkSegments[0][written].revision > 0);
        if (written == previous) {
            ++rewrites;
            assert(g_aSkidMarkSegments[0][written].revision == before + 1);
        } else if (written < previous) {
            ++wraps;
        }
        std::vector<Quad> quads;
        int count = 0;
        VisitSkidMarkQuads(Collect, &quads, g_abInstanceVisibilityMask, &count);
        for (const auto &quad : quads) {
            unsigned trail = quad.slot / SEGMENT_COUNT;
            int index = quad.slot % SEGMENT_COUNT;
            const auto &segment = g_aSkidMarkSegments[trail][index];
            const auto &old = g_aSkidMarkSegments[trail][PreviousSegment(index)];
            assert(quad.revision ==
                   (static_cast<unsigned long long>(old.revision) << 32 | segment.revision));
            auto &stored = cached[quad.slot];
            if (stored.revision != quad.revision) {
                stored = quad;
            }
            assert(std::memcmp(stored.corners.data(), quad.corners.data(), sizeof(quad.corners)) ==
                   0);
        }
    }
    assert(rewrites > 0 && wraps > 0);
    for (auto &wheel : wheels) {
        wheel.contactFrames = 0;
    }
    RecordTrackSkidMarks(&vehicle, 0);
    int head = g_anSkidMarkHead[0];
    RecordTrackSkidMarks(&vehicle, 0);
    assert(head == g_anSkidMarkHead[0]);
}
} // namespace

int main() {
    static_assert(SEGMENT_COUNT == SKID_MARK_SEGMENT_COUNT);
    CompareDrawOrder();
    CheckRecordingRevisions();
}
