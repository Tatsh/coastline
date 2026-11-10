#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#include <SDL3/SDL.h>

#include "paths.h"
#include "scenerenderer.h"
#include "videoplayer.h"

namespace coastline {

struct VideoPlayer::State {
    std::unique_ptr<RenderTexture> texture;
    AVPlayer *player = nil;
    AVPlayerItemVideoOutput *output = nil;
    std::vector<std::uint8_t> rgba;
    bool paused = false;
    bool started = false;
    double duration = 0;

    void UploadFrame() {
        if (!paused) {
            texture->Update(rgba);
            return;
        }
        auto grey = rgba;
        for (std::size_t index = 0; index < grey.size(); index += 4) {
            const auto shade = static_cast<std::uint8_t>(
                (77 * grey[index] + 150 * grey[index + 1] + 29 * grey[index + 2]) / 512);
            grey[index] = grey[index + 1] = grey[index + 2] = shade;
        }
        texture->Update(grey);
    }
};

VideoPlayer::VideoPlayer(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

VideoPlayer::~VideoPlayer() = default;

std::unique_ptr<VideoPlayer> VideoPlayer::Open(SceneRenderer &renderer,
                                               const std::filesystem::path &path) {
    @autoreleasepool {
        NSURL *url =
            [NSURL fileURLWithPath:[NSString stringWithUTF8String:Utf8FromPath(path).c_str()]];
        AVURLAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
        if (!asset || !asset.isPlayable) {
            return nullptr;
        }
        NSArray<AVAssetTrack *> *tracks = [asset tracksWithMediaType:AVMediaTypeVideo];
        if (tracks.count == 0) {
            return nullptr;
        }
        const CGSize size = tracks.firstObject.naturalSize;
        const int width = static_cast<int>(std::abs(size.width));
        const int height = static_cast<int>(std::abs(size.height));
        if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
            return nullptr;
        }
        auto state = std::make_unique<State>();
        state->rgba.resize(static_cast<std::size_t>(width) * height * 4);
        state->texture =
            RenderTexture::FromTexels(renderer, width, height, state->rgba, true, false);
        if (!state->texture) {
            return nullptr;
        }
        NSDictionary *attributes =
            @{(id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};
        state->output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:attributes];
        AVPlayerItem *item = [AVPlayerItem playerItemWithAsset:asset];
        [item addOutput:state->output];
        state->player = [AVPlayer playerWithPlayerItem:item];
        state->duration = CMTimeGetSeconds(asset.duration);
        if (!std::isfinite(state->duration) || state->duration < 0) {
            state->duration = 0;
        }
        SDL_Log("Playing intro movie %s.", Utf8FromPath(path.filename()).c_str());
        return std::unique_ptr<VideoPlayer>(new VideoPlayer(std::move(state)));
    }
}

bool VideoPlayer::Update(double seconds, float volume) {
    @autoreleasepool {
        State &state = *m_state;
        if (!std::isfinite(seconds) || seconds < 0) {
            return true;
        }
        if (state.player.currentItem.status == AVPlayerItemStatusFailed) {
            return false;
        }
        if (state.duration <= 0 &&
            state.player.currentItem.status == AVPlayerItemStatusReadyToPlay) {
            state.duration = CMTimeGetSeconds(state.player.currentItem.duration);
            if (!std::isfinite(state.duration) || state.duration < 0) {
                state.duration = 0;
            }
        }
        state.player.volume = std::clamp(volume, 0.0f, 1.0f);
        if (!state.started) {
            state.started = true;
            [state.player play];
        }
        const CMTime time = state.player.currentTime;
        if ([state.output hasNewPixelBufferForItemTime:time]) {
            CVPixelBufferRef buffer = [state.output copyPixelBufferForItemTime:time
                                                            itemTimeForDisplay:nullptr];
            if (buffer) {
                if (CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) !=
                    kCVReturnSuccess) {
                    CVPixelBufferRelease(buffer);
                    return false;
                }
                const auto *source =
                    static_cast<const std::uint8_t *>(CVPixelBufferGetBaseAddress(buffer));
                if (!source) {
                    CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
                    CVPixelBufferRelease(buffer);
                    return false;
                }
                const auto stride = CVPixelBufferGetBytesPerRow(buffer);
                const int width = state.texture->Width();
                const int height = state.texture->Height();
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        const auto *pixel = source + y * stride + x * 4;
                        auto *rgba = state.rgba.data() + (y * width + x) * 4;
                        rgba[0] = pixel[2];
                        rgba[1] = pixel[1];
                        rgba[2] = pixel[0];
                        rgba[3] = 255;
                    }
                }
                CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(buffer);
                state.UploadFrame();
            }
        }
        return !state.duration || seconds < state.duration;
    }
}

std::optional<double> VideoPlayer::Seek(double seconds) {
    if (!std::isfinite(seconds)) {
        return std::nullopt;
    }
    const double duration = Duration();
    seconds = std::clamp(
        seconds, 0.0, duration > 0 ? std::clamp(duration - 0.05, 0.0, 86400.0) : 86400.0);
    const CMTime time = CMTimeMakeWithSeconds(seconds, 600);
    [m_state->player seekToTime:time toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
    return seconds;
}

void VideoPlayer::SetPaused(bool paused) {
    State &state = *m_state;
    state.paused = paused;
    state.UploadFrame();
    if (paused) {
        [state.player pause];
    } else if (state.started) {
        [state.player play];
    }
}

double VideoPlayer::Duration() const {
    return m_state->duration;
}

const RenderTexture *VideoPlayer::Texture() const {
    return m_state->texture.get();
}

} // namespace coastline
