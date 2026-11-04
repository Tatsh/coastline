#include "common/soundpan.h"

#if TGR_BOSS_RALLY
typedef long double PanReal;
#define PAN_INTERMEDIATE(value) ((long double)(value))
#define PAN_LESS(a, b) (!((a) >= (b)))
#else
typedef f32 PanReal;
#define PAN_INTERMEDIATE(value) (value)
#define PAN_LESS(a, b) ((a) < (b))
#endif

/* A sound pans over 10 units across the camera. The far side is amplified, and the near side gains
 * 60% of its remaining volume before full. A narrow pan moves 40% as far. */
#define PAN_RANGE 10.0f
#define PAN_NARROW 0.4f
#if TGR_BOSS_RALLY
#define PAN_LOUD 1.6f
#else
#define PAN_LOUD 1.7f
#endif
#define PAN_QUIET_RAISE 0.6f

/* The volume of a sound is 1024 over its distance, from no nearer than 32. */
#define SOUND_VOLUME_SCALE 1024.0f
#define SOUND_NEAREST 32.0

void ComputeStereoPan(
    Vec3f *source, CameraFrame *camera, f32 *upper, f32 *lower, s32 *volume, s32 narrow) {
#if !TGR_BOSS_RALLY
    union {
        Vec3f value;
        f32 coordinates[3];
    } offset;
    f32 share;
    f32 rest;
    f32 distance;
    f32(*rows)[4];

    Vec3fSubtract(&offset.value, source, &camera->position);
    /* The first four rows of a camera frame are its 4 by 4 matrix. */
    rows = (f32(*)[4])camera;
    share = offset.coordinates[0] * rows[1][0] + offset.coordinates[1] * rows[1][1] +
            offset.coordinates[2] * rows[1][2];
    if (PAN_RANGE < share) {
        share = PAN_RANGE;
    } else if (share < -PAN_RANGE) {
        share = -PAN_RANGE;
    }
    if (narrow != 0) {
        share *= PAN_NARROW;
    }
    share = (share + PAN_RANGE) / (PAN_RANGE + PAN_RANGE);
    rest = 1.0f - share;
    if (share < rest) {
        share *= PAN_LOUD;
        rest += PAN_QUIET_RAISE * (1.0f - rest);
    } else {
        rest *= PAN_LOUD;
        share += PAN_QUIET_RAISE * (1.0f - share);
    }
    *upper = share;
    *lower = rest;
    distance = Vec3fLength(&offset.value);
    if (distance < SOUND_NEAREST) {
        distance = SOUND_NEAREST;
    }
    *volume = (s32)(SOUND_VOLUME_SCALE / distance);
#else
    Vec3f offset;
    f32 share;
    PanReal rest;
    PanReal distance;
    PanReal projected;

    Vec3fSubtract(&offset, source, &camera->position);
    /* The distance across the camera first, then the share of the upper half. */
    projected =
        PAN_INTERMEDIATE(camera->side.z) * offset.z +
        (PAN_INTERMEDIATE(offset.x) * camera->side.x + PAN_INTERMEDIATE(offset.y) * camera->side.y);
    if (PAN_RANGE < projected) {
        projected = PAN_RANGE;
    } else if (PAN_LESS(projected, -PAN_RANGE)) {
        projected = -PAN_RANGE;
    }
    if (narrow) {
        projected *= PAN_NARROW;
    }
    share = (projected + PAN_RANGE) * 0.05f;
    rest = PAN_INTERMEDIATE(1.0f) - share;
    /* Each bound passes an unordered value. */
    if (!(0.49f > share) && !(share > 0.51f) && !(0.49f > rest) && !(rest > 0.51f)) {
        share = 0.5f;
        rest = 0.5f;
    }
    if (PAN_LESS(share, rest)) {
        share *= PAN_LOUD;
        rest += PAN_QUIET_RAISE * (1.0f - rest);
    } else {
        rest *= PAN_LOUD;
        share += PAN_QUIET_RAISE * (PAN_INTERMEDIATE(1.0f) - share);
    }
    *upper = share;
    *lower = rest;
    distance = Vec3fLength(&offset);
    if (PAN_LESS(distance, SOUND_NEAREST)) {
        distance = SOUND_NEAREST;
    }
    *volume = (s32)(PAN_INTERMEDIATE(SOUND_VOLUME_SCALE) / distance);
#endif
}
