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
- `coastline --import-disc` copies the game data of a Boss Rally CUE sheet or ISO image and
  converts its CD audio to FLAC. `coastline --import-rom` copies a Top Gear Rally cartridge image
  after checking its SHA-1.
- The SDL game plays the music of the Nintendo 64 release with the console's player and mixers.

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/coastline/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/coastline/releases/tag/v0.0.0
