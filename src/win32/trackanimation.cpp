#include <bit>
#include <cstring>
#include <type_traits>

#include "common/trackrelocation.h"
#include "win32/render.h"

template <typename T>
    requires(std::is_trivially_copyable_v<T> && sizeof(T) == sizeof(unsigned))
static inline void ByteSwapTrackField(T &field) {
    unsigned word;
    std::memcpy(&word, &field, sizeof(word));
    word = std::byteswap(word);
    std::memcpy(&field, &word, sizeof(word));
}

template <typename T>
    requires(std::is_object_v<T> && sizeof(T *) == sizeof(unsigned))
static inline void SwapAndRelocateTrackPointer(T *&pointer) {
    unsigned address = std::byteswap(std::bit_cast<unsigned>(pointer));
    RelocatePointerValue(&address);
    pointer = std::bit_cast<T *>(address);
}

template <typename T>
    requires(std::is_object_v<T> && sizeof(T *) == sizeof(unsigned))
static inline void RelocateTrackPointer(T *&pointer) {
    unsigned address = std::bit_cast<unsigned>(pointer);
    RelocatePointerValue(&address);
    pointer = std::bit_cast<T *>(address);
}

#if TGR_PLATFORM == TGR_PLATFORM_SDL
// The SDL game stores the pointers of a track in 32 bits, and relocates the stored word.
template <typename T>
static inline void SwapAndRelocateTrackPointer(coastline::Ptr32<T> &pointer) {
    unsigned address = std::byteswap(pointer.Raw());
    RelocatePointerValue(&address);
    pointer.SetRaw(address);
}

template <typename T>
static inline void RelocateTrackPointer(coastline::Ptr32<T> &pointer) {
    unsigned address = pointer.Raw();
    RelocatePointerValue(&address);
    pointer.SetRaw(address);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

void ReadAndRelocateTrackHeader(TrackHeader *track, GameFile *file) {
    CHK_FRead(track, 1, TRACK_HEADER_SIZE, file);
    ByteSwapTrackField(track->size);
    ByteSwapTrackField(track->headerSize);
    ByteSwapTrackField(track->surfaceCount);
    ByteSwapTrackField(track->surfaces);
    ByteSwapTrackField(track->vertexCount);
    ByteSwapTrackField(track->vertices);
    ByteSwapTrackField(track->textureCount);
    ByteSwapTrackField(track->textures);
    ByteSwapTrackField(track->gridTriangles);
    ByteSwapTrackField(track->gridCellStarts);
    ByteSwapTrackField(track->minX);
    ByteSwapTrackField(track->maxX);
    ByteSwapTrackField(track->minY);
    ByteSwapTrackField(track->maxY);
    ByteSwapTrackField(track->minZ);
    ByteSwapTrackField(track->maxZ);
    ByteSwapTrackField(track->startPosition.x);
    ByteSwapTrackField(track->startPosition.y);
    ByteSwapTrackField(track->startPosition.z);
    ByteSwapTrackField(track->startHeading);
    ByteSwapTrackField(track->skyDisplayList);
    ByteSwapTrackField(track->reservedSkyData[0]);
    ByteSwapTrackField(track->reservedSkyData[1]);
    ByteSwapTrackField(track->instanceNames);
    ByteSwapTrackField(track->instances);
    ByteSwapTrackField(track->instanceCount);
    ByteSwapTrackField(track->gridInstances);
    ByteSwapTrackField(track->gridInstanceStarts);
    ByteSwapTrackField(track->rootSection);
    ByteSwapTrackField(track->reservedRouteData);
    ByteSwapTrackField(track->sections);
    ByteSwapTrackField(track->sectionCount);
    ByteSwapTrackField(track->cameraPoints);
    ByteSwapTrackField(track->cameraPointCount);
    ByteSwapTrackField(track->nearbyInstances);
    ByteSwapTrackField(track->triangleNearbyInstances);
    ByteSwapTrackField(track->triangleFlags);
    for (TrackGate &gate : track->gates) {
        for (Vec2f &end : gate.ends) {
            ByteSwapTrackField(end.x);
            ByteSwapTrackField(end.y);
        }
        ByteSwapTrackField(gate.seconds);
    }
    ByteSwapTrackField(track->gateCount);
    RelocateTrackPointer(track->surfaces);
    RelocateTrackPointer(track->vertices);
    RelocateTrackPointer(track->textures);
    RelocateTrackPointer(track->gridTriangles);
    RelocateTrackPointer(track->gridCellStarts);
    RelocateTrackPointer(track->skyDisplayList);
    RelocateTrackPointer(track->reservedSkyData[0]);
    RelocateTrackPointer(track->reservedSkyData[1]);
    RelocateTrackPointer(track->instanceNames);
    RelocateTrackPointer(track->instances);
    RelocateTrackPointer(track->gridInstances);
    RelocateTrackPointer(track->gridInstanceStarts);
    RelocateTrackPointer(track->rootSection);
    RelocateTrackPointer(track->reservedRouteData);
    RelocateTrackPointer(track->sections);
    RelocateTrackPointer(track->cameraPoints);
    RelocateTrackPointer(track->nearbyInstances);
    RelocateTrackPointer(track->triangleNearbyInstances);
    RelocateTrackPointer(track->triangleFlags);
}

void ByteSwapTrackSegmentList(TrackHeader *track) {
    TGR_PTR(TrackSection) *section = track->sections;
    for (int i = 0; i < track->sectionCount; i++, section++) {
        SwapAndRelocateTrackPointer(*section);
        ByteSwapTrackSegment(*section);
    }
}

void ByteSwapTrackSegment(TrackSection *section) {
    SwapAndRelocateTrackPointer(section->child);
    SwapAndRelocateTrackPointer(section->next);
    SwapAndRelocateTrackPointer(section->parent);
    SwapAndRelocateTrackPointer(section->reverseNext);
    section->pointCount = std::byteswap(section->pointCount);
    section->flags = std::byteswap(section->flags);
    ByteSwapTerrainVertex(&section->previousPoint);
    for (int i = 0; i <= section->pointCount; i++) {
        ByteSwapTerrainVertex(&section->points[i]);
    }
}

void ByteSwapTrackAnimationTable(TrackHeader *track) {
    int pathLength = 0;
    ByteSwapTrackField(track->specialCount);
    for (int i = 0; i < track->specialCount; i++) {
        TrackSpecial &special = track->specials[i];
        switch (special.type) {
        case TRACK_SPECIAL_SPIN_Z:
        case TRACK_SPECIAL_SPIN_X:
        case TRACK_SPECIAL_SPIN_Y:
            ByteSwapTrackField(special.target.instance);
            ByteSwapTrackField(special.param.spinAngle);
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_LEFT:
            SwapAndRelocateTrackPointer(special.target.path);
            ByteSwapTrackField(special.param.pathLength);
            pathLength = special.param.pathLength;
            for (int point = 0; point < pathLength; point++) {
                ByteSwap3DWords(&special.target.path[point]);
            }
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_RIGHT:
            SwapAndRelocateTrackPointer(special.target.path);
            /* The right path uses the most recent left path's node count. */
            for (int point = 0; point < pathLength; point++) {
                ByteSwap3DWords(&special.target.path[point]);
            }
            break;
        case TRACK_SPECIAL_AIRPLANE:
        case TRACK_SPECIAL_AIRPLANE_TRIGGER:
        case TRACK_SPECIAL_WATERFALL:
            ByteSwapTrackField(special.target.instance);
            break;
        }
    }
}
