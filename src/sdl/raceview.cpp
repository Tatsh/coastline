#include "raceview.h"

#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <string>

#include <SDL3/SDL.h>

#include "common/assetnames.h"
#include "common/camera.h"
#include "common/displaylistptr.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/hudassets.h"
#include "common/hudcanvas.h"
#include "common/input.h"
#include "common/instantreplay.h"
#include "common/model.h"
#include "common/packedvalue.h"
#include "common/particlesnapshot.h"
#include "common/race.h"
#include "common/racerender.h"
#include "common/racerslot.h"
#include "common/rallyconfig.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/settings.h"
#include "common/sky.h"
#include "common/snapshot.h"
#include "common/strings.h"
#include "common/system.h"
#include "common/text.h"
#include "common/track.h"
#include "common/trackmenuitem.h"
#include "common/trackselect.h"
#include "common/trackskidmarks.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/vehiclecamera.h"
#include "common/vehiclecount.h"
#include "common/vehicleeffects.h"
#include "common/vehiclemodel.h"
#include "common/vehiclerender.h"
#include "common/weather.h"
#include "frontend.h"
#include "gpu.h"
#include "menutext.h"
#include "racedemo.h"
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
// perspective whose field across the view is a field of view on a 4:3 screen, to a far plane.
Matrix4 CameraMatrix(const CameraFrame &camera, float fov, float farPlane, float aspect) {
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
    float fovy = fov * ASPECT_4_3 / aspect;
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
                       farPlane / (CAMERA_NEAR - farPlane),
                       -1,
                       0,
                       0,
                       CAMERA_NEAR * farPlane / (CAMERA_NEAR - farPlane),
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

// The matrix from a canvas, from its top left with y down, onto the whole frame.
Matrix4 CanvasProjection(int width, int height) {
    return {2.0f / static_cast<float>(width),
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
            1};
}

HudCanvas MakeHudCanvas(const GpuWindow &window) {
    float scale = window.DisplayScale();
    int width = std::max(static_cast<int>(static_cast<float>(window.FrameWidth()) / scale), 1);
    int height = std::max(static_cast<int>(static_cast<float>(window.FrameHeight()) / scale), 1);
    return {width, height, CanvasProjection(width, height)};
}

// The renderer and the fonts DrawText() draws the HUD with while it hands its text to the SDL game.
struct HudTextTarget {
    SceneRenderer *renderer = nullptr;
    const MenuText *text = nullptr;
};

HudTextTarget g_hudTextTarget;

// RenderTextToDisplayList() of the text of the HUD: the size, the alignment, the style, and the
// custom colours the race set, on the canvas the HUD is laid out on. The custom colours are never
// turned off once set.
void DrawHudText(const char *text, s32 x, s32 y) {
    TextStyle style;
    style.size = g_nFontSize;
    style.align = g_nTextAlignment == TEXT_ALIGN_RIGHT  ? TextStyle::Align::Right :
                  g_nTextAlignment == TEXT_ALIGN_CENTER ? TextStyle::Align::Center :
                                                          TextStyle::Align::Left;
    style.shadow = g_bTextShadowEnabled != 0;
    style.bold = g_bTextStyleBold != 0;
    style.doubled = false;
    if (g_bTextCustomColorsEnabled) {
        style.colors = std::array<int, 6>{g_dwTextCustomColorR,
                                          g_dwTextCustomColorG,
                                          g_dwTextCustomColorB,
                                          g_dwTextCustomColor2R,
                                          g_dwTextCustomColor2G,
                                          g_dwTextCustomColor2B};
    }
    g_hudTextTarget.text->Draw(*g_hudTextTarget.renderer,
                               text,
                               x,
                               y,
                               style,
                               CanvasProjection(GetHudCanvasWidth(), GetHudCanvasHeight()));
}

// The commands the HUD writes for the dashboard and its scissors go to a scratch list.
constexpr std::size_t HUD_SCRATCH_COMMANDS = 4096;

// The number of views of the race, one for each local player.
int ViewCount() {
    return g_dwNumViewports >= 2 ? 2 : 1;
}

// The views of the race on the canvas of the HUD: the whole canvas for a single view, and the
// views of a split screen scaled from the screen of the race, as BeginRaceHudCanvas() scales them.
std::array<ViewportRect, 2> HudViews(const HudCanvas &hud) {
    std::array<ViewportRect, 2> views{g_aViewportRects[0], g_aViewportRects[1]};
    int screenWidth = std::max<int>(g_dwScreenWidth, 1);
    int screenHeight = std::max<int>(g_dwScreenHeight, 1);
    for (std::size_t index = 0; index < views.size(); ++index) {
        ViewportRect &view = views[index];
        const ViewportRect &screen = g_aViewportRects[index];
        if (ViewCount() == 1) {
            view.x = 0;
            view.y = 0;
            view.width = hud.width;
            view.height = hud.height;
            continue;
        }
        view.x = screen.x * hud.width / screenWidth;
        view.y = screen.y * hud.height / screenHeight;
        view.width = (screen.x + screen.width) * hud.width / screenWidth - view.x;
        view.height = (screen.y + screen.height) * hud.height / screenHeight - view.y;
    }
    return views;
}

// DrawViewOverlay() of each view and DrawPauseMenu(), from the routines of the race, their text
// drawn by DrawHudText() on the canvas of the HUD as the patched PC release lays its HUD out on its
// canvas.
// ponytail: the gauges of the dashboard go to the scratch list and are not drawn yet.
void DrawRaceHud(SceneRenderer &renderer, const MenuText &text, const HudCanvas &hud) {
    static std::array<GbiCommand, HUD_SCRATCH_COMMANDS> scratch;
    GbiCommand *list = gDisplayListPtr;
    gDisplayListPtr = scratch.data();
    int framebufferWidth = g_dwConfigFramebufferWidth;
    int framebufferHeight = g_dwConfigFramebufferHeight;
    g_dwConfigFramebufferWidth = hud.width;
    g_dwConfigFramebufferHeight = hud.height;
    g_hudTextTarget = {&renderer, &text};
    g_pfnTextSink = DrawHudText;

    std::array<ViewportRect, 2> views = HudViews(hud);
    for (int index = 0; index < ViewCount(); ++index) {
        g_dwCurrentSplitScreenIndex = index;
        g_pActiveCameraVehicle = &g_aVehicles[views[static_cast<std::size_t>(index)].vehicleIndex];
        g_pActiveCamera = g_pActiveCameraVehicle->GetActiveCamera();
        const auto &cameras = g_pActiveCameraVehicle->GetCameras();
        if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
            SetTextStyleNormal();
            SetTextAlignCenter();
            SetTextSize(40);
            DrawText(GetStringResource(g_aVehicles[0].GetProfile()->season ? 240 : 241),
                     hud.width / 2,
                     hud.height - 30);
        } else if (g_bShowHudHelpText && g_dwGameMode != GAME_MODE_DEMO) {
            if (gInstantReplayActive) {
                SetTextStyleNormal();
                SetTextAlignLeft();
                SetTextSize(15);
                DrawText(GetStringResource(242), 28, 32);
                SetTextAlignRight();
                DrawText(GetStringResource(244), hud.width - 28, hud.height - 24);
            } else if (g_pActiveCamera == &cameras[VEHICLE_CAMERA_TRACKSIDE]) {
                if (g_pActiveCameraVehicle->GetRacer()->GetStatus() & RACER_FINISHED) {
                    DrawHudMessage(views.data());
                }
            } else {
                DrawSpeedAndHudOverlay(views.data(), g_aVehicles);
            }
        }
    }
    if (g_dwRacePauseState) {
        DrawPauseMenu(views.data(), g_aRacerSlots, g_aVehicles);
    }

    g_pfnTextSink = nullptr;
    g_dwConfigFramebufferWidth = framebufferWidth;
    g_dwConfigFramebufferHeight = framebufferHeight;
    gDisplayListPtr = list;
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

// The parts of the car of a vehicle.
struct CarModels {
    // The textured parts of the car, and the painted body and windows drawn after the highlight.
    std::unique_ptr<RenderModel> car;
    std::unique_ptr<RenderModel> paint;
    std::unique_ptr<RenderModel> underside;
    // The parts of the car the specular highlight shines on.
    std::unique_ptr<RenderModel> highlighted;
    std::unique_ptr<RenderModel> wheel;
    std::unique_ptr<RenderModel> snowWheel;
};

// A car in a colour: the car, an index of g_apszCarModelPrefixes, and its red, green, and blue.
using CarKey = std::array<int, 4>;

struct RaceView::State {
    std::filesystem::path defaults;
    std::unique_ptr<RenderModel> track;
    // ponytail: the sky loads the images of the track a second time; a model with its nodes drawn
    // by name would share them.
    std::unique_ptr<RenderModel> sky;
    // The cars in the colours the vehicles of the race have had. The computer's cars of a
    // championship take the colour of each racer they drive for, and a car in a colour not seen
    // before loads while the race runs.
    std::map<CarKey, CarModels> cars;
    std::unique_ptr<RenderModel> startLights;
    std::unique_ptr<MenuText> text;
    // The images of the day and the night sky the paint and the windows of the car reflect.
    std::unique_ptr<RenderTexture> daySky;
    std::unique_ptr<RenderTexture> nightSky;
    // The shadow of the car, the 8-bit intensities of the engine, the alpha the shade.
    std::unique_ptr<RenderTexture> shadowTexture;
    // LoadParticleTexture(): the 64 by 64 intensities of a puff of smoke.
    std::unique_ptr<RenderTexture> particleTexture;
    // LoadTireTrailTexture(): the 32 by 64 intensities of 4 bits of the spray of the tyres.
    std::unique_ptr<RenderTexture> sprayTexture;
    // CreateFallbackEnvironmentTexture() and LoadVehicleEnvMapTexture(): the 64 by 64 intensities
    // of 4 bits of the highlight by day and at night.
    std::unique_ptr<RenderTexture> dayHighlight;
    std::unique_ptr<RenderTexture> nightHighlight;
    // The first texture of the track, the texture entry 0 the skid marks are drawn with.
    std::unique_ptr<RenderTexture> skidTexture;
};

namespace {

// The texels of an image of intensities of 4 bits, two to a byte, the intensity in every channel.
std::vector<std::uint8_t> Intensities4(std::span<const unsigned char> pairs) {
    std::vector<std::uint8_t> texels;
    for (unsigned char pair : pairs) {
        for (int nibble : {pair >> 4, pair & 0xf}) {
            auto level = static_cast<std::uint8_t>(nibble * 17);
            texels.insert(texels.end(), {level, level, level, level});
        }
    }
    return texels;
}

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
// camera, with the window of the tile turning with the heading of the camera of the car of the
// view.
Reflection SkyReflection(const RenderTexture *daySky,
                         const RenderTexture *nightSky,
                         const CameraFrame &camera,
                         const Vehicle &car,
                         const Vehicle &cameraCar) {
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
    long double heading = cameraCar.GetCameraHeading();
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

// The environment colour of the shadows and the skid marks of a car, the fog by the fog of the car.
std::array<float, 4> ShadowColor(const Vehicle &car) {
    float fog = car.GetFogFactor();
    return {static_cast<float>(g_bFogR) / 255.0f * fog,
            static_cast<float>(g_bFogG) / 255.0f * fog,
            static_cast<float>(g_bFogB) / 255.0f * fog,
            1.0f};
}

// DrawSkidTrails(): each quad between two segments of a skid mark in the environment colour with
// the alpha of the texture of entry 0, the first texture of the track, as the PC release never
// sets the texture of the skid marks; 1024 of the 1/32 texels across and along a quad.
constexpr std::array<std::uint32_t, 2> SKID_COMBINE = CombineLerp(ZERO_AB,
                                                                  ZERO_AB,
                                                                  ZERO_D,
                                                                  ENVIRONMENT,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  TEXEL0,
                                                                  ZERO_AB,
                                                                  ZERO_AB,
                                                                  ZERO_D,
                                                                  ENVIRONMENT,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  TEXEL0);

struct SkidQuads {
    std::vector<FlatVertex> corners;
    float u = 0.0f;
    float v = 0.0f;
};

void DrawSkidMarks(SceneRenderer &renderer,
                   const RenderTexture &texture,
                   std::array<float, 4> color,
                   const Matrix4 &viewProjection) {
    SkidQuads quads;
    quads.u = 1024.0f / (32.0f * static_cast<float>(texture.Width()));
    quads.v = 1024.0f / (32.0f * static_cast<float>(texture.Height()));
    VisitSkidMarkQuads(
        [](const Vec3f *oldRight,
           const Vec3f *newRight,
           const Vec3f *oldLeft,
           const Vec3f *newLeft,
           void *context) {
            auto &skid = *static_cast<SkidQuads *>(context);
            auto corner = [&](const Vec3f *p, float u, float v) {
                skid.corners.push_back({p->x, p->y, u, v, {255, 255, 255, 255}, p->z});
            };
            corner(oldRight, 0.0f, 0.0f);
            corner(newRight, skid.u, 0.0f);
            corner(newLeft, skid.u, skid.v);
            corner(oldLeft, 0.0f, skid.v);
        },
        &quads);
    if (quads.corners.empty()) {
        return;
    }
    FlatMaterial material;
    material.texel0 = &texture;
    material.combine = SKID_COMBINE;
    material.env = color;
    renderer.DrawQuads(quads.corners, material, viewProjection, true);
}

// The shadow of a car as RenderTrackInstance() lays it on the instances near the car, or none
// where the race does not draw it: with the shadows off, from the mirror, or from the cockpit or
// the bumper of the car.
std::optional<CarShadow> ShadowOf(const RenderTexture *texture, const Vehicle &car) {
    const CameraFrame *active = car.GetActiveCamera();
    const auto &cameras = car.GetCameras();
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
    shadow.color = ShadowColor(car);
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

// The combiner of the spray of the tyres: the shade, its alpha the texel by the shade.
constexpr std::array<std::uint32_t, 2> SPRAY_COMBINE = PARTICLE_COMBINE;

// The width and the height of the texture of the trails of the tyres, in the 1/32 texels of the
// coordinates of a vertex of the PC release.
constexpr float SPRAY_TEXTURE_S = 32.0f * 32.0f;
constexpr float SPRAY_TEXTURE_T = 32.0f * 64.0f;

// The palettes of the lights of a car of the cartridge, as the N64 release of
// RenderVehicleModel() changes them: the headlights and the rear lights lit at night and in the
// rain, the brake lights while a brake is held, and the reversing lights while the throttle is
// below 0. A variant is the sum of 1 when lit, 2 when braking, and 4 when reversing.
constexpr int CAR_LIGHT_VARIANTS = 8;
constexpr int CAR_LIT = 1;
constexpr int CAR_BRAKING = 2;
constexpr int CAR_REVERSING = 4;
constexpr int HEADLIGHT_FIRST_ENTRY = 12;
constexpr int HEADLIGHT_ENTRIES = 4;
constexpr std::uint32_t BRAKE_BUTTONS = INPUT_BRAKE | INPUT_EBRAKE;

bool CarLightPalette(SDL_Palette *palette, int material, int variant) {
    if (palette == nullptr || palette->ncolors < 16) {
        return false;
    }
    std::array<SDL_Color, 16> colors{};
    std::copy_n(palette->colors, colors.size(), colors.begin());
    auto set = [&](int entry, std::uint32_t value) {
        colors[static_cast<std::size_t>(entry)] = {
            static_cast<Uint8>((value >> 11 & 0x1f) * 255 / 31),
            static_cast<Uint8>((value >> 6 & 0x1f) * 255 / 31),
            static_cast<Uint8>((value >> 1 & 0x1f) * 255 / 31),
            static_cast<Uint8>((value & 1) != 0 ? 255 : 0)};
    };
    bool lit = (variant & CAR_LIT) != 0;
    if (material == VEHICLE_MATERIAL_HEADLIGHTS) {
        for (int entry = HEADLIGHT_FIRST_ENTRY; entry < HEADLIGHT_FIRST_ENTRY + HEADLIGHT_ENTRIES;
             ++entry) {
            colors[static_cast<std::size_t>(entry)].a = lit ? 0 : 255;
        }
    } else if (material == VEHICLE_MATERIAL_REAR_LIGHTS) {
        std::uint32_t tail = lit ? 0x7000 : 0x9001;
        std::uint32_t lamp = lit ? 0x9082 : 0xA001;
        set(15, tail);
        set(10, lamp);
        if ((variant & CAR_BRAKING) != 0) {
            set(14, 0xC000);
            set(13, 0xC000);
            set(9, 0xF904);
            set(8, 0xF904);
        } else {
            set(14, 0x9001);
            set(9, 0xA001);
            set(13, tail);
            set(8, lamp);
        }
        set(12, 0x7981);
        set(7, 0x9241);
        bool reversing = (variant & CAR_REVERSING) != 0;
        set(11, reversing ? 0xB5AC : 0xAD6B);
        set(6, reversing ? 0xFFFE : 0xC631);
    } else {
        return false;
    }
    return SDL_SetPaletteColors(palette, colors.data(), 0, static_cast<int>(colors.size()));
}

// The variant of the lights of a car this frame.
int CarLightVariant(const Vehicle &car) {
    int variant = g_dwNightMode || g_dwRainMode ? CAR_LIT : 0;
    if (car.GetInput() != nullptr && (car.GetInput()->buttons & BRAKE_BUTTONS) != 0) {
        variant |= CAR_BRAKING;
    }
    if (car.GetThrottle() < 0.0f) {
        variant |= CAR_REVERSING;
    }
    return variant;
}

// Whether RenderVehicleModel() finds a car in a tunnel, by the first instance under it.
bool InTunnel(const Vehicle &car) {
    return car.GetGroundInstanceCount() != 0 &&
           (g_LoadedTrack.instances[car.GetGroundInstances()[0]].flags & TRACK_INSTANCE_SHELTER) !=
               0;
}

// The lights RenderVehicleModel() gives a car. The sun dims with the distance from the camera at
// night and is gone in a tunnel, where the ambient light is four fifths. The direction of
// g_DefaultLights, or at night and in the rain the reverse of the heading of the car of the camera
// at 120 a unit, lies in the space of the track. The forced matrix of the car moves its vertices
// only, and the lights turn through the matrix of the car.
Lighting CarLighting(const Vehicle &car, const Vehicle &cameraCar, const CameraFrame &camera) {
    const CameraFrame &frame = car.GetFrame();
    bool tunnel = InTunnel(car);
    float divisor = 1.0f;
    if (g_dwNightMode) {
        float distance = std::hypot(frame.position.x - camera.position.x,
                                    frame.position.y - camera.position.y,
                                    frame.position.z - camera.position.z);
        divisor = std::max(distance * 0.1f, 1.0f);
    }
    Lighting lighting;
    std::array<std::uint8_t, 3> sun{g_bDirLightR, g_bDirLightG, g_bDirLightB};
    std::array<std::uint8_t, 3> ambient{g_bAmbientLightR, g_bAmbientLightG, g_bAmbientLightB};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!g_dwNightMode && tunnel) {
            sun[i] = 0;
            ambient[i] = static_cast<std::uint8_t>(ambient[i] * 4 / 5);
        }
        lighting.directional[i] =
            static_cast<float>(static_cast<std::uint8_t>(sun[i] / divisor)) / 255.0f;
        lighting.ambient[i] = static_cast<float>(ambient[i]) / 255.0f;
    }
    std::array<float, 3> light{84.0f, 84.0f, 84.0f};
    if (g_dwNightMode || g_dwRainMode) {
        const Vec3f &forward = cameraCar.GetFrame().forward;
        light = {static_cast<float>(static_cast<s8>(forward.x * -120.0f)),
                 static_cast<float>(static_cast<s8>(forward.y * -120.0f)),
                 static_cast<float>(static_cast<s8>(forward.z * -120.0f))};
    }
    float length = std::hypot(light[0], light[1], light[2]);
    if (length > 0.0f) {
        lighting.direction = {light[0] / length, light[1] / length, light[2] / length};
    }
    return lighting;
}

// The colour of the highlight, PRIMITIVE with the alpha of TEXEL0, and its primitive colour.
constexpr std::array<std::uint32_t, 2> HIGHLIGHT_COMBINE = CombineLerp(ZERO_AB,
                                                                       ZERO_AB,
                                                                       ZERO_D,
                                                                       PRIMITIVE,
                                                                       ZERO_ALPHA,
                                                                       ZERO_ALPHA,
                                                                       ZERO_ALPHA,
                                                                       TEXEL0,
                                                                       ZERO_AB,
                                                                       ZERO_AB,
                                                                       ZERO_D,
                                                                       PRIMITIVE,
                                                                       ZERO_ALPHA,
                                                                       ZERO_ALPHA,
                                                                       ZERO_ALPHA,
                                                                       TEXEL0);
constexpr std::array<float, 4> HIGHLIGHT_COLOR{1.0f, 1.0f, 0xCC / 255.0f, 1.0f};

// The highlight RenderVehicleModel() draws when the specular light is chosen, but in the snow, in
// the rain, in a tunnel by day, and on the car of the camera at night. The light comes from the sun
// by day and from behind the camera at night, and the highlight sits halfway between it and the
// camera. The PC release of guLookAtHiliteF() places the tile by the angles of that direction
// around and above the look from the camera at the car, 256 texels of 64ths to half a turn, and
// the coordinates follow the right and the up axes of the look rounded to bytes.
std::optional<Highlight> CarHighlight(const Vehicle &car,
                                      const Vehicle &cameraCar,
                                      const CameraFrame &camera,
                                      const RenderTexture *day,
                                      const RenderTexture *night) {
    if (!g_GlobalRallyConfig.bSpecularLightingSelection || g_dwSnowMode || g_dwRainMode ||
        (InTunnel(car) && !g_dwNightMode) || (g_dwNightMode && &car == &cameraCar) ||
        car.GetCustomCarModel()) {
        return std::nullopt;
    }
    const CameraFrame &frame = car.GetFrame();
    auto normalize = [](Vec3f v) {
        float length = std::hypot(v.x, v.y, v.z);
        return length == 0.0f ? v : Vec3f{v.x / length, v.y / length, v.z / length};
    };
    Vec3f light{g_afLightDirection[0], g_afLightDirection[1], g_afLightDirection[2]};
    if (g_dwNightMode) {
        const auto &cameras = cameraCar.GetCameras();
        const Vec3f &forward = &camera == &cameras[VEHICLE_CAMERA_TRACKSIDE] ?
                                   camera.forward :
                                   cameraCar.GetFrame().forward;
        light = {-forward.x, -forward.y, -forward.z};
    }
    light = normalize(light);
    Vec3f toCamera{camera.position.x - frame.position.x,
                   camera.position.y - frame.position.y,
                   camera.position.z - frame.position.z};
    toCamera = std::hypot(toCamera.x, toCamera.y, toCamera.z) == 0.0f ?
                   Vec3f{-camera.forward.x, -camera.forward.y, -camera.forward.z} :
                   normalize(toCamera);
    Vec3f halfway{(toCamera.x + light.x) * 0.5f,
                  (toCamera.y + light.y) * 0.5f,
                  (toCamera.z + light.z) * 0.5f};
    halfway = std::hypot(halfway.x, halfway.y, halfway.z) == 0.0f ? camera.up : normalize(halfway);
    float nudgeX = 0.0f;
    float nudgeZ = 0.0f;
    if (camera.position.x == frame.position.x && camera.position.y == frame.position.y) {
        if (camera.position.z == frame.position.z) {
            nudgeZ = 1.0f;
        } else {
            nudgeX = 0.1f;
        }
    }
    // The right, the up, and the look axes of guLookAtF(), the look from the car back to the eye.
    std::array<float, 3> back = Normalized({camera.position.x - frame.position.x - nudgeX,
                                            camera.position.y - frame.position.y,
                                            camera.position.z - frame.position.z - nudgeZ});
    std::array<float, 3> right = Normalized(Cross({0.0f, 0.0f, 1.0f}, back));
    std::array<std::array<float, 3>, 3> axes{right, Normalized(Cross(back, right)), back};
    auto along = [&](int column) {
        const auto &a = axes[static_cast<std::size_t>(column)];
        return static_cast<double>(a[0]) * halfway.x + static_cast<double>(a[1]) * halfway.y +
               static_cast<double>(a[2]) * halfway.z;
    };
    double angle = std::atan2(along(0), along(2));
    double rise = std::asin(std::clamp(along(1), -1.0, 1.0));
    auto tile = [](double value) {
        return static_cast<std::int32_t>(
            256u -
            static_cast<std::uint32_t>(static_cast<std::int64_t>(value * -81.48734781601175)));
    };
    auto axis = [&](int column) {
        const auto &a = axes[static_cast<std::size_t>(column)];
        return std::array<float, 3>{static_cast<float>(ConvertDoubleToSignedByte(a[0])) / 128.0f,
                                    static_cast<float>(ConvertDoubleToSignedByte(a[1])) / 128.0f,
                                    static_cast<float>(ConvertDoubleToSignedByte(a[2])) / 128.0f};
    };
    Highlight highlight;
    highlight.texture = g_dwNightMode ? night : day;
    highlight.across = axis(0);
    highlight.up = axis(1);
    highlight.s = static_cast<float>(128 - tile(angle)) / 256.0f;
    highlight.t = static_cast<float>(128 - tile(rise)) / 256.0f;
    highlight.combine = HIGHLIGHT_COMBINE;
    highlight.primitive = HIGHLIGHT_COLOR;
    return highlight;
}

// The instances RenderTrack() hides: the ones of a dark or wet track on a clear day, the ones of
// arcade races in every other mode but the race after the first season, and the ones of a clear sky
// in weather on every track but the coastline.
std::uint32_t HiddenInstanceFlags(const Vehicle &first) {
    std::uint32_t flags = 0;
    if (!g_dwNightMode && !g_dwRainMode) {
        flags = TRACK_INSTANCE_DARK_OR_WET;
    }
    if (g_dwGameMode != GAME_MODE_ARCADE && g_dwGameMode != GAME_MODE_NETWORK &&
        (g_dwGameMode != GAME_MODE_AFTER_SEASON || first.GetProfile()->season != 0)) {
        flags |= TRACK_INSTANCE_ARCADE;
    }
    if (g_dwWeatherActive && (g_dwSnowMode || g_dwRainMode || g_dwNightMode) &&
        g_dwSelectedTrackId != TRACK_COASTLINE && g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE) {
        flags |= TRACK_INSTANCE_CLEAR_SKY;
    }
    return flags;
}

// DrawTireSpray(): behind each wheel, the quads between the segments of its spray but the newest,
// skipping a quad whose two edges have stopped, in the place of the spray matrix of the car.
// The spray is not drawn in the snow but on the coast.
void DrawTireSpray(SceneRenderer &renderer,
                   const RenderTexture &texture,
                   const Vehicle &car,
                   const Matrix4 &viewProjection) {
    if (g_dwSnowMode && g_dwSelectedTrackId != TRACK_COASTLINE &&
        g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE) {
        return;
    }
    const auto &vertices = car.GetSprayVertices();
    const auto &velocities = car.GetSprayVelocities();
    std::vector<FlatVertex> quads;
    auto corner = [&](const ConvertedModelVertex &vertex) {
        // An unlit vertex of the PC release takes its colour from its normal, from 0 to 255.
        quads.push_back({vertex.position.x,
                         vertex.position.y,
                         vertex.texture.x / SPRAY_TEXTURE_S,
                         vertex.texture.y / SPRAY_TEXTURE_T,
                         {static_cast<std::uint8_t>(std::clamp(vertex.color.x, 0.0f, 255.0f)),
                          static_cast<std::uint8_t>(std::clamp(vertex.color.y, 0.0f, 255.0f)),
                          static_cast<std::uint8_t>(std::clamp(vertex.color.z, 0.0f, 255.0f)),
                          0xFF},
                         vertex.position.z});
    };
    auto stopped = [](const s16(&velocity)[3]) {
        return velocity[0] == 0 && velocity[1] == 0 && velocity[2] == 0;
    };
    for (int wheel = 0; wheel < VEHICLE_WHEELS; ++wheel) {
        for (int segment = 1; segment + 1 < SPRAY_SEGMENTS; ++segment) {
            if (stopped(velocities[wheel][segment][1]) &&
                stopped(velocities[wheel][segment + 1][1])) {
                continue;
            }
            // The two triangles of gSP2Triangles(), the first and the second edge across them.
            const auto &near = vertices[wheel][segment];
            const auto &far = vertices[wheel][segment + 1];
            corner(far[0]);
            corner(far[1]);
            corner(near[1]);
            corner(near[0]);
        }
    }
    if (quads.empty()) {
        return;
    }
    Matrix4 place{};
    std::memcpy(place.data(), car.GetSprayMatrix(), sizeof(place));
    FlatMaterial material;
    material.texel0 = &texture;
    material.combine = SPRAY_COMBINE;
    renderer.DrawQuads(quads, material, MultiplyMatrices(viewProjection, place), true);
}

} // namespace

RaceView::RaceView(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

RaceView::~RaceView() = default;

namespace {

// The car of the nearest level of detail, the one level a single view draws, in its parts: the
// body, the underside, and a wheel of each kind, which the race places one by one. The geometry of
// the car is that of the PC release, which its race moves, and its textures are those of the
// cartridge, painted as SetMaterialColor() paints them. None of the parts load when the car is
// missing.
// ponytail: each part, and each colour of a car, loads the images of the car again; a model drawn
// by parts and painted as it is drawn would share them.
CarModels
LoadCarModels(SceneRenderer &renderer, const std::filesystem::path &defaults, const CarKey &key) {
    std::string prefix = g_apszCarModelPrefixes[key[0]];
    auto carFile = defaults / "cars" / (prefix + ".gltf");
    CarModels models;
    ModelOptions options;
    options.paint = std::array<std::uint8_t, 3>{static_cast<std::uint8_t>(key[1]),
                                                static_cast<std::uint8_t>(key[2]),
                                                static_cast<std::uint8_t>(key[3])};
    options.images = CartridgeCar(defaults, prefix);
    options.parts = [](const std::string &n) {
        return n == "lod0-nearDetail" || n == "lod0-textured";
    };
    options.variants = CAR_LIGHT_VARIANTS;
    options.paletteVariant = CarLightPalette;
    models.car = RenderModel::Load(renderer, carFile, options);
    options.variants = 0;
    options.paletteVariant = nullptr;
    options.parts = [](const std::string &n) { return n == "lod0-paint"; };
    models.paint = RenderModel::Load(renderer, carFile, options);
    options.parts = [](const std::string &n) { return n == "lod0-underside"; };
    models.underside = RenderModel::Load(renderer, carFile, options);
    // The highlighted parts are a node outside the scene, drawn without the turn of its root.
    options.parts = nullptr;
    options.node = "lod0-highlighted";
    models.highlighted = RenderModel::Load(renderer, carFile, options);
    options.node = "lod0-wheel0";
    models.wheel = RenderModel::Load(renderer, carFile, options);
    options.node = "lod0-wheelSnow0";
    models.snowWheel = RenderModel::Load(renderer, carFile, options);
    if (!models.car || !models.paint || !models.wheel) {
        return {};
    }
    return models;
}

} // namespace

std::unique_ptr<RaceView> RaceView::Load(SceneRenderer &renderer,
                                         const std::filesystem::path &defaults,
                                         int track,
                                         std::span<const RaceCar> cars) {
    auto state = std::make_unique<State>();
    std::filesystem::path trackFile = defaults / "tracks" / g_apszTrackFilenames[track];
    trackFile.replace_extension(".gltf");
    state->track = RenderModel::Load(renderer, trackFile);
    std::filesystem::path skidImage = trackFile;
    skidImage.replace_filename(trackFile.stem().string() + "-0.png");
    state->skidTexture = RenderTexture::Load(renderer, skidImage, true, true);
    ModelOptions sky;
    sky.node = "sky";
    state->sky = RenderModel::Load(renderer, trackFile, sky);
    state->defaults = defaults;
    for (const RaceCar &vehicle : cars) {
        CarKey key{vehicle.car, vehicle.paint[0], vehicle.paint[1], vehicle.paint[2]};
        CarModels models = LoadCarModels(renderer, defaults, key);
        if (!models.car) {
            return nullptr;
        }
        state->cars.try_emplace(key, std::move(models));
    }
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
    state->sprayTexture = RenderTexture::FromTexels(
        renderer, 32, 64, Intensities4(g_abHudTexture32x64Data), true, true);
    state->dayHighlight = RenderTexture::FromTexels(
        renderer, 64, 64, Intensities4(g_abFallbackEnvTextureData), true, true);
    state->nightHighlight = RenderTexture::FromTexels(
        renderer, 64, 64, Intensities4(g_abHudTexture64BData), true, true);
    // The race of the PC release draws its text with the fonts of the cartridge.
    state->text = MenuText::Load(renderer, defaults / "n64" / "usa" / "fonts");
    // The start lights, the model of BossRally.pod.
    state->startLights = RenderModel::Load(renderer, defaults / "BossRally" / "modellights.gltf");
    if (!state->track || state->cars.empty() || !state->text || !state->startLights) {
        return nullptr;
    }
    return std::unique_ptr<RaceView>(new RaceView(std::move(state)));
}

namespace {

// RenderRaceView() looks from the car of a view before SetupFog() writes the fog of the view and
// its two commands, which go to a scratch list.
void LookFromView(int index) {
    std::array<GbiCommand, 2> commands{};
    GbiCommand *list = gDisplayListPtr;
    gDisplayListPtr = commands.data();
    g_dwCurrentSplitScreenIndex = index;
    g_pActiveCameraVehicle = &g_aVehicles[g_aViewportRects[index].vehicleIndex];
    g_pActiveCamera = g_pActiveCameraVehicle->GetActiveCamera();
    SetupFog();
    gDisplayListPtr = list;
}

// The rectangle of the frame in pixels a view of a split screen fills, from its place on the screen
// of the race.
SDL_Rect FrameRect(const ViewportRect &view, const GpuWindow &window) {
    auto width = static_cast<int>(window.FrameWidth());
    auto height = static_cast<int>(window.FrameHeight());
    int screenWidth = std::max<int>(g_dwScreenWidth, 1);
    int screenHeight = std::max<int>(g_dwScreenHeight, 1);
    int left = view.x * width / screenWidth;
    int top = view.y * height / screenHeight;
    return {left,
            top,
            (view.x + view.width) * width / screenWidth - left,
            (view.y + view.height) * height / screenHeight - top};
}

// The flash of lightning RenderSkybox() mixes a quarter of into the fog.
constexpr std::array<int, 3> LIGHTNING_FLASH{0xF0, 0xF8, 0xFF};

// RenderSkybox() when the fog fills the sky: DrawFilledRect() of the view in the colour of the fog,
// a quarter of the way to the flash on the odd frames of lightning.
void FillSkyWithFog(SceneRenderer &renderer) {
    std::array<int, 3> fog{g_bFogR, g_bFogG, g_bFogB};
    std::array<std::uint8_t, 4> color{0, 0, 0, 0xFF};
    bool flash = g_dwLightningFlashCounter > 0 && (g_dwLightningFlashCounter & 1) != 0;
    for (std::size_t i = 0; i < fog.size(); ++i) {
        color[i] =
            static_cast<std::uint8_t>(flash ? (fog[i] * 3 + LIGHTNING_FLASH[i]) >> 2 : fog[i]);
    }
    std::array<FlatVertex, 4> corners{{{-1.0f, 1.0f, 0.0f, 0.0f, color, 0.0f},
                                       {1.0f, 1.0f, 0.0f, 0.0f, color, 0.0f},
                                       {1.0f, -1.0f, 0.0f, 0.0f, color, 0.0f},
                                       {-1.0f, -1.0f, 0.0f, 0.0f, color, 0.0f}}};
    renderer.DrawQuads(corners, FlatMaterial{}, IDENTITY_MATRIX);
}

} // namespace

std::array<float, 4> RaceView::ClearColor() const {
    // ClearScreen() blacks out the screen around the views of a split screen.
    if (ViewCount() > 1) {
        return {0.0f, 0.0f, 0.0f, 1.0f};
    }
    LookFromView(0);
    return {static_cast<float>(g_bFogR) / 255.0f,
            static_cast<float>(g_bFogG) / 255.0f,
            static_cast<float>(g_bFogB) / 255.0f,
            1.0f};
}

void RaceView::Draw(SceneRenderer &renderer, float share) const {
    State &s = *m_state;
    GpuWindow &window = renderer.Window();
    // The frame draws the snapshots of the race blended, or the race itself before it has two.
    FrameSnapshot *snapshot = InterpolateFrameSnapshots(share);
    // The race keeps the 640 by 480 screen of the PC release, and the HUD is drawn on the canvas of
    // the display scale from the same offsets from the edges of the screen.
    HudCanvas hud = MakeHudCanvas(window);
    renderer.SetMirrored(g_dwTrackMirrored != 0);
    // The animated textures of the track take the time of the race.
    renderer.SetTextureTime(static_cast<std::uint32_t>(g_dwElapsedTimeMs));
    // UpdateTrackSpecials() spins instances and flies the airplane in their matrices.
    renderer.SetInstanceMatrices([](int index) -> const float * {
        return index < g_LoadedTrack.instanceCount ? &g_LoadedTrack.instances[index].matrix[0][0] :
                                                     nullptr;
    });
    // DrawTrackShadows() lays the skid marks of the cars of each drawn frame, and each view draws
    // them.
    if (snapshot != nullptr && s.skidTexture) {
        RecordFrameSkidMarks(snapshot->GetVehicles());
    }
    // The puffs of the snapshot the frame draws.
    if (snapshot != nullptr) {
        LoadParticleListHeads(&snapshot->GetParticles());
    }
    for (int index = 0; index < ViewCount(); ++index) {
        DrawView(renderer, snapshot, index);
    }
    renderer.SetViewport(std::nullopt);
    renderer.SetFog(std::nullopt);

    DrawRaceHud(renderer, *s.text, hud);

    // The help of the attract demo, from the corner of the canvas of the HUD, under the fade. The
    // console shows nothing over its demo.
    const char *help = GetStringResource(DEMO_HELP_STRING);
    if (g_bShowHudHelpText && g_dwGameMode == GAME_MODE_DEMO &&
        g_dwRaceSessionType == RACE_SESSION_ATTRACT && !AttractDemoFromConsole() &&
        help != nullptr) {
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

// RenderRaceView() of a view: the track, the cars, the puffs, and the start lights from the active
// camera of the car of the view.
void RaceView::DrawView(SceneRenderer &renderer, const FrameSnapshot *snapshot, int index) const {
    State &s = *m_state;
    GpuWindow &window = renderer.Window();
    int cameraIndex = g_aViewportRects[index].vehicleIndex;
    const Vehicle &vehicle =
        snapshot != nullptr ? snapshot->GetVehicle(cameraIndex) : g_aVehicles[cameraIndex];
    const Vehicle &first = snapshot != nullptr ? snapshot->GetVehicle(0) : g_aVehicles[0];
    const CameraFrame *camera = vehicle.GetActiveCamera();
    if (camera == nullptr) {
        return;
    }
    // A view of a split screen fills its part of the screen, and looks narrower and not as far.
    bool split = ViewCount() > 1;
    SDL_Rect frame{
        0, 0, static_cast<int>(window.FrameWidth()), static_cast<int>(window.FrameHeight())};
    if (split) {
        frame = FrameRect(g_aViewportRects[index], window);
        renderer.SetViewport(frame);
    }
    float aspect = static_cast<float>(frame.w) / static_cast<float>(std::max(frame.h, 1));
    Matrix4 viewProjection = CameraMatrix(*camera,
                                          split ? camera->fov * 0.7f : camera->fov,
                                          split ? CAMERA_FAR * 0.65f : CAMERA_FAR,
                                          aspect);
    // SetViewport() mirrors the view across on a mirrored track, where the steering turns the
    // other way, and the faces of the models then turn the other way.
    if (g_dwTrackMirrored) {
        constexpr Matrix4 ACROSS{-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        viewProjection = MultiplyMatrices(ACROSS, viewProjection);
    }
    LookFromView(index);
    if (split && IsSkyFogFilled() && !g_dwSkyboxDisabled) {
        FillSkyWithFog(renderer);
    }
    renderer.SetHiddenInstanceFlags(HiddenInstanceFlags(first));
    // UpdateD3DFogRenderState(): the fog of the weather, farther on a clear night.
    renderer.SetFog(std::nullopt);
    if (g_dwWeatherActive) {
        renderer.SetFog(Fog{{static_cast<float>(g_bFogR) / 255.0f,
                             static_cast<float>(g_bFogG) / 255.0f,
                             static_cast<float>(g_bFogB) / 255.0f},
                            !g_dwRainMode && !g_dwSnowMode && g_dwNightMode ? 16384.0f : 8192.0f});
    }
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
    std::optional<CarShadow> shadow = ShadowOf(s.shadowTexture.get(), vehicle);
    if (shadow) {
        renderer.DrawShadowedModel(
            *s.track, ConsoleSpace(), viewProjection, lighting, std::move(*shadow));
    } else {
        renderer.DrawModel(*s.track, ConsoleSpace(), viewProjection, lighting, std::nullopt, true);
    }
    if (snapshot != nullptr && s.skidTexture && g_GlobalRallyConfig.bSkidMarksSelection) {
        DrawSkidMarks(renderer, *s.skidTexture, ShadowColor(vehicle), viewProjection);
    }
    // RenderVehicleModel() does not draw the car of the view from its own cockpit or its mirror.
    const auto &cameras = vehicle.GetCameras();
    bool inside =
        camera == &cameras[VEHICLE_CAMERA_COCKPIT] || camera == &cameras[VEHICLE_CAMERA_MIRROR];
    Lighting cameraCarLighting = CarLighting(vehicle, vehicle, *camera);
    // RenderRacerVehicles(): the car of each vehicle of the race.
    // A ghost, the car of a time attack recorded before or a car fading back in after it is put
    // back on the track, is drawn after the cars that are not, in the render mode of a ghost of
    // BRD3D. The opacity of a ghost goes to the alpha of the primitive colour, which that render
    // mode does not read.
    for (int index = 0; index < std::max(g_dwNumActiveVehicles, 1); ++index) {
        if (index == cameraIndex && inside && !g_dwVehicleCullingDisabled) {
            continue;
        }
        const Vehicle &car = snapshot != nullptr ? snapshot->GetVehicle(index) : g_aVehicles[index];
        const auto &color = car.GetColor();
        CarKey key{car.GetCarModel(), color[0], color[1], color[2]};
        auto found = s.cars.find(key);
        // A car that does not load is not tried again.
        if (found == s.cars.end()) {
            found = s.cars.try_emplace(key, LoadCarModels(renderer, s.defaults, key)).first;
        }
        const CarModels &models = found->second;
        if (!models.car) {
            continue;
        }
        bool ghost = car.GetGhostMode() == VEHICLE_GHOST;
        renderer.SetGhost(ghost);
        Lighting carLighting = CarLighting(car, vehicle, *camera);
        Matrix4 body = MultiplyMatrices(FrameMatrix(car.GetFrame()), ConsoleSpace());
        Reflection reflection =
            SkyReflection(s.daySky.get(), s.nightSky.get(), *camera, car, g_aVehicles[cameraIndex]);
        // The textured parts, the underside, the highlight, and the painted body in turn.
        models.car->SetVariant(CarLightVariant(car));
        renderer.DrawReflectingModel(*models.car, body, viewProjection, carLighting, reflection);
        // The underside shows only from below the car.
        const CameraFrame &frame = car.GetFrame();
        float below = frame.up.x * (frame.position.x - camera->position.x) +
                      frame.up.y * (frame.position.y - camera->position.y) +
                      frame.up.z * (frame.position.z - camera->position.z);
        if (models.underside && below > 0.0f) {
            renderer.DrawModel(
                *models.underside, body, viewProjection, carLighting, std::nullopt, true);
        }
        std::optional<Highlight> highlight =
            CarHighlight(car, vehicle, *camera, s.dayHighlight.get(), s.nightHighlight.get());
        // The highlight draws only where the depth is equal, and a ghost does not write the depth.
        if (highlight && models.highlighted && !ghost) {
            renderer.DrawHighlight(
                *models.highlighted, FrameMatrix(frame), viewProjection, *highlight);
        }
        renderer.DrawReflectingModel(*models.paint, body, viewProjection, carLighting, reflection);
        // RenderWheelModels(): each wheel at the matrix the race gives it, in snow the wheel of
        // the snow.
        const RenderModel &wheel =
            g_dwSnowMode && models.snowWheel ? *models.snowWheel : *models.wheel;
        for (const auto &rows : car.GetWheelMatrices()) {
            Matrix4 place{};
            std::memcpy(place.data(), rows, sizeof(place));
            renderer.DrawModel(wheel, place, viewProjection, carLighting, std::nullopt, true);
        }
    }
    renderer.SetGhost(false);
    if (snapshot != nullptr) {
        DrawParticles(renderer, *s.particleTexture, *s.shadowTexture, *camera, viewProjection);
        // A championship has more racers than vehicles, and only the vehicles have spray.
        for (int index = 0; index < g_dwNumActiveVehicles; ++index) {
            DrawTireSpray(renderer, *s.sprayTexture, snapshot->GetVehicle(index), viewProjection);
        }
    }
    // RenderStartLights() draws under the lights the cars left: g_DefaultLights in their colours.
    // ponytail: from the cockpit the PC release leaves the colours of the frame before.
    std::optional<Matrix4> lights = StartLightsMatrix();
    if (lights) {
        renderer.DrawModel(*s.startLights,
                           MultiplyMatrices(*lights, ConsoleSpace()),
                           viewProjection,
                           cameraCarLighting,
                           StartLightsPoses(*s.startLights),
                           true);
    }
}

} // namespace coastline
