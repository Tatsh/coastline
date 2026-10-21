/* Test of src/model.c and src/vehiclemodel.c: the stepping and blending of animations, the remap of
 * a display list, and the paint of a car. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "model.c"
#include "vehiclemodel.c"

f32 g_fFrameDeltaTime;
VehicleModel g_aVehicleModels[1];
CarModel g_aCarModels[CAR_MODELS];
s32 g_adwVehicleModelSlotCars[2];

u32 DmaReadAssetSize(u32 romOffset) {
    return 0;
}
void DmaRead(void *dst, u32 romOffset, u32 size) {
}
void DecompressFromRom(void *dst, u32 romOffset, DecompressJob *job) {
}
void osSyncPrintf(const char *format, ...) {
}
void SetDebugMessage(const char *message) {
}

/* A key with one vertex: its time, the position, and the normal after it. */
typedef struct {
    f32 time;
    s16 position[3];
    s8 normal[3];
} OneVertexKey;

int main(void) {
    static OneVertexKey keys[2] = {{0.0f, {0, 0, 0}, {0, 0, 0}},
                                   {1.0f, {400, -800, 40}, {64, 0, -64}}};
    static u8 animationBuffer[sizeof(ModelAnimation) + 2 * sizeof(ModelKeyframe *)];
    static u8 listBuffer[sizeof(ModelAnimationList) + sizeof(ModelAnimation *)];
    static u8 modelBuffer[sizeof(Model)];
    ModelAnimation *animation = (ModelAnimation *)animationBuffer;
    ModelAnimationList *list = (ModelAnimationList *)listBuffer;
    Model *model = (Model *)modelBuffer;
    Vtx vertex;
    Gfx displayList[3];
    static ModelTexture textures[3];
    static u16 palette[16];
    VehicleModel *car = &g_aVehicleModels[0];
    Vehicle vehicle;

    animation->vertexCount = 1;
    animation->vertices = &vertex;
    animation->keyCount = 2;
    animation->keys[0] = (ModelKeyframe *)&keys[0];
    animation->keys[1] = (ModelKeyframe *)&keys[1];
    animation->startTime = 0.0f;
    animation->endTime = 1.0f;
    animation->time = 0.0f;
    animation->flags = 0;
    animation->startKey = 1;
    list->count = 1;
    list->entries[0] = animation;
    model->animations = list;

    /* A quarter of the way, the vertex is a quarter of the way between the keys. */
    g_fFrameDeltaTime = 0.25f;
    AdvanceModelAnimations(model);
    assert(vertex.v.ob[0] == 100 && vertex.v.ob[1] == -200 && vertex.v.ob[2] == 10);
    assert(vertex.n.n[0] == 16 && vertex.n.n[2] == -16);

    /* Played once, it holds at the last key. */
    g_fFrameDeltaTime = 2.0f;
    AdvanceModelAnimations(model);
    assert(vertex.v.ob[0] == 400 && vertex.n.n[0] == 64);

    /* Looped, 2.75 wraps to 0.75. Bounced, 1.25 has turned back at the end to 0.75, and 2.75 runs
     * forwards again at 0.75. */
    animation->time = 0.75f;
    LoopModelAnimations(model);
    AdvanceModelAnimations(model);
    assert(fabsf(animation->time - 0.75f) < 1e-6f && vertex.v.ob[0] == 300);
    animation->time = 0.75f;
    BounceModelAnimations(model);
    AdvanceModelAnimations(model);
    assert(!(animation->flags & MODEL_ANIMATION_REVERSING));
    assert(fabsf(animation->time - 0.75f) < 1e-6f && vertex.v.ob[0] == 300);
    g_fFrameDeltaTime = 0.5f;
    AdvanceModelAnimations(model);
    assert(animation->flags & MODEL_ANIMATION_REVERSING);
    assert(fabsf(animation->time - 0.75f) < 1e-6f && vertex.v.ob[0] == 300);
    /* Running back, it reaches a quarter of the way. */
    AdvanceModelAnimations(model);
    assert(fabsf(animation->time - 0.25f) < 1e-6f && vertex.v.ob[0] == 100);
    PlayModelAnimationsOnce(model);
    assert((animation->flags & (MODEL_ANIMATION_REPEAT | MODEL_ANIMATION_BOUNCE)) == 0);

    /* The vertices and textures of a display list are remapped, other words are not. */
    displayList[0].words.w0 = (u32)G_VTX << 24;
    displayList[0].words.w1 = 0x100;
    displayList[1].words.w0 = (u32)G_SETTIMG << 24;
    displayList[1].words.w1 = 0x900;
    displayList[2].words.w0 = (u32)G_ENDDL << 24;
    displayList[2].words.w1 = 0x100;
    FixupDisplayListPointers(displayList, 0, 0x800, 0x5000);
    assert(displayList[0].words.w1 == 0x5100 && displayList[1].words.w1 == 0x900);
    assert(displayList[2].words.w1 == 0x100);

    /* The paint is set in the first two entries of the palette, and read back widened. */
    car->textures = textures;
    car->materialTextures[VEHICLE_MATERIAL_PAINT] = 1;
    textures[1].palette = palette;
    textures[1].format = TEXTURE_FORMAT_CI4;
    palette[2] = 1;
    SetMaterialColor(car, 31, 16, 0);
    assert(palette[0] == ((31 << 11) | (16 << 6) | 1));
    ExtractVehicleBaseColor(&vehicle, car);
    assert(vehicle.color[0] == 0xFF && vehicle.color[1] == 0x84 && vehicle.color[2] == 0);

    puts("model ok");
    return 0;
}
