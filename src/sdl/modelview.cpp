#include "modelview.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <SDL3/SDL.h>

#include "gpu.h"
#include "scenerenderer.h"

namespace coastline {

struct ModelView::State {
    std::unique_ptr<SceneRenderer> renderer;
    std::unique_ptr<RenderModel> model;
    float yaw = 0.8f;
    float pitch = 0.5f;
    float distance = 2.0f;
};

ModelView::ModelView(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

ModelView::~ModelView() = default;

std::unique_ptr<ModelView> ModelView::Create(GpuWindow &window, const std::filesystem::path &path) {
    auto state = std::make_unique<State>();
    state->renderer = SceneRenderer::Create(window);
    if (!state->renderer) {
        return nullptr;
    }
    state->model = RenderModel::Load(*state->renderer, path);
    if (!state->model) {
        return nullptr;
    }
    state->distance = state->model->Radius() * 2.2f;
    return std::unique_ptr<ModelView>(new ModelView(std::move(state)));
}

void ModelView::Draw(const std::filesystem::path *capture) {
    State &s = *m_state;
    const bool *keys = SDL_GetKeyboardState(nullptr);
    float radius = s.model->Radius();
    auto centre = s.model->Centre();
    s.yaw += (keys[SDL_SCANCODE_RIGHT] ? 0.03f : 0.0f) - (keys[SDL_SCANCODE_LEFT] ? 0.03f : 0.0f);
    s.pitch += (keys[SDL_SCANCODE_UP] ? 0.02f : 0.0f) - (keys[SDL_SCANCODE_DOWN] ? 0.02f : 0.0f);
    s.pitch = std::clamp(s.pitch, -1.5f, 1.5f);
    float zoom = keys[SDL_SCANCODE_W] ? 0.98f : keys[SDL_SCANCODE_S] ? 1.02f : 1.0f;
    s.distance = std::clamp(s.distance * zoom, radius * 0.02f, radius * 10.0f);

    // A camera about the centre, y up, looking at it.
    std::array<float, 3> eye{centre[0] + s.distance * std::cos(s.pitch) * std::sin(s.yaw),
                             centre[1] + s.distance * std::sin(s.pitch),
                             centre[2] + s.distance * std::cos(s.pitch) * std::cos(s.yaw)};
    std::array<float, 3> forward{centre[0] - eye[0], centre[1] - eye[1], centre[2] - eye[2]};
    float length =
        std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    for (float &f : forward) {
        f /= length;
    }
    std::array<float, 3> right{forward[2], 0.0f, -forward[0]};
    float rightLength = std::sqrt(right[0] * right[0] + right[2] * right[2]);
    right = {-right[0] / rightLength, 0.0f, -right[2] / rightLength};
    std::array<float, 3> up{right[1] * forward[2] - right[2] * forward[1],
                            right[2] * forward[0] - right[0] * forward[2],
                            right[0] * forward[1] - right[1] * forward[0]};
    auto dot = [](const std::array<float, 3> &a, const std::array<float, 3> &b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };
    Matrix4 view{right[0],
                 up[0],
                 -forward[0],
                 0,
                 right[1],
                 up[1],
                 -forward[1],
                 0,
                 right[2],
                 up[2],
                 -forward[2],
                 0,
                 -dot(right, eye),
                 -dot(up, eye),
                 dot(forward, eye),
                 1};
    GpuWindow &window = s.renderer->Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    float nearPlane = radius * 0.005f;
    float farPlane = s.distance + radius * 3.0f;
    float focal = 1.0f / std::tan(0.5f * 1.0f);
    // Depth from 0 to 1, as Vulkan, Metal, and Direct3D 12 take it.
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
                       farPlane / (nearPlane - farPlane),
                       -1,
                       0,
                       0,
                       nearPlane * farPlane / (nearPlane - farPlane),
                       0};
    // A model with no lights of its own is lit as a clear day of the race lights it.
    Lighting lighting;
    lighting.direction = {0.5571f, 0.7428f, -0.3714f};
    lighting.ambient = {102.0f / 255.0f, 102.0f / 255.0f, 119.0f / 255.0f};
    lighting.directional = {1.0f, 1.0f, 204.0f / 255.0f};
    s.renderer->Begin({0.18f, 0.22f, 0.3f, 1.0f});
    s.renderer->DrawModel(*s.model,
                          IDENTITY_MATRIX,
                          MultiplyMatrices(projection, view),
                          lighting,
                          std::nullopt,
                          true);
    s.renderer->End(capture);
}

} // namespace coastline
