<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.1/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Added

- GLB models with embedded buffers and PNG images, plus data URI buffers and images. Paint-shop
  models retain model and paint metadata. Selected-pack models override defaults across formats;
  `.gltf` takes precedence within a pack, and an invalid `.gltf` reports an error instead of loading
  `.glb`.
- GPU selection preferences through `--gpu` and `GpuPreference`, with adapter and driver logging.
  NVML samples utilisation only for a uniquely matched active NVIDIA adapter.
- Ctrl+U toggles an unlocked frame rate in the game and setup, saving `UnlockedFramerate` and
  restoring configured presentation pacing and frame limits when disabled.
- Separate CPU work and frame interval statistics. CPU work excludes presentation waits and the
  software frame limiter; frame intervals include pacing and display four decimal places.
- An in-game QBASIC-style setup wizard with an embedded unscii font for missing, incomplete, or
  outdated game data. It validates disc and ROM images and imports data with progress and
  cancellation while controls remain responsive. Finish starts the game in the same process.
  - A single Files screen combines disc selection with three interchangeable ROM fields that detect
    each release. Recursive directory search displays discovered images and permits retries after
    partial results.
  - Search includes hidden files and folders and ROM entries in ZIP archives. ZIP entries pass CRC
    and ROM SHA-1 checks before extraction to user cache.
  - Clear buttons and text selection with Ctrl+A and Alt+Backspace for word deletion.
- Installed asset lookup checks system and relocated installation directories before usable user
  data. Saves and cache remain writeable in user directories, with fallback to user mod packs.
- `COASTLINE_INSTALL_ASSETS`, `COASTLINE_DISC_IMAGE`, and `COASTLINE_ROM_IMAGES` build and install
  imported game data. Builds with installed assets omit setup and all reset options.
- `--reset` deletes configuration, imported assets, mods, saved games, state, and cache before startup.
  Confirmation accepts `y` or `n` without case sensitivity; `--yes-really` or `-y` skips confirmation.
- `--config INI` reads and saves settings in an explicit INI file. Reset also deletes the specified
  INI file.
- Game startup with EUR or JPN cartridge data without USA cartridge data, using available regional
  menu assets and music and language fallback.
- The `sdl` platform and preset: a stand-alone 64-bit game built with SDL 3 from vcpkg. It
  draws through the SDL GPU API (Vulkan, Metal, or Direct3D 12), stores its files in the XDG
  directories (or the equivalent folders on Windows and macOS), reads `BossRally.ini`, binds
  gamepads and the keyboard to controller ports without configuration, and installs with
  `cmake --install`.
- `coastline import --cd` copies game data from a Boss Rally CUE sheet, ISO image, or Mode 1 BIN
  image and converts CD audio to FLAC. BIN import prefers a matching adjacent CUE sheet; standalone
  BIN import copies data without CD audio. `coastline import --rom` copies a Top Gear Rally
  cartridge image after checking its SHA-1. Import with explicit `--data-dir` avoids writes to user
  configuration and logs.
- The SDL game plays the music of the Nintendo 64 release with the console's player and mixers.
- Quality of life changes in the Windows libraries built with `TGR_ENABLE_PATCHES`:
  - The first press of the key that resets the car shows a hint, and a second press within half
    a second resets it.
  - Screens narrower than 4:3, such as 5:4, retain the horizontal field of view of 4:3 and lay
    the HUD out 640 units wide. Wider screens retain the vertical field of view as before.
  - `scene=` on the command line starts a race in place of the menus: `arcade`, `timeattack`,
    `practice`, `demo`, or `seasonwinner`, with `season=` for the season of the season winner
    race and the existing `chosenTrack=`, `chosenCar=`, and `chosenWeather=`. Saved data is not
    written until the game exits.
  - A season loaded from or saved to a numbered save is written to it after each race, and a
    time attack run that beats the ghost of the slot last loaded or saved replaces that ghost
    without the prompt.

### Changed

- Imported byte-identical assets are stored once, with runtime paths reconstructed in cache before
  mod overrides are applied.
- CLI and setup imports require a disc and at least one ROM in a staged batch, using USA as the base
  when supplied or the first ROM otherwise. Successful imports replace generated packs and preserve
  mods; failed or cancelled conversions preserve previous assets.
- Skid-mark rendering reuses persistent GPU ring slots and uploads only changed geometry while
  preserving draw order.

### Fixed

- Fixed a crash when quitting the game during a race by releasing race music before SDL audio
  shutdown.
- Restored Japanese paint-shop dialogue dimensions, text sizes, alignment, and button positions for
  decal loading, saving, overwrite confirmation, and Controller Pak errors.

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/coastline/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/coastline/releases/tag/v0.0.0
