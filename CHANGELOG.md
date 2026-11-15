# Changelog

## [unreleased]

### Added

- Added a **Run Coastline** checkbox, selected by default, to the final page of the Windows
  installer.

### Changed

- Removed the Microsoft Visual C++ runtime DLLs (`vcruntime140.dll`, `msvcp140.dll`, and related
  libraries) from MSVC Windows packages. Install the Microsoft Visual C++ Redistributable; the NSIS
  installer offers its download when the runtime is missing or older than the build toolset.

### Fixed

- Prevented the game from exiting at startup on Windows, and under Wine with vkd3d-proton, when
  Direct3D 12 is available. The game requested DXIL shaders it does not ship, SDL selected
  Direct3D 12, and shader creation failed with `GPU driver requires SPIR-V or MSL support`. Windows
  now uses Vulkan.

## [0.0.3] - 2026-11-11

### Fixed

- Corrected TYPE-ES driver-side door decals on load and after painting.
- Extended Paint Shop and Data Load/Save backgrounds across wide menus.
- Returned to Car Select after leaving Paint Shop, including patched N64 builds.

## [0.0.2] - 2026-11-10

First release.
