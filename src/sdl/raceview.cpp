#include "raceview.h"

#include <array>
#include <cmath>
#include <string>

#include <SDL3/SDL.h>

#include "common/assetnames.h"
#include "common/render.h"
#include "common/sky.h"
#include "common/vehicle.h"
#include "common/weather.h"
#include "frontend.h"
#include "gpu.h"
#include "scenerenderer.h"

namespace coastline {

namespace {

// The perspective of the race: its near plane, as CAMERA_NEAR of the PC release, and its far plane,
// g_fCameraProjectionScale of a view of one player.
constexpr float CAMERA_NEAR = 0.8f;
constexpr float CAMERA_FAR = 400.0f;
constexpr float ASPECT_4_3 = 1.3333334f;

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

// The glTF of a model of the console stands it up from z up to y up, and this turns it back to the
// world of the race.
Matrix4 ConsoleSpace() {
    return RotationX(HALF_PI);
}

} // namespace

// The sky sits just inside the far end of the view along the depth of the camera, as RenderSkybox()
// places it.
constexpr float SKY_DEPTH_SCALE = 0.99f;

struct RaceView::State {
    std::unique_ptr<RenderModel> track;
    // ponytail: the sky loads the images of the track a second time; a model with its nodes drawn
    // by name would share them.
    std::unique_ptr<RenderModel> sky;
    std::unique_ptr<RenderModel> car;
};

RaceView::RaceView(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

RaceView::~RaceView() = default;

std::unique_ptr<RaceView>
RaceView::Load(SceneRenderer &renderer, const std::filesystem::path &defaults, int track, int car) {
    auto state = std::make_unique<State>();
    std::filesystem::path trackFile = defaults / "tracks" / g_apszTrackFilenames[track];
    trackFile.replace_extension(".gltf");
    state->track = RenderModel::Load(renderer, trackFile);
    state->sky = RenderModel::Load(renderer, trackFile, "sky");
    state->car = RenderModel::Load(
        renderer, defaults / "cars" / (std::string(g_apszCarModelPrefixes[car]) + ".gltf"));
    if (!state->track || !state->car) {
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

void RaceView::Draw(SceneRenderer &renderer) const {
    const State &s = *m_state;
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    const CameraFrame *camera = g_aVehicles[0].GetActiveCamera();
    if (camera == nullptr) {
        return;
    }
    Matrix4 viewProjection = CameraMatrix(*camera, aspect);

    // The lights of the weather of the race.
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
    renderer.DrawModel(*s.track, ConsoleSpace(), viewProjection, lighting, std::nullopt, true);
    renderer.DrawModel(*s.car,
                       MultiplyMatrices(FrameMatrix(g_aVehicles[0].GetFrame()), ConsoleSpace()),
                       viewProjection,
                       lighting,
                       std::nullopt,
                       true);
}

} // namespace coastline
