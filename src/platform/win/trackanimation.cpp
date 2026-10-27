#include <bit>
#include <cstring>
#include <type_traits>

#include "platform/win/render.h"
#include "platform/win/trackrelocation.h"

template <typename T>
    requires(std::is_trivially_copyable_v<T> && sizeof(T) == sizeof(unsigned))
static inline void ByteSwapTrackField(T &field) {
    unsigned word;
    std::memcpy(&word, &field, sizeof(word));
    word = std::byteswap(word);
    std::memcpy(&field, &word, sizeof(word));
}

static inline void RelocateTrackPath(Vec3f *&path) {
    unsigned address;
    std::memcpy(&address, &path, sizeof(address));
    address = std::byteswap(address);
    RelocatePointerValue(&address);
    std::memcpy(&path, &address, sizeof(address));
}

void ByteSwapTrackSegmentList(TrackHeader *track) {
    TrackSection **section = track->sections;
    for (int i = 0; i < track->sectionCount; i++, section++) {
        unsigned address = std::byteswap(std::bit_cast<unsigned>(*section));
        RelocatePointerValue(&address);
        *section = std::bit_cast<TrackSection *>(address);
        ByteSwapTrackSegment(*section);
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
            RelocateTrackPath(special.target.path);
            ByteSwapTrackField(special.param.pathLength);
            pathLength = special.param.pathLength;
            for (int point = 0; point < pathLength; point++) {
                ByteSwap3DWords(&special.target.path[point]);
            }
            break;
        case TRACK_SPECIAL_AIRPLANE_PATH_RIGHT:
            RelocateTrackPath(special.target.path);
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
