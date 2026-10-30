#include "raceview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iterator>
#include <map>
#include <optional>
#include <span>
#include <string>

#include <SDL3/SDL.h>

#include "common/assetnames.h"
#include "common/camera.h"
#include "common/convertedvertex.h"
#include "common/displaylistptr.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/gbicommand.h"
#include "common/hud.h"
#include "common/hudassets.h"
#include "common/hudcanvas.h"
#include "common/input.h"
#include "common/instantreplay.h"
#include "common/model.h"
#include "common/packedvalue.h"
#include "common/paintshop.h"
#include "common/particlesnapshot.h"
#include "common/qualityoflife.h"
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
#include "common/trackrender.h"
#include "common/trackselect.h"
#include "common/trackskidmarks.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/vehiclecamera.h"
#include "common/vehiclecolor.h"
#include "common/vehiclecount.h"
#include "common/vehicleeffects.h"
#include "common/vehiclemodel.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"
#include "enginetextures.h"
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

// The widest field across and the highest vertical field of a view, 120 degrees.
constexpr float MAX_FIELD = 2.0943952f;

// The multiplier of the fields of view.
float g_fieldOfViewScale = 1.0f;

} // namespace

float RaceFieldOfView(float fovy, float aspect) {
    // A frame at least as wide as 4:3 keeps the vertical field and widens across, and a narrower
    // frame keeps the field across of 4:3 and grows the vertical field, as GetHorPlusFovY() of the
    // patched PC release gives them. Past the widest field across, the vertical field shrinks.
    fovy = GetHorPlusFovY(fovy * g_fieldOfViewScale, aspect, 1.0f);
    if (2.0f * std::atan(std::tan(fovy * 0.5f) * aspect) > MAX_FIELD) {
        fovy = 2.0f * std::atan(std::tan(MAX_FIELD * 0.5f) / aspect);
    }
    return std::min(fovy, MAX_FIELD);
}

void SetFieldOfViewScale(float scale) {
    g_fieldOfViewScale = scale > 0.0f ? scale : 1.0f;
}

namespace {

// SetupCameraMatrix() of the PC release: guLookAtF() from the camera along its forward axis, and a
// perspective of a vertical field, to a far plane.
Matrix4 CameraMatrix(const CameraFrame &camera, float fovy, float farPlane, float aspect) {
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

// The canvas the HUD of the race is laid out on, as GetHudCanvasSize() of the patched PC release
// sizes it: 480 high, or 640 wide on a frame narrower than 4:3, with the shape of the frame, from
// its top left with y down onto the whole frame.
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
    int width = 0;
    int height = 0;
    GetHudCanvasSize(static_cast<int>(window.FrameWidth()),
                     static_cast<int>(window.FrameHeight()),
                     &width,
                     &height);
    width = std::max(width, 1);
    height = std::max(height, 1);
    return {width, height, CanvasProjection(width, height)};
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

// The textures of the engine the HUD draws, by their entry, each with the version of its texels.
using HudTextures = std::map<unsigned, std::pair<unsigned, std::unique_ptr<RenderTexture>>>;

// The commands of the display list the HUD writes that RenderDashboardGauges() draws with.
constexpr unsigned GBI_OP_SET_TEXTURE_ENTRY = 0xdc;
constexpr unsigned GBI_OP_LOAD_TEXTURE_ENTRY = 0xdd;
constexpr unsigned GBI_OP_TEXTURE_RECTANGLE = 0xe3;

// The HUD being drawn: the renderer, the fonts DrawText() draws with while it hands its text to the
// SDL game, the textures and the canvas of the commands the HUD writes, the first command not drawn
// yet, and the texture entry and the vertices the commands drawn have loaded.
struct HudDrawing {
    SceneRenderer *renderer = nullptr;
    const MenuText *text = nullptr;
    HudTextures *textures = nullptr;
    const HudCanvas *hud = nullptr;
    const GbiCommand *drawn = nullptr;
    unsigned entry = 0;
    const ConvertedModelVertex *vertices = nullptr;
    unsigned firstVertex = 0;
    unsigned vertexCount = 0;
};

HudDrawing g_hudDrawing;

// The commands of RenderDashboardGauges() not drawn yet, up to the next one the HUD writes, drawn
// as BRD3D draws them on the canvas of the HUD: the face and the gear of the dashboard as
// rectangles of their textures, a texel of an alpha below a half drawing nothing, and the needle of
// the tachometer in the colour of its corners, its places counted up from the bottom of the canvas.
void DrawHudCommands(HudDrawing &drawing, const GbiCommand *end) {
    SceneRenderer &renderer = *drawing.renderer;
    HudTextures &textures = *drawing.textures;
    const HudCanvas &hud = *drawing.hud;
    std::span<const GbiCommand> commands{drawing.drawn, end};
    drawing.drawn = end;
    auto texture = [&](unsigned entry) -> const RenderTexture * {
        const EngineTexture *source = FindEngineTexture(entry);
        if (source == nullptr || source->rgba.empty()) {
            return nullptr;
        }
        auto &cached = textures[entry];
        if (!cached.second || cached.first != source->version) {
            cached = {source->version,
                      RenderTexture::FromTexels(
                          renderer, source->width, source->height, source->rgba, false, false)};
        }
        return cached.second.get();
    };
    unsigned &entry = drawing.entry;
    const ConvertedModelVertex *&vertices = drawing.vertices;
    unsigned &firstVertex = drawing.firstVertex;
    unsigned &vertexCount = drawing.vertexCount;
    for (const GbiCommand &command : commands) {
        unsigned opcode = command.w0 >> 24;
        if (opcode == GBI_OP_SET_TEXTURE_ENTRY) {
            entry = command.w0 & 0xffffff;
        } else if (opcode == GBI_OP_LOAD_TEXTURE_ENTRY) {
            const void *colors = command.pData;
            LoadEngineTexture(command.w0 & 0xffffff, static_cast<const unsigned char *>(colors));
        } else if (opcode == GBI_OP_TEXTURE_RECTANGLE) {
            const RenderTexture *image = texture(entry);
            if (image == nullptr) {
                continue;
            }
            auto left = static_cast<float>(command.w1 >> 12 & 0xfff);
            auto top = static_cast<float>(command.w1 & 0xfff);
            auto right = static_cast<float>(command.w0 >> 12 & 0xfff);
            auto bottom = static_cast<float>(command.w0 & 0xfff);
            // DrawTextureRectangle() of BRD3D gives the top corners the lower right T of the tile
            // and the bottom corners its upper left T, the centres of the last and the first rows.
            // The images of the dashboard are stored from the bottom row up.
            const EngineTexture *source = FindEngineTexture(entry);
            float halfU = 0.5f / static_cast<float>(source->width);
            float halfV = 0.5f / static_cast<float>(source->height);
            std::array<FlatVertex, 4> corners{{{left, top, halfU, 1.0f - halfV},
                                               {right, top, 1.0f - halfU, 1.0f - halfV},
                                               {right, bottom, 1.0f - halfU, halfV},
                                               {left, bottom, halfU, halfV}}};
            FlatMaterial material;
            material.texel0 = image;
            material.alphaCut = 0.5f;
            renderer.DrawQuads(corners, material, hud.projection);
        } else if (opcode == GBI_OP_VERTEX) {
            vertices = command.vertexData.pConverted;
            firstVertex = command.w0 >> 16 & 0xff;
            vertexCount = command.w0 >> 10 & 0x3f;
        } else if (opcode == GBI_OP_TRI2 && vertices != nullptr) {
            std::vector<FlatVertex> corners;
            for (unsigned word : {command.w0 & 0xffffff, command.w1 & 0xffffff}) {
                std::array<unsigned, 3> triangle{word >> 16 & 0xff, word >> 8 & 0xff, word & 0xff};
                // A triangle is drawn as a quad whose last two corners meet.
                for (unsigned index : {triangle[0], triangle[1], triangle[2], triangle[2]}) {
                    if (index < firstVertex || index >= firstVertex + vertexCount) {
                        break;
                    }
                    const ConvertedModelVertex &vertex = vertices[index - firstVertex];
                    corners.push_back(
                        {vertex.position.x,
                         static_cast<float>(hud.height) - vertex.position.y,
                         0.0f,
                         0.0f,
                         {static_cast<std::uint8_t>(std::clamp(vertex.color.x, 0.0f, 255.0f)),
                          static_cast<std::uint8_t>(std::clamp(vertex.color.y, 0.0f, 255.0f)),
                          static_cast<std::uint8_t>(std::clamp(vertex.color.z, 0.0f, 255.0f)),
                          0xFF}});
                }
            }
            if (corners.size() % 4 == 0 && !corners.empty()) {
                renderer.DrawQuads(corners, FlatMaterial{}, hud.projection);
            }
        }
    }
}

// RenderTextToDisplayList() of the text of the HUD after the commands the HUD wrote before it: the
// size, the alignment, the style, and the custom colours the race set, on the canvas the HUD is
// laid out on. The custom colours are never turned off once set.
void DrawHudText(const char *text, s32 x, s32 y) {
    DrawHudCommands(g_hudDrawing, gDisplayListPtr);
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
    g_hudDrawing.text->Draw(*g_hudDrawing.renderer,
                            text,
                            x,
                            y,
                            style,
                            CanvasProjection(GetHudCanvasWidth(), GetHudCanvasHeight()));
}

// DrawFpsCounterOverlay(), DrawViewOverlay() of each view, and DrawPauseMenu(), from the routines
// of the race, their text drawn by DrawHudText() on the canvas of the HUD as the patched PC release
// lays its HUD out on its canvas, in the order of the commands they write, the gauges of the
// dashboard before the speed.
void DrawRaceHud(SceneRenderer &renderer,
                 const MenuText &text,
                 HudTextures &textures,
                 const HudCanvas &hud) {
    static std::array<GbiCommand, HUD_SCRATCH_COMMANDS> scratch;
    GbiCommand *list = gDisplayListPtr;
    gDisplayListPtr = scratch.data();
    int framebufferWidth = g_dwConfigFramebufferWidth;
    int framebufferHeight = g_dwConfigFramebufferHeight;
    g_dwConfigFramebufferWidth = hud.width;
    g_dwConfigFramebufferHeight = hud.height;
    g_hudDrawing = {&renderer, &text, &textures, &hud, scratch.data()};
    g_pfnTextSink = DrawHudText;
    DrawFpsCounterOverlay();

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

    DrawHudCommands(g_hudDrawing, gDisplayListPtr);

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

// The parts of the car of a vehicle at a level of detail.
struct CarLevel {
    // The textured parts of the car, and the painted body and windows drawn after the highlight.
    std::unique_ptr<RenderModel> car;
    std::unique_ptr<RenderModel> paint;
    std::unique_ptr<RenderModel> underside;
    // The parts of the car the specular highlight shines on.
    std::unique_ptr<RenderModel> highlighted;
};

// The parts of the car of a vehicle: each level of detail, the nearest first, once it is drawn,
// and the wheels of the nearest level, which RenderWheelModels() draws at every level.
struct CarModels {
    std::array<std::optional<CarLevel>, VEHICLE_LODS> levels;
    std::unique_ptr<RenderModel> wheel;
    std::unique_ptr<RenderModel> snowWheel;
    // The beams of the headlights of the nearest level, the one level that draws them.
    std::unique_ptr<RenderModel> beams;
};

// A car in a colour: the car, an index of g_apszCarModelPrefixes, its red, green, and blue, and
// 1 more than the vehicle whose images come from another document, or 0 for the cartridge.
using CarKey = std::array<int, 5>;

// LoadPaintDamageBitmaps(): the 64 by 64 intensities of each level of damage after the first, and
// the car of the first vehicle, whose paint sets LoadCarPaintTextures() damages, or -1 for none.
struct CarDamageMasks {
    int car = -1;
    std::array<std::vector<std::uint8_t>, CAR_DAMAGE_LEVELS> masks;
};

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
    // The documents of the images of the vehicles that do not take those of the cartridge.
    std::map<int, std::filesystem::path> images;
    std::unique_ptr<RenderModel> startLights;
    std::unique_ptr<MenuText> text;
    // The images of the day and the night sky the paint and the windows of the car reflect.
    std::unique_ptr<RenderTexture> daySky;
    std::unique_ptr<RenderTexture> nightSky;
    // The shadow of the car, the 8-bit intensities of the engine, the alpha the shade.
    std::unique_ptr<RenderTexture> shadowTexture;
    CarDamageMasks damage;
    // LoadParticleTexture(): the 64 by 64 intensities of a puff of smoke.
    std::unique_ptr<RenderTexture> particleTexture;
    // LoadTireTrailTexture(): the 32 by 64 intensities of 4 bits of the spray of the tyres.
    std::unique_ptr<RenderTexture> sprayTexture;
    // CreateFallbackEnvironmentTexture() and LoadVehicleEnvMapTexture(): the 64 by 64 intensities
    // of 4 bits of the highlight by day and at night.
    std::unique_ptr<RenderTexture> dayHighlight;
    std::unique_ptr<RenderTexture> nightHighlight;
    // The textures of the engine the HUD draws, the gauges of the dashboard.
    HudTextures hudTextures;
    // The vertices of the pool of the quads of the track the skid marks of the frame have taken.
    int skidVertices = 0;
    // The streak of the rain of each view, and the texels it was made from.
    std::array<std::vector<std::uint8_t>, WEATHER_VIEWS> streakTexels;
    std::array<std::unique_ptr<RenderTexture>, WEATHER_VIEWS> streaks;
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
// the alpha of texture entry 0, as the PC release never sets the texture of the skid marks.
// InitializeTextureSubsystem() empties the entries before each race, and
// CreateWeatherShadeTexture() then makes entry 0 a 4 by 4 texture of 8-bit intensities, 0x20 in the
// rain and the snow and 0x80 in the other weathers, its alpha the intensity. The environment alpha
// takes the place of the texture of one alpha.
constexpr std::array<std::uint32_t, 2> SKID_COMBINE = CombineLerp(ZERO_AB,
                                                                  ZERO_AB,
                                                                  ZERO_D,
                                                                  ENVIRONMENT,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ENVIRONMENT,
                                                                  ZERO_AB,
                                                                  ZERO_AB,
                                                                  ZERO_D,
                                                                  ENVIRONMENT,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ENVIRONMENT);

// The render mode of the skid marks blends them by their alpha without writing the depth, a texel
// of an alpha below 1 drawing nothing, or below 8 at night and in the rain. The quads draw where an
// instance under them is in view and not near the eighth vehicle, from the pool of vertices every
// view of the frame shares.
void DrawSkidMarks(SceneRenderer &renderer,
                   std::array<float, 4> color,
                   const Matrix4 &viewProjection,
                   const std::vector<unsigned char> &mask,
                   int &vertices) {
    std::vector<FlatVertex> corners;
    VisitSkidMarkQuads(
        [](const Vec3f *oldRight,
           const Vec3f *newRight,
           const Vec3f *oldLeft,
           const Vec3f *newLeft,
           void *context) {
            auto &quads = *static_cast<std::vector<FlatVertex> *>(context);
            for (const Vec3f *p : {oldRight, newRight, newLeft, oldLeft}) {
                quads.push_back({p->x, p->y, 0.0f, 0.0f, {255, 255, 255, 255}, p->z});
            }
        },
        &corners,
        mask.data(),
        &vertices);
    if (corners.empty()) {
        return;
    }
    color[3] =
        (g_dwChosenWeather == WEATHER_RAIN || g_dwChosenWeather == WEATHER_SNOW ? 0x20 : 0x80) /
        255.0f;
    FlatMaterial material;
    material.combine = SKID_COMBINE;
    material.env = color;
    material.alphaCut = (!g_dwNightMode && !g_dwRainMode ? 1.0f : 8.0f) / 255.0f;
    renderer.DrawQuads(corners, material, viewProjection, true);
}

// The shadow of a car as RenderTrackInstance() lays it on the instances near the car, those whose
// byte of g_abInstanceVisibilityMask has the bit of the vehicle.
CarShadow ShadowOf(const RenderTexture *texture,
                   const Vehicle &car,
                   unsigned bit,
                   const std::vector<unsigned char> &mask) {
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
    for (int index = 0; index < g_LoadedTrack.instanceCount; ++index) {
        if ((mask[static_cast<std::size_t>(index)] & bit) != 0 &&
            !(g_LoadedTrack.instances[index].flags & TRACK_INSTANCE_NO_SHADOW)) {
            shadow.instances.push_back(index);
        }
    }
    return shadow;
}

// RenderTrackInstance() lays the shadow of each vehicle BuildVisibleInstanceList() gives a shadow
// in a view, in the order of the vehicles, but none with the shadows off or from the mirror, and
// none of the car of the view from its cockpit or its bumper. The byte of an instance the view does
// not list has every bit.
std::vector<CarShadow> ShadowsOf(const RenderTexture *texture,
                                 const Vehicle *vehicles,
                                 int cameraIndex,
                                 const std::vector<unsigned char> &mask) {
    std::vector<CarShadow> shadows;
    const Vehicle &viewer = vehicles[cameraIndex];
    const CameraFrame *active = viewer.GetActiveCamera();
    const auto &cameras = viewer.GetCameras();
    if (texture == nullptr || !g_GlobalRallyConfig.bCarShadowSelection ||
        active == &cameras[VEHICLE_CAMERA_MIRROR]) {
        return shadows;
    }
    bool inside =
        active == &cameras[VEHICLE_CAMERA_COCKPIT] || active == &cameras[VEHICLE_CAMERA_BUMPER];
    std::vector<unsigned char> listed = mask;
    std::ranges::replace(listed, 0xFF, 0);
    // The bit of a vehicle past the eighth is lost to the byte.
    for (int index = 0; index < std::min<int>(g_dwNumActiveVehicles, 8); ++index) {
        if (index == cameraIndex && inside) {
            continue;
        }
        CarShadow shadow = ShadowOf(texture, vehicles[index], 1u << index, listed);
        if (!shadow.instances.empty()) {
            shadows.push_back(std::move(shadow));
        }
    }
    return shadows;
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

// RenderTireSmoke() by day, in the colours of the dirt and of the pavement of the track.
void DrawTireSmoke(SceneRenderer &renderer,
                   const RenderTexture &smoke,
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
}

// RenderExhaustAndTireSpray() in the snow, the exhaust in the half of its texture the PC release
// samples.
void DrawExhaust(SceneRenderer &renderer,
                 const RenderTexture &exhaust,
                 const CameraFrame &camera,
                 const Matrix4 &viewProjection) {
    FlatMaterial material;
    material.combine = PARTICLE_COMBINE;
    if (g_dwSnowMode) {
        std::vector<FlatVertex> quads;
        AddBillboards(quads, g_wRenderExhaustParticleListHead, {0xE0, 0xE0, 0xFF}, camera, 0.5f);
        if (!quads.empty()) {
            material.texel0 = &exhaust;
            renderer.DrawQuads(quads, material, viewProjection, true);
        }
    }
}

// RenderPrecipitationEffect(): the colour of the rain or the snow, its alpha the texel.
constexpr std::array<std::uint32_t, 2> PRECIPITATION_COMBINE = CombineLerp(ZERO_AB,
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

// The scale from the place of a particle of the rain or the snow to the units of the world.
constexpr float PRECIPITATION_SCALE = 6.10370189e-05f;

// RenderPrecipitationEffect() of the view the globals of the race look from, to a scratch list, for
// the streak of the rain it writes into the texture of the view through CopyTileColumnScaled():
// the white texels of the alpha ramp along the fall of the rain on the screen. The commands are not
// read.
std::vector<std::uint8_t> RainStreakTexels(int view) {
    static std::array<GbiCommand, RAIN_PARTICLES + 64> scratch;
    GbiCommand *list = gDisplayListPtr;
    gDisplayListPtr = scratch.data();
    RenderPrecipitationEffect();
    gDisplayListPtr = list;
    const auto &texels = g_abPrecipitationTextures[view];
    return {std::begin(texels), std::end(texels)};
}

// RenderPrecipitationEffect() of a view: each particle in front of the camera and on the screen is
// a rectangle of the texture from the particle, at half the scale of the projection, its size 96
// by 144 for the snow and 128 for the rain over its distance, in quarters of a pixel of the 480
// lines of the screen of the PC release. A particle lies around the camera opposite to its place
// in the axes of the world. The rectangle takes the texture upside down, the half of its texture
// the snow samples, in the colour of the rain or of the snow by the alpha of the texture. On a
// mirrored track the race negates the places across and the fall of the rain, and the viewport
// turns both back, so the particles keep their places and only the rectangles turn.
void DrawPrecipitation(SceneRenderer &renderer,
                       const RenderTexture &texture,
                       int view,
                       const CameraFrame &camera,
                       const Matrix4 &viewProjection,
                       float widen,
                       bool across) {
    float width = g_dwSnowMode ? 96.0f : 128.0f;
    float height = g_dwSnowMode ? 144.0f : 128.0f;
    float textureScale = g_dwSnowMode ? 0.5f : 1.0f;
    float first = 0.5f / 64.0f * textureScale;
    float last = 63.5f / 64.0f * textureScale;
    std::array<std::uint8_t, 4> color = g_dwSnowMode ?
                                            std::array<std::uint8_t, 4>{0xE0, 0xE0, 0xFF, 0xFF} :
                                            std::array<std::uint8_t, 4>{0x78, 0x80, 0x88, 0xFF};
    std::vector<FlatVertex> corners;
    for (s32 i = 0; i < g_dwRainParticleCount; ++i) {
        const s16 *particle = g_awRainParticlePos[view][i];
        std::array<float, 4> place{
            camera.position.x - static_cast<float>(particle[0]) * PRECIPITATION_SCALE,
            camera.position.y - static_cast<float>(particle[1]) * PRECIPITATION_SCALE,
            camera.position.z - static_cast<float>(particle[2]) * PRECIPITATION_SCALE,
            1.0f};
        std::array<float, 4> clip{};
        for (std::size_t row = 0; row < 4; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                clip[row] += viewProjection[column * 4 + row] * place[column];
            }
        }
        if (!(clip[3] >= 0.1f)) {
            continue;
        }
        float scale = 0.5f / clip[3];
        float top = clip[1] * scale;
        float left = clip[0] * scale;
        if (!(top >= -1.0f) || top > 1.0f || !(left >= -1.0f) || left > 1.0f) {
            continue;
        }
        auto sizeX = static_cast<int>(scale * width);
        auto sizeY = static_cast<int>(scale * height);
        if (sizeX < 2 || sizeY < 2) {
            continue;
        }
        float span = static_cast<float>(sizeX) / 1280.0f / widen;
        float right = across ? left - span : left + span;
        float bottom = top - static_cast<float>(sizeY) / 960.0f;
        corners.push_back({left, top, first, last, color, 0.0f});
        corners.push_back({right, top, last, last, color, 0.0f});
        corners.push_back({right, bottom, last, first, color, 0.0f});
        corners.push_back({left, bottom, first, first, color, 0.0f});
    }
    if (corners.empty()) {
        return;
    }
    FlatMaterial material;
    material.texel0 = &texture;
    material.combine = PRECIPITATION_COMBINE;
    material.prim = {color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, 1.0f};
    renderer.DrawQuads(corners, material, IDENTITY_MATRIX);
}

// The glare of the beams of the headlights that shine at the camera of a view, as
// RenderVehicleHeadlightBeams() adds it up: of the beams in front of the camera in
// g_fHeadlightGlare, and of those behind it, which its mirror faces, in g_fScreenFogPulse.
struct HeadlightGlare {
    float ahead = 0.0f;
    float behind = 0.0f;
};

// RenderVehicleHeadlightBeams() of a vehicle other than the car of the view: its beams glare the
// camera the more the more straight they shine at it, and the nearer they are. The beams point
// along the vehicle, as the tilt of their up axis is 0.
void AddHeadlightGlare(HeadlightGlare &glare, const Vehicle &car, const CameraFrame &camera) {
    CameraFrame frame = car.GetFrame();
    Vec3f eye = camera.position;
    Vec3f toCamera{};
    Vec3fAdd(&toCamera, &frame.position, &frame.forward);
    Vec3fSubtract(&toCamera, &eye, &toCamera);
    long double rawDistance = Vec3fLength(&toCamera);
    if (rawDistance == 0.0L || rawDistance != rawDistance) {
        return;
    }
    auto distance = static_cast<f32>(rawDistance);
    Vec3fDivideInPlace(&toCamera, distance);
    long double squared = static_cast<long double>(distance) * distance;
    long double facing = Vec3fDot(&toCamera, &camera.forward);
    auto along = static_cast<f32>(Vec3fDot(&toCamera, &frame.forward));
    if (!(facing >= 0.0L)) {
        long double level = -(facing * along);
        if (0.95f < level) {
            glare.ahead += static_cast<f32>(750.0f * (level - 0.95f) / squared);
        }
    }
    if (facing > 0.95f) {
        long double level = facing * along;
        if (0.95f < level) {
            glare.behind += static_cast<f32>(750.0f * (level - 0.95f) / squared);
        }
    }
}

// RenderHeadlightGlare(): the view in white by the glare, from a glare of 0.1 and at most 0.7,
// blended over the frame.
void DrawHeadlightGlare(SceneRenderer &renderer, float level) {
    if (!(level >= 0.1f)) {
        return;
    }
    level = std::min(level, 0.7f);
    auto alpha = static_cast<std::uint8_t>(static_cast<s32>(static_cast<s64>(255.0L * level)));
    std::array<std::uint8_t, 4> white{0xFF, 0xFF, 0xFF, alpha};
    std::array<FlatVertex, 4> corners{{{-1.0f, 1.0f, 0.0f, 0.0f, white, 0.0f},
                                       {1.0f, 1.0f, 0.0f, 0.0f, white, 0.0f},
                                       {1.0f, -1.0f, 0.0f, 0.0f, white, 0.0f},
                                       {-1.0f, -1.0f, 0.0f, 0.0f, white, 0.0f}}};
    renderer.DrawQuads(corners, FlatMaterial{}, IDENTITY_MATRIX);
}

// The combiner of the spray of the tyres: the shade, its alpha the texel by the shade.
constexpr std::array<std::uint32_t, 2> SPRAY_COMBINE = PARTICLE_COMBINE;

// The width and the height of the texture of the trails of the tyres, in the 1/32 texels of the
// coordinates of a vertex of the PC release.
constexpr float SPRAY_TEXTURE_S = 32.0f * 32.0f;
constexpr float SPRAY_TEXTURE_T = 32.0f * 64.0f;

// The palettes of the lights of a car of the cartridge. ByteSwapAndRelocateCarModel() lights the
// headlights at night and in the rain. The rear lights take the palette SetVehicleLightPalette()
// gives each of the four light textures of CreateCarLightTextures(), lit at night and in the
// rain, and RenderVehicleModel() draws the texture for a held brake and a throttle that is not at
// or above 0. A variant is the sum of 1 when lit, 2 when braking, and 4 when reversing.
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
        // The texture of the first light sets the entries the other three retain.
        std::array<u16, 16> words{};
        SetVehicleLightPalette(words.data(), 0, lit);
        int light = variant / CAR_BRAKING;
        if (light != 0) {
            SetVehicleLightPalette(words.data(), light, 0);
        }
        for (int entry = 6; entry < 16; ++entry) {
            set(entry, PackMaterialPaletteColor(words[static_cast<std::size_t>(entry)]));
        }
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
    if (!(car.GetThrottle() >= 0.0f)) {
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

// The lights BuildVisibleInstanceList() gives the track and RenderTrackGeometry() loads, in the
// colours of the weather. The direction is the sun, or at night above and ahead of the car of the
// camera, at 120 a unit rounded to bytes.
Lighting TrackLighting(const Vehicle &cameraCar) {
    Vec3f light{g_afLightDirection[0], g_afLightDirection[1], g_afLightDirection[2]};
    if (g_dwNightMode) {
        const CameraFrame &frame = cameraCar.GetFrame();
        light = {static_cast<f32>(static_cast<long double>(frame.up.x) -
                                  static_cast<long double>(frame.forward.x) * 4.0f),
                 static_cast<f32>(static_cast<long double>(frame.up.y) -
                                  static_cast<long double>(frame.forward.y) * 4.0f),
                 static_cast<f32>(static_cast<long double>(frame.up.z) -
                                  static_cast<long double>(frame.forward.z) * 4.0f)};
    }
    Vec3fNormalize(&light);
    Vec3fScaleInPlace(&light, 120.0f);
    std::array<float, 3> rounded{static_cast<float>(static_cast<s8>(static_cast<s32>(light.x))),
                                 static_cast<float>(static_cast<s8>(static_cast<s32>(light.y))),
                                 static_cast<float>(static_cast<s8>(static_cast<s32>(light.z)))};
    Lighting lighting;
    float length = std::hypot(rounded[0], rounded[1], rounded[2]);
    if (length > 0.0f) {
        lighting.direction = {rounded[0] / length, rounded[1] / length, rounded[2] / length};
    }
    lighting.ambient = {static_cast<float>(g_bAmbientLightR) / 255.0f,
                        static_cast<float>(g_bAmbientLightG) / 255.0f,
                        static_cast<float>(g_bAmbientLightB) / 255.0f};
    lighting.directional = {static_cast<float>(g_bDirLightR) / 255.0f,
                            static_cast<float>(g_bDirLightG) / 255.0f,
                            static_cast<float>(g_bDirLightB) / 255.0f};
    for (std::size_t i = 0; i < lighting.ramp.size(); ++i) {
        lighting.ramp[i] = {static_cast<float>(g_abLightRampR[i]) / 255.0f,
                            static_cast<float>(g_abLightRampG[i]) / 255.0f,
                            static_cast<float>(g_abLightRampB[i]) / 255.0f};
    }
    // A ramped instance lit at night takes half the directional colour.
    if (g_dwNightMode) {
        lighting.nightLit = {static_cast<float>(g_bDirLightR >> 1) / 255.0f,
                             static_cast<float>(g_bDirLightG >> 1) / 255.0f,
                             static_cast<float>(g_bDirLightB >> 1) / 255.0f};
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

// The options of the models of a car: its textures those of the cartridge or of another document,
// painted as SetMaterialColor() paints them, and the car of the first vehicle damaged as
// LoadVehicleModelFromRom() damages the paint sets of the first model. The paint sets of a car
// are loaded once, and a computer's car of the same kind shares the damaged sets.
ModelOptions CarOptions(const std::filesystem::path &defaults,
                        const std::string &prefix,
                        const CarKey &key,
                        const std::optional<std::filesystem::path> &images,
                        const CarDamageMasks &damage) {
    ModelOptions options;
    options.paint = std::array<std::uint8_t, 3>{static_cast<std::uint8_t>(key[1]),
                                                static_cast<std::uint8_t>(key[2]),
                                                static_cast<std::uint8_t>(key[3])};
    options.images = images ? images : CartridgeCar(defaults, prefix);
    if (key[0] == damage.car) {
        int car = key[0];
        options.damage = [car](int set) -> std::optional<ImageDamage> {
            if (set >= CAR_PAINT_SET_COUNT) {
                return std::nullopt;
            }
            const CarPaintTextureSet &paint = g_CarPaintTextureSets[car][set];
            if (paint.pszBitmapName == nullptr || paint.nOption10 < 0 ||
                paint.nOption10 >= CAR_DAMAGE_SIDES) {
                return std::nullopt;
            }
            return ImageDamage{paint.nOption10,
                               {paint.nLeft, paint.nRight, paint.nTop, paint.nBottom}};
        };
        options.damageMasks = &damage.masks;
    }
    return options;
}

// A level of detail of a car in its parts, the children of the root of the level: the textured
// parts, the body, and the underside, and the highlighted parts, a node of their own. A part the
// level does not have is none, as RenderVehicleModel() passes over a display list the level does
// not have, and only a part of the nearest level is missed in the log.
// ponytail: each part, each level, and each colour of a car loads the images of the car again; a
// model drawn by parts and painted as it is drawn would share them.
CarLevel LoadCarLevel(SceneRenderer &renderer,
                      const std::filesystem::path &defaults,
                      const CarKey &key,
                      int lod,
                      const std::optional<std::filesystem::path> &images,
                      const CarDamageMasks &damage) {
    std::string prefix = g_apszCarModelPrefixes[key[0]];
    auto carFile = defaults / "cars" / (prefix + ".gltf");
    std::string root = "lod" + std::to_string(lod);
    std::string level = root + "-";
    CarLevel parts;
    ModelOptions options = CarOptions(defaults, prefix, key, images, damage);
    options.optional = lod != 0;
    options.node = root;
    options.parts = [&level](const std::string &n) {
        return n == level + "nearDetail" || n == level + "textured";
    };
    options.variants = CAR_LIGHT_VARIANTS;
    options.paletteVariant = CarLightPalette;
    parts.car = RenderModel::Load(renderer, carFile, options);
    options.variants = 0;
    options.paletteVariant = nullptr;
    options.parts = [&level](const std::string &n) { return n == level + "paint"; };
    parts.paint = RenderModel::Load(renderer, carFile, options);
    options.parts = [&level](const std::string &n) { return n == level + "underside"; };
    parts.underside = RenderModel::Load(renderer, carFile, options);
    // The highlighted parts are a node outside the scene, drawn without the turn of its root.
    options.parts = nullptr;
    options.node = level + "highlighted";
    parts.highlighted = RenderModel::Load(renderer, carFile, options);
    return parts;
}

// The car of the nearest level of detail, the one level a single view draws, and a wheel of each
// kind, which the race places one by one. The geometry of the car is that of the PC release, which
// its race moves. None of the parts load when the car is missing.
CarModels LoadCarModels(SceneRenderer &renderer,
                        const std::filesystem::path &defaults,
                        const CarKey &key,
                        const std::optional<std::filesystem::path> &images,
                        const CarDamageMasks &damage) {
    std::string prefix = g_apszCarModelPrefixes[key[0]];
    auto carFile = defaults / "cars" / (prefix + ".gltf");
    CarModels models;
    models.levels[0] = LoadCarLevel(renderer, defaults, key, 0, images, damage);
    ModelOptions options = CarOptions(defaults, prefix, key, images, damage);
    options.node = "lod0-wheel0";
    models.wheel = RenderModel::Load(renderer, carFile, options);
    options.node = "lod0-wheelSnow0";
    models.snowWheel = RenderModel::Load(renderer, carFile, options);
    // The beams are a node outside the scene, drawn without the turn of its root.
    options.node = "lod0-headlightBeams";
    options.optional = true;
    models.beams = RenderModel::Load(renderer, carFile, options);
    if (!models.levels[0]->car || !models.levels[0]->paint || !models.wheel) {
        return {};
    }
    return models;
}

// The masks of LoadPaintDamageBitmaps() from Paint of the import, the red of each texel. A car
// without every mask takes no damage, as ApplyPaintDamageOverlay() passes over a missing mask.
CarDamageMasks LoadDamageMasks(const std::filesystem::path &defaults, int car) {
    constexpr int MASK_SIDE = 64;
    CarDamageMasks damage;
    for (int level = 0; level < CAR_DAMAGE_LEVELS; ++level) {
        auto path = defaults / "Paint" / ("damage" + std::to_string(level + 1) + ".png");
        SDL_Surface *loaded = SDL_LoadPNG(path.string().c_str());
        SDL_Surface *rgba = loaded ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32) : nullptr;
        SDL_DestroySurface(loaded);
        if (rgba == nullptr || rgba->w != MASK_SIDE || rgba->h != MASK_SIDE) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "%s is not a mask of damage of 64 by 64 texels.",
                        path.string().c_str());
            SDL_DestroySurface(rgba);
            return {};
        }
        for (int y = 0; y < MASK_SIDE; ++y) {
            const auto *row = static_cast<const std::uint8_t *>(rgba->pixels) + y * rgba->pitch;
            for (int x = 0; x < MASK_SIDE; ++x) {
                damage.masks[static_cast<std::size_t>(level)].push_back(row[x * 4]);
            }
        }
        SDL_DestroySurface(rgba);
    }
    damage.car = car;
    return damage;
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
    ModelOptions sky;
    sky.node = "sky";
    state->sky = RenderModel::Load(renderer, trackFile, sky);
    state->defaults = defaults;
    state->damage = LoadDamageMasks(defaults, cars.empty() ? -1 : cars.front().car);
    for (std::size_t i = 0; i < cars.size(); ++i) {
        const RaceCar &vehicle = cars[i];
        int custom = vehicle.images ? static_cast<int>(i) + 1 : 0;
        if (vehicle.images) {
            state->images[static_cast<int>(i)] = *vehicle.images;
        }
        CarKey key{vehicle.car, vehicle.paint[0], vehicle.paint[1], vehicle.paint[2], custom};
        CarModels models = LoadCarModels(renderer, defaults, key, vehicle.images, state->damage);
        if (!models.levels[0]) {
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
// its two commands to a scratch list.
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

// The instances of the track a view draws, as BuildVisibleInstanceList() lists them and
// RenderTrackGeometry() draws them.
struct ViewInstances {
    // The opaque instances, the nearest cells first, after the nodes of no instance.
    std::vector<int> opaque{-1};
    // The translucent instances the second pass draws, the farthest first.
    std::vector<int> translucent;
    // For each instance, its byte of g_abInstanceVisibilityMask: 0xFF when the view does not list
    // it in view, the groups it hides or not, and else the bits of the vehicles whose shadows fall
    // on it, the bit of a vehicle 1 shifted by its number in a byte.
    std::vector<unsigned char> mask;
};

// The vehicles BuildVisibleInstanceList() gives shadows, the nearest of those in view at least 2
// ahead of the camera, as many as 4.
constexpr std::size_t SHADOW_VEHICLES = 4;

// RenderRaceView() casts a ray down from the camera, and the groups the instances under it hide are
// not drawn. RenderRaceScene() checks which vehicles the camera sees with CheckVehicleVisibility().
// BuildVisibleInstanceList() lists the instances of the cells of the grid the frustum of the camera
// covers, the nearest cells first and each instance once, the airplane before the first cell it is
// in view at, and marks the instances near the vehicles it gives shadows. RenderTrackGeometry()
// draws the opaque ones in that order, and its second pass draws the translucent ones the farthest
// first.
// ponytail: the cells of one distance sort by the qsort() of the platform, and their order on the
// PC release is not reproduced.
ViewInstances VisibleInstances(const CameraFrame &camera,
                               float fov,
                               float farPlane,
                               SDL_Rect frame,
                               Vehicle *vehicles,
                               int cameraIndex) {
    Vec3f eye = camera.position;
    std::array<u16, RAY_INSTANCES> nearby{};
    s32 nearbyCount = 0;
    f32 groundDistance = 0.0f;
    s32 groundSurface = 0;
    CastRayAgainstTrack(nullptr,
                        nullptr,
                        &eye,
                        g_awCameraRayHitInstances,
                        &g_dwCameraRayHitCount,
                        nearby.data(),
                        &nearbyCount,
                        &groundDistance,
                        &groundSurface);
    u16 hidden = 0;
    for (s32 i = 0; i < g_dwCameraRayHitCount; ++i) {
        hidden |= g_LoadedTrack.instances[g_awCameraRayHitInstances[i]].hidesGroups;
    }

    CameraFrame look = camera;
    SetupFrustumCorners(&look, fov, farPlane, static_cast<f32>(frame.w), static_cast<f32>(frame.h));
    BuildVisibleGrid();
    // CheckVehicleVisibility() reads the camera and its vehicle from the globals of the view being
    // drawn.
    CameraFrame *savedCamera = g_pActiveCamera;
    Vehicle *savedVehicle = g_pActiveCameraVehicle;
    g_pActiveCameraVehicle = &vehicles[cameraIndex];
    g_pActiveCamera = g_pActiveCameraVehicle->GetActiveCamera();
    for (int i = 0; i < g_dwNumActiveVehicles; ++i) {
        CheckVehicleVisibility(&vehicles[i]);
    }
    g_pActiveCamera = savedCamera;
    g_pActiveCameraVehicle = savedVehicle;

    std::vector<VisibleCell> cells;
    for (s32 row = g_dwVisibleGridMinRow; row <= g_dwVisibleGridMaxRow; ++row) {
        for (s32 column = g_adwVisibleGridRowMinCol[row];
             column <= g_adwVisibleGridRowMaxCol[row] && cells.size() < VISIBLE_CELLS;
             ++column) {
            cells.push_back({static_cast<u8>(column), static_cast<u8>(row), 0});
        }
    }
    constexpr s32 GRID_CELLS = 64;
    constexpr float GRID_CELL_SIZE = 32.0f;
    // The cell of the camera, clamped to the grid at its far edges.
    auto cameraColumn = static_cast<s32>(camera.position.x * (1.0f / GRID_CELL_SIZE));
    if (cameraColumn == GRID_CELLS) {
        --cameraColumn;
    }
    auto cameraRow = static_cast<s32>(camera.position.y * (1.0f / GRID_CELL_SIZE));
    if (cameraRow == GRID_CELLS) {
        --cameraRow;
    }
    for (VisibleCell &cell : cells) {
        s32 across = cell.column - cameraColumn;
        s32 along = cell.row - cameraRow;
        cell.distance = static_cast<s16>(across * across + along * along);
    }
    std::qsort(cells.data(), cells.size(), sizeof(VisibleCell), CompareVisibleCellDistance);

    ViewInstances view;
    view.mask.assign(static_cast<std::size_t>(g_LoadedTrack.instanceCount), 0xFF);
    std::vector<unsigned char> &mask = view.mask;
    auto list = [&](u16 index) {
        mask[index] = 0;
        const TrackInstance &instance = g_LoadedTrack.instances[index];
        if ((instance.groups & hidden) != 0) {
            return;
        }
        if ((instance.flags & TRACK_INSTANCE_TRANSLUCENT) != 0) {
            view.translucent.insert(view.translucent.begin(), index);
        } else {
            view.opaque.push_back(index);
        }
    };
    auto airplane = static_cast<u16>(g_dwAirplaneInstance);
    for (const VisibleCell &cell : cells) {
        if (airplane != 0 && mask[airplane] != 0) {
            const f32 *position = g_LoadedTrack.instances[airplane].matrix[3];
            if (IsPointVisible(position[0], position[1])) {
                list(airplane);
            }
        }
        u32 found = LookupInstanceGridCell(cell.column, cell.row);
        if (found == 0) {
            continue;
        }
        TrackGridCursor cursor{static_cast<u16>(found), static_cast<u16>(found >> 16)};
        for (u16 index = NextGridCellInstance(&cursor); index != 0;
             index = NextGridCellInstance(&cursor)) {
            if (mask[index] != 0 && index != airplane) {
                list(index);
            }
        }
    }

    std::vector<std::pair<long double, int>> nearest;
    for (int i = 0; i < g_dwNumActiveVehicles; ++i) {
        const Vehicle &car = vehicles[i];
        if (car.GetDriver() == nullptr || !g_adwVehicleVisible[i]) {
            continue;
        }
        Vec3f delta{};
        Vec3fSubtract(&delta, &car.GetFrame().position, &camera.position);
        long double dot = Vec3fDot(&camera.forward, &delta);
        if (dot >= 2.0L) {
            nearest.emplace_back(dot, i);
        }
    }
    std::stable_sort(nearest.begin(), nearest.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });
    if (nearest.size() > SHADOW_VEHICLES) {
        nearest.resize(SHADOW_VEHICLES);
    }
    for (const auto &[dot, i] : nearest) {
        // The bit of a vehicle past the eighth is lost to the byte.
        vehicles[i].MarkNearbyInstanceShadows(
            mask.data(), static_cast<u8>(1u << (static_cast<unsigned>(i) & 31)));
    }
    return view;
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

// The rectangle of RenderRearViewMirror() on the screen of the race: a quarter, five sixteenths,
// or three eighths and two of the width of the view by g_dwRearViewMirrorMode, a quarter of that
// high, centred across and a sixteenth of the height of the view from its top.
ViewportRect RearViewMirrorRect(const ViewportRect &view) {
    ViewportRect mirror = view;
    if (g_dwRearViewMirrorMode == 1) {
        mirror.width = view.width / 4;
    } else if (g_dwRearViewMirrorMode == 2) {
        mirror.width = view.width * 5 / 16;
    } else {
        mirror.width = view.width * 3 / 8 + 2;
    }
    mirror.height = mirror.width >> 2;
    mirror.x = ((view.width - mirror.width) >> 1) + view.x;
    mirror.y = view.height / 16 + view.y;
    return mirror;
}

// The mirror of a view on the frame: its rectangle on the screen of the race, and in pixels of the
// frame with the shape of that rectangle, and the pixels of the frame for a pixel of the screen.
struct MirrorPlace {
    ViewportRect screen;
    SDL_Rect frame;
    float scale;
};

// The mirror of a view keeps its shape and its size against the view as the view has on the
// screen of the race, scaled by the height of the view on a wider frame and by its width on a
// narrower one, centred across the view.
MirrorPlace RearViewMirrorOnFrame(const ViewportRect &view, const GpuWindow &window) {
    MirrorPlace place{RearViewMirrorRect(view), FrameRect(view, window), 1.0f};
    SDL_Rect viewPixels = place.frame;
    place.scale = std::min(static_cast<float>(viewPixels.w) / static_cast<float>(view.width),
                           static_cast<float>(viewPixels.h) / static_cast<float>(view.height));
    auto width = static_cast<int>(static_cast<float>(place.screen.width) * place.scale);
    place.frame = {viewPixels.x + (viewPixels.w - width) / 2,
                   viewPixels.y +
                       static_cast<int>(static_cast<float>(place.screen.y - view.y) * place.scale),
                   width,
                   static_cast<int>(static_cast<float>(place.screen.height) * place.scale)};
    return place;
}

// DrawRdpBorderRect() of BRD3D around the mirror: each of its fill rectangles, from its first to
// its last pixel, clears the frame to the black of the fill colour in either render mode.
void DrawRearViewMirrorFrame(SceneRenderer &renderer, const MirrorPlace &place) {
    GpuWindow &window = renderer.Window();
    auto width = static_cast<int>(window.FrameWidth());
    auto height = static_cast<int>(window.FrameHeight());
    const ViewportRect &mirror = place.screen;
    // A pixel of the screen from the mirror on the screen to the mirror on the frame.
    auto across = [&](int x) {
        return static_cast<float>(place.frame.x) + static_cast<float>(x - mirror.x) * place.scale;
    };
    auto down = [&](int y) {
        return static_cast<float>(place.frame.y) + static_cast<float>(y - mirror.y) * place.scale;
    };
    auto fill = [&](int left, int top, int right, int bottom) {
        std::array<std::uint8_t, 4> black{0, 0, 0, 0xFF};
        float x0 = across(left);
        float y0 = down(top);
        float x1 = across(right + 1);
        float y1 = down(bottom + 1);
        std::array<FlatVertex, 4> corners{{{x0, y0, 0.0f, 0.0f, black, 0.0f},
                                           {x1, y0, 0.0f, 0.0f, black, 0.0f},
                                           {x1, y1, 0.0f, 0.0f, black, 0.0f},
                                           {x0, y1, 0.0f, 0.0f, black, 0.0f}}};
        renderer.DrawQuads(corners, FlatMaterial{}, CanvasProjection(width, height));
    };
    int x = mirror.x;
    int y = mirror.y;
    int right = mirror.x + mirror.width;
    int bottom = mirror.y + mirror.height;
    fill(x - 1, y - 2, right, y - 1);
    fill(x - 1, bottom, right, bottom + 1);
    fill(x - 2, y - 1, x - 1, bottom);
    fill(right, y - 1, right + 1, bottom);
    fill(x, y, right, y + 1);
    fill(x, bottom - 1, right, bottom);
    fill(x, y, x + 1, bottom);
    fill(right - 1, y, right, bottom);
    fill(x - 3, y - 3, right + 3, y - 2);
    fill(x - 3, bottom + 2, right + 3, bottom + 3);
    fill(x - 3, y - 3, x - 2, bottom + 3);
    fill(right + 2, y - 3, right + 3, bottom + 3);
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
    if (snapshot != nullptr) {
        RecordFrameSkidMarks(snapshot->GetVehicles());
    }
    // The puffs of the snapshot the frame draws.
    if (snapshot != nullptr) {
        LoadParticleListHeads(&snapshot->GetParticles());
    }
    // ResetShadowBuffers() empties the pool of the quads of the skid marks once a frame.
    s.skidVertices = 0;
    renderer.SetDrawnInOrder(true);
    Vehicle *vehicles = snapshot != nullptr ? snapshot->GetVehicles() : g_aVehicles;
    for (int index = 0; index < ViewCount(); ++index) {
        DrawView(renderer, snapshot, index, false);
        // RenderRaceView() draws the rear-view mirror over a single view from the bumper. While
        // RenderRearViewMirror() draws it, the mirror is the active camera of the vehicle.
        int cameraIndex = g_aViewportRects[index].vehicleIndex;
        Vehicle &vehicle = vehicles[cameraIndex];
        Vehicle &live = g_aVehicles[cameraIndex];
        if (vehicle.GetActiveCamera() != &vehicle.GetCameras()[VEHICLE_CAMERA_BUMPER] ||
            g_dwNumViewports != 1 || !g_dwRearViewMirrorMode) {
            continue;
        }
        CameraFrame *saved = vehicle.GetActiveCamera();
        CameraFrame *savedLive = live.GetActiveCamera();
        vehicle.SetActiveCamera(&vehicle.GetCameras()[VEHICLE_CAMERA_MIRROR]);
        live.SetActiveCamera(&live.GetCameras()[VEHICLE_CAMERA_MIRROR]);
        DrawView(renderer, snapshot, index, true);
        live.SetActiveCamera(savedLive);
        vehicle.SetActiveCamera(saved);
        renderer.SetViewport(std::nullopt);
        renderer.SetMirrored(g_dwTrackMirrored != 0);
        DrawRearViewMirrorFrame(renderer,
                                RearViewMirrorOnFrame(g_aViewportRects[index], renderer.Window()));
    }
    renderer.SetDrawnInOrder(false);
    renderer.SetViewport(std::nullopt);
    renderer.SetFog(std::nullopt);

    DrawRaceHud(renderer, *s.text, s.hudTextures, hud);

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

// A car drawn in a view: the car, the car of the view and its number, the camera of the view, its
// projection, and the lights of the car.
struct RaceView::CarInView {
    const Vehicle *car;
    // The index of the car among the vehicles, which the car of a snapshot does not give by its
    // address.
    int carIndex;
    const Vehicle *cameraCar;
    int cameraIndex;
    const CameraFrame *camera;
    const Matrix4 *viewProjection;
    const Lighting &lighting;
};

// RenderRaceView() of a view: the track, the cars, the puffs, and the start lights from the active
// camera of the car of the view.
void RaceView::DrawView(SceneRenderer &renderer,
                        FrameSnapshot *snapshot,
                        int index,
                        bool mirror) const {
    State &s = *m_state;
    GpuWindow &window = renderer.Window();
    int cameraIndex = g_aViewportRects[index].vehicleIndex;
    Vehicle *vehicles = snapshot != nullptr ? snapshot->GetVehicles() : g_aVehicles;
    const Vehicle &vehicle = vehicles[cameraIndex];
    const Vehicle &first = vehicles[0];
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
    // The mirror fills its rectangle, its frustum reaches a fifth as far and its projection three
    // tenths as far, and its depth is not cleared, as ClearZBufferRect() of the PC release is
    // empty.
    if (mirror) {
        frame = RearViewMirrorOnFrame(g_aViewportRects[index], window).frame;
        renderer.SetViewport(frame);
    }
    float aspect = static_cast<float>(frame.w) / static_cast<float>(std::max(frame.h, 1));
    // SetupCameraMatrix() takes the width and the height of the view, or of the mirror, on the
    // screen of the race, and its field is the vertical field of that shape.
    ViewportRect screen =
        mirror ? RearViewMirrorRect(g_aViewportRects[index]) : g_aViewportRects[index];
    float fov = (split ? camera->fov * 0.7f : camera->fov) * ASPECT_4_3 *
                static_cast<float>(screen.height) / static_cast<float>(std::max(screen.width, 1));
    fov = RaceFieldOfView(fov, aspect);
    float farPlane = split ? CAMERA_FAR * 0.65f : CAMERA_FAR;
    float frustumFar = farPlane;
    if (mirror) {
        farPlane = CAMERA_FAR * 0.3f;
        frustumFar = CAMERA_FAR * 0.2f;
    }
    Matrix4 viewProjection = CameraMatrix(*camera, fov, farPlane, aspect);
    // SetViewport() mirrors the view across on a mirrored track, where the steering turns the
    // other way, and for the negative width of the mirror, and the faces of the models then turn
    // the other way. The mirror of a mirrored track is not mirrored.
    bool across = (g_dwTrackMirrored != 0) != mirror;
    renderer.SetMirrored(across);
    if (across) {
        constexpr Matrix4 ACROSS{-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        viewProjection = MultiplyMatrices(ACROSS, viewProjection);
    }
    LookFromView(index);
    // RenderSkybox() fills the view of a split screen or the mirror with the fog, where a single
    // view is cleared to it.
    if ((split || mirror) && IsSkyFogFilled() && !g_dwSkyboxDisabled) {
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
    // RenderTrackGeometry(): the first pass draws the opaque instances in view.
    ViewInstances instances =
        VisibleInstances(*camera, fov, frustumFar, frame, vehicles, cameraIndex);
    std::vector<CarShadow> shadows =
        ShadowsOf(s.shadowTexture.get(), vehicles, cameraIndex, instances.mask);
    const Lighting trackLighting = TrackLighting(vehicle);
    renderer.DrawShadowedModel(*s.track,
                               ConsoleSpace(),
                               viewProjection,
                               trackLighting,
                               std::move(instances.opaque),
                               shadows);
    // RenderVehicleModel() draws a car the view sees, but not the car of the view from its own
    // cockpit or its mirror, which returns before it sets the colours of the lights.
    const auto &cameras = vehicle.GetCameras();
    bool inside =
        camera == &cameras[VEHICLE_CAMERA_COCKPIT] || camera == &cameras[VEHICLE_CAMERA_MIRROR];
    // The lights the start lights draw under: those of the track until a car sets its own.
    Lighting lastLighting = trackLighting;
    // RenderRacerVehicles(): the car of each vehicle of the race that is, or is not, a ghost. A
    // ghost, the car of a time attack recorded before or a car fading back in after it is put back
    // on the track, is drawn in the render mode of a ghost of BRD3D. The opacity of a ghost goes to
    // the alpha of the primitive colour, and that render mode does not read it.
    auto drawCars = [&](bool ghosts) {
        for (int index = 0; index < std::max(g_dwNumActiveVehicles, 1); ++index) {
            const Vehicle &car = vehicles[index];
            const auto &wheels = car.GetBody().wheels;
            if ((car.GetGhostMode() == VEHICLE_GHOST) != ghosts || car.GetDriver() == nullptr ||
                std::ranges::find(wheels, nullptr) != std::end(wheels) ||
                !g_adwVehicleInViewRange[index] ||
                (index == cameraIndex && inside && !g_dwVehicleCullingDisabled)) {
                continue;
            }
            lastLighting = CarLighting(car, vehicle, *camera);
            DrawCar(renderer,
                    {&car, index, &vehicle, cameraIndex, camera, &viewProjection, lastLighting});
        }
    };
    // RenderRaceScene() draws the cars and the smoke of the tyres after the second pass of the
    // track in the snow on every track but the coastline, there the smoke only in the view and not
    // in its mirror.
    bool carsAfterTrack = g_dwSnowMode && g_dwSelectedTrackId != TRACK_COASTLINE &&
                          g_dwSelectedTrackId != TRACK_MIRROR_COASTLINE;
    auto drawCarsAndSmoke = [&]() {
        drawCars(false);
        if (snapshot != nullptr && !(carsAfterTrack && mirror)) {
            DrawTireSmoke(renderer, *s.particleTexture, *camera, viewProjection);
        }
    };
    if (!carsAfterTrack) {
        drawCarsAndSmoke();
    }
    // RenderStartLights() draws under the lights the last car drawn left, g_DefaultLights in its
    // colours, or else under those of the track, in the view and not in its mirror.
    std::optional<Matrix4> lights = StartLightsMatrix();
    if (lights && !mirror) {
        renderer.DrawModel(*s.startLights,
                           MultiplyMatrices(*lights, ConsoleSpace()),
                           viewProjection,
                           lastLighting,
                           StartLightsPoses(*s.startLights),
                           true);
    }
    // The second pass of RenderTrackGeometry() draws the translucent instances in view, and
    // DrawTrackShadows() draws the skid marks after them.
    renderer.DrawShadowedModel(*s.track,
                               ConsoleSpace(),
                               viewProjection,
                               trackLighting,
                               std::move(instances.translucent),
                               std::move(shadows));
    if (snapshot != nullptr && g_GlobalRallyConfig.bSkidMarksSelection) {
        DrawSkidMarks(
            renderer, ShadowColor(vehicle), viewProjection, instances.mask, s.skidVertices);
    }
    if (carsAfterTrack) {
        drawCarsAndSmoke();
    }
    // RenderVehicleHeadlightBeams() of each vehicle at night and in the rain, but not of a ghost,
    // of a vehicle the view does not see, or of the car of the view from its bumper.
    // The beams of the other vehicles glare the view.
    HeadlightGlare glare;
    if (g_dwNightMode || g_dwRainMode) {
        for (int index = 0; index < g_dwNumActiveVehicles; ++index) {
            const Vehicle &car = vehicles[index];
            if (!g_adwVehicleVisible[index] || car.GetGhostMode() == VEHICLE_GHOST ||
                (index == cameraIndex && camera == &cameras[VEHICLE_CAMERA_BUMPER])) {
                continue;
            }
            if (index != cameraIndex) {
                AddHeadlightGlare(glare, car, *camera);
            }
            const auto &color = car.GetColor();
            CarKey key{car.GetCarModel(),
                       color[0],
                       color[1],
                       color[2],
                       s.images.contains(index) ? index + 1 : 0};
            auto found = s.cars.find(key);
            if (found != s.cars.end() && found->second.beams) {
                renderer.DrawModel(*found->second.beams,
                                   FrameMatrix(car.GetFrame()),
                                   viewProjection,
                                   Lighting{},
                                   std::nullopt,
                                   true);
            }
        }
    }
    drawCars(true);
    // RenderExhaustAndTireSpray(): the exhaust in the snow, and the spray of the vehicles. A
    // championship has more racers than vehicles, and only the vehicles have spray. The mirror
    // draws neither.
    if (snapshot != nullptr && !mirror) {
        DrawExhaust(renderer, *s.shadowTexture, *camera, viewProjection);
        for (int index = 0; index < g_dwNumActiveVehicles; ++index) {
            DrawTireSpray(renderer, *s.sprayTexture, vehicles[index], viewProjection);
        }
    }
    // RenderPrecipitationEffect() in the rain and the snow, in the view and not in its mirror, and
    // not with the camera under a shelter. It takes the projection before a mirrored track turns
    // it. The rectangles keep their shape across a view wider than the screen of the PC release.
    bool sheltered = std::ranges::any_of(
        std::span(g_awCameraRayHitInstances, static_cast<std::size_t>(g_dwCameraRayHitCount)),
        [](u16 hit) { return (g_LoadedTrack.instances[hit].flags & TRACK_INSTANCE_SHELTER) != 0; });
    if (!mirror && (g_dwSnowMode || g_dwRainMode) && !sheltered) {
        const RenderTexture *precipitation = s.shadowTexture.get();
        if (g_dwRainMode) {
            std::vector<std::uint8_t> texels = RainStreakTexels(index);
            auto view = static_cast<std::size_t>(index);
            if (!s.streaks[view] || texels != s.streakTexels[view]) {
                s.streaks[view] = RenderTexture::FromTexels(renderer,
                                                            RAIN_STREAK_TEXTURE_SIDE,
                                                            RAIN_STREAK_TEXTURE_SIDE,
                                                            texels,
                                                            true,
                                                            false);
                s.streakTexels[view] = std::move(texels);
            }
            precipitation = s.streaks[view].get();
        }
        if (precipitation != nullptr) {
            DrawPrecipitation(renderer,
                              *precipitation,
                              index,
                              *camera,
                              CameraMatrix(*camera, fov, farPlane, aspect),
                              aspect * static_cast<float>(screen.height) /
                                  static_cast<float>(std::max(screen.width, 1)),
                              across);
        }
    }
    // RenderHeadlightGlare() over the view with half of the glare from behind, and over its mirror
    // with all of it.
    DrawHeadlightGlare(renderer,
                       mirror ? glare.behind + glare.ahead :
                                static_cast<float>(static_cast<long double>(glare.ahead) -
                                                   static_cast<long double>(glare.behind) * -0.5f));
}

// RenderVehicleModel() of a car in a view.
void RaceView::DrawCar(SceneRenderer &renderer, const CarInView &view) const {
    State &s = *m_state;
    const Vehicle &car = *view.car;
    const Vehicle &vehicle = *view.cameraCar;
    const CameraFrame &camera = *view.camera;
    const Matrix4 &viewProjection = *view.viewProjection;
    int cameraIndex = view.cameraIndex;
    const auto &color = car.GetColor();
    int vehicleIndex = view.carIndex;
    auto images = s.images.find(vehicleIndex);
    std::optional<std::filesystem::path> document;
    if (images != s.images.end()) {
        document = images->second;
    }
    CarKey key{car.GetCarModel(), color[0], color[1], color[2], document ? vehicleIndex + 1 : 0};
    auto found = s.cars.find(key);
    // A car that does not load is not tried again.
    if (found == s.cars.end()) {
        found =
            s.cars.try_emplace(key, LoadCarModels(renderer, s.defaults, key, document, s.damage))
                .first;
    }
    CarModels &models = found->second;
    if (!models.levels[0]) {
        return;
    }
    const CameraFrame &frame = car.GetFrame();
    // With the screen split in two, a car 40 from the camera is drawn at the second level of
    // detail, and one 80 from it at the third, as SPLIT_LOD1_DISTANCE and SPLIT_LOD2_DISTANCE of
    // RenderVehicleModel() pick them. Its g_dwVehicleLodBias is never set on the PC release.
    constexpr long double SPLIT_LOD1_DISTANCE = 40.0L;
    constexpr long double SPLIT_LOD2_DISTANCE = 80.0L;
    std::size_t lod = 0;
    if (g_dwNumViewports == 2) {
        Vec3f from = frame.position;
        Vec3f to = camera.position;
        long double distance = Vec3fDistance(&from, &to);
        if (distance >= SPLIT_LOD2_DISTANCE) {
            lod = 2;
        } else if (distance >= SPLIT_LOD1_DISTANCE) {
            lod = 1;
        }
    }
    if (!models.levels[lod]) {
        models.levels[lod] =
            LoadCarLevel(renderer, s.defaults, key, static_cast<int>(lod), document, s.damage);
    }
    const CarLevel &level = *models.levels[lod];
    bool ghost = car.GetGhostMode() == VEHICLE_GHOST;
    renderer.SetGhost(ghost);
    CarDamage damage{};
    std::ranges::copy(car.GetDamageTextureLevels(), damage.begin());
    renderer.SetDamage(damage);
    const Lighting &carLighting = view.lighting;
    Matrix4 body = MultiplyMatrices(FrameMatrix(frame), ConsoleSpace());
    Reflection reflection =
        SkyReflection(s.daySky.get(), s.nightSky.get(), camera, car, g_aVehicles[cameraIndex]);
    // The textured parts, the underside, the highlight, and the painted body in turn.
    if (level.car) {
        level.car->SetVariant(CarLightVariant(car));
        renderer.DrawReflectingModel(*level.car, body, viewProjection, carLighting, reflection);
    }
    // The underside shows only from below the car.
    float below = frame.up.x * (frame.position.x - camera.position.x) +
                  frame.up.y * (frame.position.y - camera.position.y) +
                  frame.up.z * (frame.position.z - camera.position.z);
    if (level.underside && below > 0.0f) {
        renderer.DrawModel(*level.underside, body, viewProjection, carLighting, std::nullopt, true);
    }
    std::optional<Highlight> highlight =
        CarHighlight(car, vehicle, camera, s.dayHighlight.get(), s.nightHighlight.get());
    // The highlight draws only where the depth is equal, and a ghost does not write the depth.
    if (highlight && level.highlighted && !ghost) {
        renderer.DrawHighlight(*level.highlighted, FrameMatrix(frame), viewProjection, *highlight);
    }
    if (level.paint) {
        renderer.DrawReflectingModel(*level.paint, body, viewProjection, carLighting, reflection);
    }
    // RenderWheelModels(): each wheel at the matrix the race gives it, in snow the wheel of the
    // snow.
    const RenderModel &wheel = g_dwSnowMode && models.snowWheel ? *models.snowWheel : *models.wheel;
    for (const auto &rows : car.GetWheelMatrices()) {
        Matrix4 place{};
        std::memcpy(place.data(), rows, sizeof(place));
        renderer.DrawModel(wheel, place, viewProjection, carLighting, std::nullopt, true);
    }
    renderer.SetGhost(false);
    renderer.SetDamage({});
}

} // namespace coastline
