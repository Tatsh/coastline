<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.1/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Added

- The `sdl` platform and preset: a stand-alone 64-bit game built with SDL 3 from vcpkg. It
  draws through the SDL GPU API (Vulkan, Metal, or Direct3D 12), stores its files in the XDG
  directories (or the equivalent folders on Windows and macOS), reads `BossRally.ini`, binds
  gamepads and the keyboard to controller ports without configuration, and installs with
  `cmake --install`.
- `coastline import --cd` copies the game data of a Boss Rally CUE sheet or ISO image and
  converts its CD audio to FLAC. `coastline import --rom` copies a Top Gear Rally cartridge image
  after checking its SHA-1.
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

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/coastline/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/coastline/releases/tag/v0.0.0
