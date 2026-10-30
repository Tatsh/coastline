#include "raceview.h"

#include <array>
#include <cmath>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>

#include "common/assetnames.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/hudassets.h"
#include "common/model.h"
#include "common/particlesnapshot.h"
#include "common/race.h"
#include "common/rallyconfig.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/sky.h"
#include "common/snapshot.h"
#include "common/strings.h"
#include "common/system.h"
#include "common/track.h"
#include "common/trackmenuitem.h"
#include "common/trackselect.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/vehiclecamera.h"
#include "common/weather.h"
#include "frontend.h"
#include "gpu.h"
#include "menutext.h"
#include "scenerenderer.h"

namespace coastline {

namespace {

// The perspective of the race: its near plane, as CAMERA_NEAR of the PC release, and its far plane,
// g_fCameraProjectionScale of a view of one player.
constexpr float CAMERA_NEAR = 0.8f;
constexpr float CAMERA_FAR = 400.0f;
constexpr float ASPECT_4_3 = 1.3333334f;

// The string of the help the attract demo shows, "Press ESC to exit".
constexpr unsigned DEMO_HELP_STRING = 244;

std::array<float, 3> Normalized(std::array<float, 3> v) {
    float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length > 0.0f) {
        for (float &f : v) {
            f /= length;
        }
    }
    return v;
}

std::array<float, 3> Cross(const std::array<float, 3> &a, const std::array<float, 3> &b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

float Dot(const std::array<float, 3> &a, const std::array<float, 3> &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

std::array<float, 3> Vector(const Vec3f &v) {
    return {v.x, v.y, v.z};
}

// SetupCameraMatrix() of the PC release: guLookAtF() from the camera along its forward axis, and a
// perspective whose field across the frame is that of the camera on a 4:3 screen.
Matrix4 CameraMatrix(const CameraFrame &camera, float aspect) {
    std::array<float, 3> eye = Vector(camera.position);
    std::array<float, 3> forward = Normalized(Vector(camera.forward));
    std::array<float, 3> side = Normalized(Cross(forward, Vector(camera.up)));
    std::array<float, 3> up = Cross(side, forward);
    Matrix4 view{side[0],
                 up[0],
                 -forward[0],
                 0,
                 side[1],
                 up[1],
                 -forward[1],
                 0,
                 side[2],
                 up[2],
                 -forward[2],
                 0,
                 -Dot(side, eye),
                 -Dot(up, eye),
                 Dot(forward, eye),
                 1};
    float fovy = camera.fov * ASPECT_4_3 / aspect;
    float focal = 1.0f / std::tan(fovy * 0.5f);
    Matrix4 projection{focal / aspect,
                       0,
                       0,
                       0,
                       0,
                       focal,
                       0,
                       0,
                       0,
                       0,
                       CAMERA_FAR / (CAMERA_NEAR - CAMERA_FAR),
                       -1,
                       0,
                       0,
                       CAMERA_NEAR * CAMERA_FAR / (CAMERA_NEAR - CAMERA_FAR),
                       0};
    return MultiplyMatrices(projection, view);
}

// The place of a frame of the engine, whose rows are its axes and its position.
Matrix4 FrameMatrix(const CameraFrame &frame) {
    return {frame.forward.x,
            frame.forward.y,
            frame.forward.z,
            0,
            frame.side.x,
            frame.side.y,
            frame.side.z,
            0,
            frame.up.x,
            frame.up.y,
            frame.up.z,
            0,
            frame.position.x,
            frame.position.y,
            frame.position.z,
            1};
}

// The canvas the HUD of the race is laid out on: the frame in points of the operating system, so
// the HUD keeps the size the display scale gives it, from its top left with y down onto the whole
// frame.
struct HudCanvas {
    int width;
    int height;
    Matrix4 projection;
};

HudCanvas MakeHudCanvas(const GpuWindow &window) {
    float scale = window.DisplayScale();
    int width = std::max(static_cast<int>(static_cast<float>(window.FrameWidth()) / scale), 1);
    int height = std::max(static_cast<int>(static_cast<float>(window.FrameHeight()) / scale), 1);
    return {width,
            height,
            {2.0f / static_cast<float>(width),
             0,
             0,
             0,
             0,
             -2.0f / static_cast<float>(height),
             0,
             0,
             0,
             0,
             1,
             0,
             -1.0f,
             1.0f,
             0,
             1}};
}

// The glTF of a model of the console stands it up from z up to y up, and this turns it back to the
// world of the race.
Matrix4 ConsoleSpace() {
    return RotationX(HALF_PI);
}

} // namespace

// The sky sits just inside the far end of the view along the depth of the camera, as RenderSkybox()
// places it.
constexpr float SKY_DEPTH_SCALE = 0.99f;

namespace {

// RenderStartLights(): the lights face across the track from the right edge of its first point,
// lowered as they drop away, scaled by 1/1024.
std::optional<Matrix4> StartLightsMatrix() {
    if (!g_bStartLightsVisible || g_LoadedTrack.rootSection == nullptr) {
        return std::nullopt;
    }
    TrackPoint &first = g_LoadedTrack.rootSection->points[0];
    Vec3f across{};
    Vec3f forward{};
    Vec3f up{0.0f, 0.0f, 1.0f};
    Vec3fDirection(&across, &first.position, &first.right);
    Vec3fCross(&forward, &across, &up);
    Vec3fCross(&across, &up, &forward);
    Vec3f position{
        first.position.x,
        first.position.y,
        static_cast<f32>(static_cast<long double>(first.position.z) + g_fStartLightsDrop + 2.0f)};
    Vec3fMulAdd(&position, &position, &forward, -0.3f);
    Vec3fMulAdd(&position, &position, &across, -0.6f);
    Matrix4 place{forward.x,
                  forward.y,
                  forward.z,
                  0,
                  across.x,
                  across.y,
                  across.z,
                  0,
                  up.x,
                  up.y,
                  up.z,
                  0,
                  position.x,
                  position.y,
                  position.z,
                  1};
    return MultiplyMatrices(place, Scale(1.0f / 1024.0f));
}

// The poses of the animations of the start lights at the times the race has them.
std::vector<MorphPose> StartLightsPoses(const RenderModel &lights) {
    std::vector<MorphPose> poses;
    if (g_pStartLightsModel == nullptr || g_pStartLightsModel->animations == nullptr) {
        return poses;
    }
    const auto &animations = lights.Animations();
    int count = g_pStartLightsModel->animations->count;
    for (int i = 0; i < count && static_cast<std::size_t>(i) < animations.size(); ++i) {
        std::optional<MorphPose> pose =
            PoseAtTime(animations[static_cast<std::size_t>(i)],
                       g_pStartLightsModel->animations->entries[i]->time);
        if (pose) {
            poses.push_back(*pose);
        }
    }
    return poses;
}

} // namespace

struct RaceView::State {
    std::unique_ptr<RenderModel> track;
    // ponytail: the sky loads the images of the track a second time; a model with its nodes drawn
    // by name would share them.
    std::unique_ptr<RenderModel> sky;
    std::unique_ptr<RenderModel> car;
    std::unique_ptr<RenderModel> underside;
    std::unique_ptr<RenderModel> wheel;
    std::unique_ptr<RenderModel> snowWheel;
    std::unique_ptr<RenderModel> startLights;
    std::unique_ptr<MenuText> text;
    // The images of the day and the night sky the paint and the windows of the car reflect.
    std::unique_ptr<RenderTexture> daySky;
    std::unique_ptr<RenderTexture> nightSky;
    // The shadow of the car, the 8-bit intensities of the engine, the alpha the shade.
    std::unique_ptr<RenderTexture> shadowTexture;
    // LoadParticleTexture(): the 64 by 64 intensities of a puff of smoke.
    std::unique_ptr<RenderTexture> particleTexture;
};

namespace {

// The model of a car of the PC release on a cartridge, by the prefix of its files: the releases of
// the United States, Japan, and Europe in turn, as the cars of the PC release that the cartridge of
// the United States does not have are on the Japanese one. None when no cartridge has the car.
std::optional<std::filesystem::path> CartridgeCar(const std::filesystem::path &defaults,
                                                  const std::string &prefix) {
    static constexpr std::array<std::pair<std::string_view, std::string_view>, 4> NAMED{
        {{"bb", "beach-ball"}, {"cu", "cupra"}, {"hm", "helmet"}, {"mt", "milk-truck"}}};
    std::string name = "type-" + prefix;
    for (const auto &[from, to] : NAMED) {
        if (prefix == from) {
            name = to;
        }
    }
    for (std::string_view release : {"usa", "jpn", "eur"}) {
        auto model = defaults / "n64" / release / "cars" / name / "model.gltf";
        std::error_code error;
        if (std::filesystem::exists(model, error)) {
            return model;
        }
    }
    return std::nullopt;
}

// The flag of an instance of a track that is a tunnel.
constexpr std::uint32_t TRACK_INSTANCE_TUNNEL = 0x10;

// The reflection of the sky on the car as RenderVehicleModel() sets it up: the night sky at night
// or in a tunnel, else the day sky, across the axes of guLookAtReflectF() from the heading of the
// camera, with the window of the tile turning with the heading of the camera of the race.
Reflection SkyReflection(const RenderTexture *daySky,
                         const RenderTexture *nightSky,
                         const CameraFrame &camera,
                         const Vehicle &car) {
    bool inTunnel =
        car.GetGroundInstanceCount() != 0 &&
        (g_LoadedTrack.instances[car.GetGroundInstances()[0]].flags & TRACK_INSTANCE_TUNNEL) != 0;
    Reflection reflection;
    reflection.texture = g_dwNightMode || inTunnel ? nightSky : daySky;
    float eyeX = camera.forward.x;
    float eyeY = camera.forward.y;
    if (eyeX == 0.0f && eyeY == 0.0f) {
        eyeX = 1e-4f;
    }
    std::array<float, 3> look = Normalized({-eyeX, -eyeY, 0.0f});
    reflection.across = {-look[1], look[0], 0.0f};
    reflection.up = {0.0f, 0.0f, 1.0f};
    long double heading = g_aVehicles[0].GetCameraHeading();
    auto offset = static_cast<std::int32_t>(
        0xffffffdfu -
        static_cast<std::uint32_t>(static_cast<std::int64_t>(heading * -20.3718318939209L)));
    reflection.column = static_cast<float>(offset + 2) / 4.0f;
    // The reflection blends towards the fog, more in a tunnel, and not at all in clear weather.
    int alpha = 0xFF;
    if (g_dwWeatherActive) {
        alpha = inTunnel ? (g_bFogIntensity >> 3) + 0xDF : (g_bFogIntensity >> 1) + 0x7F;
    }
    reflection.environment = {static_cast<float>(g_bFogR) / 255.0f,
                              static_cast<float>(g_bFogG) / 255.0f,
                              static_cast<float>(g_bFogB) / 255.0f,
                              static_cast<float>(alpha) / 255.0f};
    return reflection;
}

// The scales of the shadow of a car along it and across it, for the first wheel of its model.
constexpr float SHADOW_LENGTH_SCALE = 235.52f;
constexpr float SHADOW_WIDTH_SCALE = 161.28f;

// The shadow of a car as RenderTrackInstance() lays it on the instances near the car, or none
// where the race does not draw it: with the shadows off, from the mirror, or from the cockpit or
// the bumper of the car.
std::optional<CarShadow> ShadowOf(const RenderTexture *texture, const Vehicle &car) {
    const CameraFrame *active = g_aVehicles[0].GetActiveCamera();
    const auto &cameras = g_aVehicles[0].GetCameras();
    if (texture == nullptr || !g_GlobalRallyConfig.bCarShadowSelection ||
        active == &cameras[VEHICLE_CAMERA_MIRROR] || active == &cameras[VEHICLE_CAMERA_COCKPIT] ||
        active == &cameras[VEHICLE_CAMERA_BUMPER]) {
        return std::nullopt;
    }
    const CameraFrame &frame = car.GetFrame();
    Vec3f forward = frame.forward;
    if (forward.x == 0.0f && forward.y == 0.0f) {
        forward = frame.up;
    }
    float length = std::max(std::hypot(forward.x, forward.y), 0.5f);
    float width = std::max(std::hypot(frame.side.x, frame.side.y), 0.5f);
    const VehicleModel *model = car.GetModel();
    CarShadow shadow;
    shadow.texture = texture;
    shadow.x = frame.position.x;
    shadow.y = frame.position.y;
    shadow.along = SHADOW_LENGTH_SCALE / model->wheels[0].x / length;
    shadow.across = SHADOW_WIDTH_SCALE / model->wheels[0].y / width;
    shadow.forward = {forward.x, forward.y};
    float fog = car.GetFogFactor();
    shadow.color = {static_cast<float>(g_bFogR) / 255.0f * fog,
                    static_cast<float>(g_bFogG) / 255.0f * fog,
                    static_cast<float>(g_bFogB) / 255.0f * fog,
                    1.0f};
    std::vector<u8> nearby(static_cast<std::size_t>(g_LoadedTrack.instanceCount), 0);
    car.MarkNearbyInstanceShadows(nearby.data(), 1);
    for (int index = 0; index < g_LoadedTrack.instanceCount; ++index) {
        if (nearby[static_cast<std::size_t>(index)] != 0 &&
            !(g_LoadedTrack.instances[index].flags & TRACK_INSTANCE_NO_SHADOW)) {
            shadow.instances.push_back(index);
        }
    }
    return shadow;
}

// WriteParticleCombineMode(): the colour of a puff, its alpha the texel by its opacity. The
// colour and the opacity of a puff come in the colour of its corners.
constexpr std::array<std::uint32_t, 2> PARTICLE_COMBINE = CombineLerp(ZERO_AB,
                                                                      ZERO_AB,
                                                                      ZERO_D,
                                                                      SHADE,
                                                                      TEXEL0,
                                                                      ZERO_ALPHA,
                                                                      SHADE,
                                                                      ZERO_ALPHA,
                                                                      ZERO_AB,
                                                                      ZERO_AB,
                                                                      ZERO_D,
                                                                      SHADE,
                                                                      TEXEL0,
                                                                      ZERO_ALPHA,
                                                                      SHADE,
                                                                      ZERO_ALPHA);

// RenderBillboardParticles(): each puff of a list faces the camera, its height 16/3 and its width
// 4 times its size over the focal length of the camera on a screen of 4:3, as the rectangles of the
// PC release are drawn, in a colour with the opacity of the puff.
void AddBillboards(std::vector<FlatVertex> &quads,
                   u32 head,
                   std::array<std::uint8_t, 3> color,
                   const CameraFrame &camera,
                   float textureScale) {
    std::array<float, 3> forward = Normalized(Vector(camera.forward));
    std::array<float, 3> side = Normalized(Cross(forward, Vector(camera.up)));
    std::array<float, 3> up = Cross(side, forward);
    float focal = 1.0f / std::tan(camera.fov * 0.5f);
    for (u32 index = head; index != 0; index = g_pParticleListData[index].next) {
        const Particle &particle = g_pParticleListData[index];
        float halfHeight = particle.size * (16.0f / 3.0f) / focal;
        float halfWidth = particle.size * 4.0f / focal;
        auto opacity = static_cast<std::uint8_t>((particle.alpha * particle.fade) >> 8);
        std::array<std::uint8_t, 4> rgba{color[0], color[1], color[2], opacity};
        auto corner = [&](float across, float down, float u, float v) {
            quads.push_back({particle.position.x + side[0] * across - up[0] * down,
                             particle.position.y + side[1] * across - up[1] * down,
                             u * textureScale,
                             v * textureScale,
                             rgba,
                             particle.position.z + side[2] * across - up[2] * down});
        };
        corner(-halfWidth, -halfHeight, 0.0f, 0.0f);
        corner(halfWidth, -halfHeight, 1.0f, 0.0f);
        corner(halfWidth, halfHeight, 1.0f, 1.0f);
        corner(-halfWidth, halfHeight, 0.0f, 1.0f);
    }
}

// RenderTireSmoke() by day, in the colours of the dirt and of the pavement of the track, and
// RenderExhaustAndTireSpray() in the snow, the exhaust in the half of its texture the PC release
// samples.
void DrawParticles(SceneRenderer &renderer,
                   const RenderTexture &smoke,
                   const RenderTexture &exhaust,
                   const CameraFrame &camera,
                   const Matrix4 &viewProjection) {
    FlatMaterial material;
    material.combine = PARTICLE_COMBINE;
    if (!g_dwNightMode) {
        std::array<std::uint8_t, 3> dirt{0xA0, 0x88, 0x60};
        std::array<std::uint8_t, 3> paved{0x70, 0x68, 0x58};
        if (g_dwSelectedTrackId == TRACK_MOUNTAIN || g_dwSelectedTrackId == TRACK_MIRROR_MOUNTAIN) {
            dirt = {0x60, 0x54, 0x38};
            paved = {0x60, 0x5C, 0x50};
        } else if (g_dwSelectedTrackId == TRACK_JUNGLE ||
                   g_dwSelectedTrackId == TRACK_MIRROR_JUNGLE) {
            dirt = {0x70, 0x58, 0x38};
            paved = {0x70, 0x68, 0x58};
        }
        std::vector<FlatVertex> quads;
        AddBillboards(quads, g_wRenderTireSmokeListHead, dirt, camera, 1.0f);
        AddBillboards(quads, g_wRenderPavedTireSmokeListHead, paved, camera, 1.0f);
        if (!quads.empty()) {
            material.texel0 = &smoke;
            renderer.DrawQuads(quads, material, viewProjection, true);
        }
    }
    if (g_dwSnowMode) {
        std::vector<FlatVertex> quads;
        AddBillboards(quads, g_wRenderExhaustParticleListHead, {0xE0, 0xE0, 0xFF}, camera, 0.5f);
        if (!quads.empty()) {
            material.texel0 = &exhaust;
            renderer.DrawQuads(quads, material, viewProjection, true);
        }
    }
}

} // namespace

RaceView::RaceView(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

RaceView::~RaceView() = default;

std::unique_ptr<RaceView> RaceView::Load(SceneRenderer &renderer,
                                         const std::filesystem::path &defaults,
                                         int track,
                                         int car,
                                         std::array<std::uint8_t, 3> color) {
    auto state = std::make_unique<State>();
    std::filesystem::path trackFile = defaults / "tracks" / g_apszTrackFilenames[track];
    trackFile.replace_extension(".gltf");
    state->track = RenderModel::Load(renderer, trackFile);
    ModelOptions sky;
    sky.node = "sky";
    state->sky = RenderModel::Load(renderer, trackFile, sky);
    // The car of the nearest level of detail, the one level a single view draws, in its parts:
    // the body, the underside, and a wheel of each kind, which the race places one by one.
    // ponytail: each part loads the images of the car again; a model drawn by parts would share
    // them.
    // The geometry of the car is that of the PC release, which its race moves, and its textures
    // are those of the cartridge.
    std::string prefix = g_apszCarModelPrefixes[car];
    auto carFile = defaults / "cars" / (prefix + ".gltf");
    ModelOptions options;
    options.paint = color;
    options.images = CartridgeCar(defaults, prefix);
    options.parts = [](const std::string &n) {
        return n == "lod0-nearDetail" || n == "lod0-paint" || n == "lod0-textured";
    };
    state->car = RenderModel::Load(renderer, carFile, options);
    options.parts = [](const std::string &n) { return n == "lod0-underside"; };
    state->underside = RenderModel::Load(renderer, carFile, options);
    options.parts = nullptr;
    options.node = "lod0-wheel0";
    state->wheel = RenderModel::Load(renderer, carFile, options);
    options.node = "lod0-wheelSnow0";
    state->snowWheel = RenderModel::Load(renderer, carFile, options);
    // The skies of the track in cargfx, as the import converted them.
    auto skyImage = [&](const SkyTextureFiles *files) {
        // The reflection wraps across the sky, as its tile wraps.
        return RenderTexture::Load(
            renderer, (defaults / files->image).replace_extension(".png"), true, true);
    };
    state->daySky = skyImage(g_aTrackDescriptors[track].daySkyFiles);
    state->nightSky = skyImage(g_aTrackDescriptors[track].nightSkyFiles);
    // LoadHudOverlayTexture(): the 64 by 64 intensities of the shadow, clamped at its edges.
    std::vector<std::uint8_t> shadow;
    for (unsigned char level : g_abHudOverlayTextureData) {
        shadow.insert(shadow.end(), {level, level, level, level});
    }
    state->shadowTexture = RenderTexture::FromTexels(renderer, 64, 64, shadow, true, false);
    std::vector<std::uint8_t> puff;
    for (unsigned char level : g_abHudTexture64AData) {
        puff.insert(puff.end(), {level, level, level, level});
    }
    state->particleTexture = RenderTexture::FromTexels(renderer, 64, 64, puff, true, true);
    // The race of the PC release draws its text with the fonts of the cartridge.
    state->text = MenuText::Load(renderer, defaults / "n64" / "usa" / "fonts");
    // The start lights, the model of BossRally.pod.
    state->startLights = RenderModel::Load(renderer, defaults / "BossRally" / "modellights.gltf");
    if (!state->track || !state->car || !state->wheel || !state->text || !state->startLights) {
        return nullptr;
    }
    return std::unique_ptr<RaceView>(new RaceView(std::move(state)));
}

std::array<float, 4> RaceView::ClearColor() const {
    return {static_cast<float>(g_bFogR) / 255.0f,
            static_cast<float>(g_bFogG) / 255.0f,
            static_cast<float>(g_bFogB) / 255.0f,
            1.0f};
}

void RaceView::Draw(SceneRenderer &renderer, float share) const {
    const State &s = *m_state;
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    // The frame draws the snapshots of the race blended, or the race itself before it has two.
    const FrameSnapshot *snapshot = InterpolateFrameSnapshots(share);
    const Vehicle &vehicle = snapshot != nullptr ? snapshot->GetVehicle(0) : g_aVehicles[0];
    const CameraFrame *camera = vehicle.GetActiveCamera();
    if (camera == nullptr) {
        return;
    }
    // The race keeps the 640 by 480 screen of the PC release, and the HUD is drawn on the canvas of
    // the display scale from the same offsets from the edges of the screen.
    HudCanvas hud = MakeHudCanvas(window);
    Matrix4 viewProjection = CameraMatrix(*camera, aspect);

    // The lights of the weather of the race.
    // The animated textures of the track take the time of the race.
    renderer.SetTextureTime(static_cast<std::uint32_t>(g_dwElapsedTimeMs));
    SetupWeatherLights();
    Lighting lighting;
    lighting.direction = {g_afLightDirection[0], g_afLightDirection[1], g_afLightDirection[2]};
    lighting.ambient = {static_cast<float>(g_bAmbientLightR) / 255.0f,
                        static_cast<float>(g_bAmbientLightG) / 255.0f,
                        static_cast<float>(g_bAmbientLightB) / 255.0f};
    lighting.directional = {static_cast<float>(g_bDirLightR) / 255.0f,
                            static_cast<float>(g_bDirLightG) / 255.0f,
                            static_cast<float>(g_bDirLightB) / 255.0f};

    // RenderSkybox(): the sky centred on the camera without the depth, unless the fog fills it with
    // the clear colour. Its node is in the world of the race, outside the root of the track that
    // stands the track up.
    if (s.sky && !IsSkyFogFilled() && !g_dwSkyboxDisabled) {
        Matrix4 place = Translation(
            camera->position.x, camera->position.y, camera->position.z * SKY_DEPTH_SCALE);
        renderer.DrawModel(*s.sky, place, viewProjection, lighting, std::nullopt, false);
    }
    std::optional<CarShadow> shadow = ShadowOf(s.shadowTexture.get(), vehicle);
    if (shadow) {
        renderer.DrawShadowedModel(
            *s.track, ConsoleSpace(), viewProjection, lighting, std::move(*shadow));
    } else {
        renderer.DrawModel(*s.track, ConsoleSpace(), viewProjection, lighting, std::nullopt, true);
    }
    // RenderVehicleModel() does not draw the car from its own cockpit or its mirror.
    const CameraFrame *active = g_aVehicles[0].GetActiveCamera();
    const auto &cameras = g_aVehicles[0].GetCameras();
    bool inside =
        active == &cameras[VEHICLE_CAMERA_COCKPIT] || active == &cameras[VEHICLE_CAMERA_MIRROR];
    if (!inside || g_dwVehicleCullingDisabled) {
        Matrix4 body = MultiplyMatrices(FrameMatrix(vehicle.GetFrame()), ConsoleSpace());
        renderer.DrawReflectingModel(
            *s.car,
            body,
            viewProjection,
            lighting,
            SkyReflection(s.daySky.get(), s.nightSky.get(), *camera, vehicle));
        // The underside shows only from below the car.
        const CameraFrame &frame = vehicle.GetFrame();
        float below = frame.up.x * (frame.position.x - camera->position.x) +
                      frame.up.y * (frame.position.y - camera->position.y) +
                      frame.up.z * (frame.position.z - camera->position.z);
        if (s.underside && below > 0.0f) {
            renderer.DrawModel(*s.underside, body, viewProjection, lighting, std::nullopt, true);
        }
        // RenderWheelModels(): each wheel at the matrix the race gives it, in snow the wheel of
        // the snow.
        const RenderModel &wheel = g_dwSnowMode && s.snowWheel ? *s.snowWheel : *s.wheel;
        for (const auto &rows : vehicle.GetWheelMatrices()) {
            Matrix4 place{};
            std::memcpy(place.data(), rows, sizeof(place));
            renderer.DrawModel(wheel, place, viewProjection, lighting, std::nullopt, true);
        }
    }
    // The puffs of the snapshot the frame draws.
    if (snapshot != nullptr) {
        LoadParticleListHeads(&snapshot->GetParticles());
        DrawParticles(renderer, *s.particleTexture, *s.shadowTexture, *camera, viewProjection);
    }
    std::optional<Matrix4> lights = StartLightsMatrix();
    if (lights) {
        renderer.DrawModel(*s.startLights,
                           MultiplyMatrices(*lights, ConsoleSpace()),
                           viewProjection,
                           lighting,
                           StartLightsPoses(*s.startLights),
                           true);
    }

    // The help of the attract demo, from the corner of the canvas of the HUD, under the fade.
    const char *help = GetStringResource(DEMO_HELP_STRING);
    if (g_bShowHudHelpText && g_dwGameMode == GAME_MODE_DEMO &&
        g_dwRaceSessionType == RACE_SESSION_ATTRACT && help != nullptr) {
        TextStyle style;
        style.size = 15;
        style.align = TextStyle::Align::Right;
        style.doubled = false;
        s.text->Draw(renderer, help, hud.width - 28, hud.height - 24, style, hud.projection);
    }

    // ApplyScreenFade(): black left and right of the edges of the fade, across the whole frame.
    if (g_fFadeLevel != 1.0f && g_dwScreenWidth > 0) {
        ScreenFade wipe;
        wipe.level = g_fFadeLevel;
        wipe.left = g_dwFadeRectLeft * static_cast<int>(SCREEN_WIDTH) / g_dwScreenWidth;
        wipe.right = g_dwFadeRectRight * static_cast<int>(SCREEN_WIDTH) / g_dwScreenWidth;
        wipe.Draw(renderer);
    }
}

} // namespace coastline
