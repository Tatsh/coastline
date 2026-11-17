# Display

## Display keys

Alt+Enter and F11 switch between a window and the full screen on every screen of the game. F cycles
through three states on every screen, from the first at each start. The first does not show
statistics, and the second shows the frame rate at the bottom of the screen, as in the PC
release. The third adds lines at the top left with the time of the last frame and of the slowest
frame of the second before, the use of one core and of every core together by the game, its memory
(`GAME RAM`), the bytes of the textures and buffers the game has created on the GPU (`GAME VRAM`),
and the share of an NVIDIA GPU the game used, read from NVML when the driver has it. On Linux
`GAME RAM` excludes the shared libraries and the GPU driver, and on Windows and macOS it includes
the private memory of the driver, as Windows and macOS do not separate it. The fonts of the menus
have no percent sign, and `PCT` stands for it. In a race, F12 selects the next music track, and
F11 does not select the previous one as in the PC release. F9 or Print Screen saves the frame as a
PNG file to `screenshots` in the data directory (`~/.local/share/coastline/screenshots`), named by
the time it was taken. F9 works where the desktop takes Print Screen for its screenshots, as Plasma
does. V switches between waiting for the vertical blank and presenting at once on every screen,
except while a screen takes typed text, and saves the choice to `VSync`. From `mailbox`, V presents
at once. Ctrl+U toggles an unlocked frame rate, temporarily bypassing VSync and the software frame
limit. Toggling back restores the configured presentation mode and frame limit. The choice is saved
as `UnlockedFramerate`. Setup starts with independent rendering and default pacing, preserving
window and graphics-device selection. Ctrl+U can still toggle unlocked pacing during setup.

## Statistics

Statistics separate `CPU WORK` (frame work excluding presentation waits and the software limiter)
from `INTERVAL` (measured time between frames, including pacing, displayed to four decimal places).
CPU work is CPU-side elapsed time, not GPU execution time. FPS remains the measured frame rate.

Single-F FPS retains the PC counter's gradient and size, rendered at internal resolution and
affected by scaling and screen shaders. Full statistics draw at output resolution after scaling
and screen shaders, adapting to window display scaling. Statistics can use the full window,
including aspect-ratio bars. Disabling statistics releases their output texture. Screenshots with
statistics include the full output at window resolution.

## GPU selection

`--gpu auto|high-performance|low-power` overrides `GpuPreference` for one run. Automatic selection
retains SDL's default preference for a dedicated GPU. Low-power selection prefers an
integrated GPU when available. The log records the actual adapter, backend, driver, and preference.
NVML loads only for an identified NVIDIA adapter and samples only a unique matching device.
Non-NVIDIA and unknown adapters omit NVML utilisation statistics; game memory counters remain.

## Presets

Options offers persistent Preset choices. Selecting a preset saves its settings in
`BossRally.ini`; subsequent manual graphics adjustments select Custom. N64 and Boss Rally select
their respective music and sound effects; other presets preserve both audio choices. Language and
region remain independent. N64 and Boss Rally select 4:3; Ultra fills the screen. Worst and Low
power preserve the aspect ratio.

| Preset     | Minimum internal resolution | Filtering and effects                                                               | Screen shader | Frame limit     |
| ---------- | --------------------------- | ----------------------------------------------------------------------------------- | ------------- | --------------- |
| N64        | 320x240                     | N64 filtering, console effects, original branding, and no anti-aliasing             | Consumer CRT  | 30 FPS          |
| Boss Rally | 640x480                     | Bilinear PC textures, PC effects, and Pure PC branding                              | PC CRT        | 60 FPS          |
| Ultra      | Window size                 | Modern filtering, 8x anti-aliasing, 16x anisotropic filtering, and 2x draw distance | Off           | Display refresh |
| Worst      | 320x240                     | Nearest textures and scaling, no anti-aliasing, and 1x draw distance                | Off           | 30 FPS          |
| Low power  | 640x480                     | Modern textures, 2x anti-aliasing, 2x anisotropic filtering, and 1x draw distance   | Off           | 60 FPS          |

Preset changes apply after exiting Options. Asset-pack, renderer, or frame-limit changes restart
the game at the logos; other preset adjustments apply during the black transition screen.
`--preset n64` and `--preset br` select the corresponding look for one run without saving
the preset. Explicit asset-pack and display arguments override the preset.

N64 and Boss Rally use 100% field of view and 1x draw distance. Anti-aliasing, anisotropic
filtering, post-process anti-aliasing, and upscaling are off. Boss Rally uses Modern texture
filtering without mipmaps, matching the original bilinear filtering.
Both presets preserve the title screen, auto-save, and region for models and textures.
N64 selects N64 window reflections, and Boss Rally selects PC window reflections.

`--preset ultra` fills the screen at window resolution with 2x draw distance, 100% field of
view, 8-sample anti-aliasing, alpha to coverage, 16-sample anisotropic filtering, modern texture
filtering, and PC visual effects. Upscaling, post-process anti-aliasing, and screen shaders are off.
Ultra preserves the title screen, auto-save, sound effects, music, models and textures, and asset
pack.
Ultra uses wide menu backgrounds only while the internal frame is 16:9. Other aspect ratios use
the normal menu background framing. `WideMenus=auto` under `[Video]` selects the 16:9 behaviour;
`1` fills all wide frames, and `0` disables wide menus.
Ultra uses borderless full screen unless overridden. `--preset worst` uses 1x draw distance,
nearest scaling and texture filtering, no anti-aliasing or screen filters, and a 320 by 240 base
frame at 30 FPS. Worst preserves the aspect ratio and does not change GPU preference.
`--preset low-power` requests SDL's
lower-power adapter, when available, and uses the moderate settings above. Other explicit display
arguments still apply. Unique choice prefixes are accepted, such as `n`, `u`, and `l`; ambiguous
prefixes produce an argument error.

CLI overrides disable matching Options entries and grey their coins. Their labels show the active
override, and the temporary values are not saved. For example, `--aspect-ratio 4:3` disables only
Aspect ratio, while `--preset n64` disables all settings controlled by the preset. GPU preference
changes recreate the GPU device and restart the game after Options closes.

## Video settings

The `[Video]` section of `BossRally.ini` sets up the display.

| Key                    | Values                                                                                                                                                                                                                                    |
| ---------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Fullscreen`           | `1` for the full screen, or `0` for a resizable window. Alt+Enter and F11 save their choice here.                                                                                                                                         |
| `FullscreenMode`       | `borderless`, the default, covers the display at its desktop mode. `exclusive` switches the display to `Width` by `Height` at `RefreshRate`.                                                                                              |
| `RefreshRate`          | The refresh rate of the exclusive mode in hertz, or `0` for the rate of the desktop.                                                                                                                                                      |
| `VSync`                | `vsync` or `1` waits for the vertical blank. `mailbox` presents without tearing and without waiting, and `immediate` or `0` presents at once. V saves its choice here.                                                                    |
| `GpuDriver`            | The GPU driver of SDL, `vulkan`, `metal`, or `direct3d12`, or empty, the default, for the driver SDL chooses.                                                                                                                             |
| `GpuPreference`        | `auto` (default), `high-performance`, or `low-power`. `--gpu` overrides the setting for one run.                                                                                                                                          |
| `UnlockedFramerate`    | `1` bypasses presentation pacing and `FrameRateLimit`; `0` restores configured pacing. Ctrl+U toggles and saves this choice.                                                                                                              |
| `FrameRateLimit`       | The most frames drawn each second, or `0` for no limit.                                                                                                                                                                                   |
| `InternalWidth`        | The width of the frame the game draws in pixels, or `0`, the default, for the width of the window.                                                                                                                                        |
| `InternalHeight`       | The height of the frame the game draws in pixels, or `0`, the default, for the height of the window.                                                                                                                                      |
| `RenderScale`          | The pixels of the frame for each pixel of the window, from `0.25` to `4`, when the frame follows the window. The default is `1`.                                                                                                          |
| `ScaleFilter`          | `linear`, the default, or `nearest`, how the frame is scaled to the window.                                                                                                                                                               |
| `AspectRatio`          | `auto`, the default, for the shape of the window, or a shape the frame retains in a window and in the full screen with bars around it: a ratio such as `4:3` or `16:10`, or a number such as `1.6`. `--aspect-ratio` sets it for one run. |
| `FieldOfView`          | The multiplier of the field of view of a race, `1` for that of the PC release.                                                                                                                                                            |
| `DrawDistance`         | The race draw-distance multiplier, from `1` (default) to `4`, also applied to split views and mirrors. The Options menu offers 1x, 1.5x, 2x, 3x, and 4x. Applies as Options closes.                                                       |
| `Antialias`            | The samples of each pixel the scene is drawn with, `2`, `4`, or `8` for multisample anti-aliasing, or `1`, the default, for none.                                                                                                         |
| `AlphaToCoverage`      | `1`, the default, smooths the edges of the cut texels of trees and fences by the share of the samples of a pixel their alpha covers, with `Antialias` above `1`. `0` cuts them.                                                           |
| `TextureFilter`        | `modern`, the default, filters textures bilinearly as the PC release does. `n64` filters three texels as the RDP of the Nintendo 64 does. `nearest` takes the nearest texel for a blocky look, the menus and the HUD included.            |
| `AnisotropicFiltering` | `2`, `4`, `8`, or `16` mipmaps the textures of the tracks and the cars and filters them with that many samples, with the `modern` filter. `0`, the default, does neither.                                                                 |
| `AntialiasPost`        | `fxaa` blends the finished frame along the edges it finds with fast approximate anti-aliasing. `none`, the default, does not.                                                                                                             |

## Resolution and scaling

By default the frame follows the size of the window in pixels, and changes with it as the window
is resized or covers the display. Options lists explicit internal resolutions from 320x240 through
the highest resolution reported by connected displays, plus a default `Window size` choice.
The list includes common intermediate sizes and reported display modes, with each size listed once.
Selecting a resolution resets `RenderScale` to `1` and applies as Options closes.
`InternalWidth` and `InternalHeight` set a size in place of the size of the window and of
`RenderScale`, and the frame is scaled to
the window with bars where its shape differs. `Width` and `Height` set the size of the window and
of the exclusive mode. A script that saves frames draws them at `Width` by `Height` times
`RenderScale`, for the same captures on every machine. A `RenderScale` of `2` draws four pixels for
each pixel of the window and filters them down (supersampling).

The frame passes through the anti-aliasing of `AntialiasPost` and then the scale of `ScaleFilter`
on its way to the window.

## Screen shaders

Options also offers edge-adaptive upscaling and four screen shaders. Set `ScaleFilter` to
`edge-adaptive` to reconstruct detail along detected edges when enlarging a lower internal
resolution. Set `ScreenShader` to `consumer-crt`, `pvm`, `aperture-grille`, or `pc-crt` for CRT
simulation, or `none` (default) to disable screen shaders. Consumer CRT adds curved corners, softer
pixels, and a staggered phosphor mask. PVM has stronger scanlines and a finer mask.
Aperture grille has a flat image and vertical RGB stripes. Scanlines fade near native resolution
to reduce aliasing.
PC CRT simulates a sharper progressive VGA monitor with a fine staggered shadow mask, faint
scanlines, and mild curvature.
Both settings apply as Options closes. Screenshots include enabled presentation filters at the
displayed resolution.

For a 1990s consumer CRT look, choose 320x240 internal resolution, Upscaling Off, and Consumer CRT.
Upscaling Off uses nearest-neighbour enlargement (`ScaleFilter=nearest`) without smoothing or edge
reconstruction. The CRT shader still applies its softness, scanlines, curvature, and phosphor mask.

## Aspect ratio and field of view

Aspect ratio defaults to Fill screen (`AspectRatio=auto`), matching the current window or display.
Options also offers 4:3, 16:9, 21:9, and 32:9. A selected internal resolution supplies minimum
dimensions; the frame expands across or down to match the ratio, with at least 320x240 pixels.
For example, 320x240 becomes 427x240 at 16:9, 560x240 at 21:9, or 854x240 at 32:9. Fill screen
recalculates the dimensions when the window changes size. Explicit ratios display bars when the
window has another shape. Changes apply as Options closes.

A race retains the vertical field of view of the PC release on a frame wider than 4:3 and widens
across, and retains the field across of 4:3 on a narrower frame. Neither field exceeds 120 degrees.
The rear-view mirror retains its shape and its size against the height of the view. The menus
retain their 4:3 layout in the middle of the frame, with bars at the sides or at the top and bottom.

## Vertical sync

A mode of `VSync` the GPU driver does not support falls back to the other mode that does not wait,
then to `vsync`. `--no-vsync` selects `immediate`. Under Wayland, `vsync` presents to the mailbox
and paces the frames at the refresh rate of the display, as waiting for the vertical blank there
drops frames on some drivers. `fifo` waits for the vertical blank under Wayland too. The races run
at 30 steps a second and draw between the steps, and the menus move by the time between frames, at
any frame rate.
