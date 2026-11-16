# Changelog

## [unreleased]

### Added

- Added a **Run Coastline** checkbox, selected by default, to the final page of the Windows
  installer.
- Added Direct3D 12 rendering on Windows. Windows builds compile the shaders to signed DXIL with
  DXC, and SDL selects Direct3D 12 before Vulkan. Set `GpuDriver = vulkan` to select Vulkan.

### Changed

- Removed the Microsoft Visual C++ runtime DLLs (`vcruntime140.dll`, `msvcp140.dll`, and related
  libraries) from MSVC Windows packages. Install the Microsoft Visual C++ Redistributable; the NSIS
  installer offers its download when the runtime is missing or older than the build toolset.

### Fixed

- Prevented the game from exiting at startup on Windows, and under Wine with vkd3d-proton, when
  Direct3D 12 is available. The game requested DXIL shaders it did not ship, SDL selected
  Direct3D 12, and shader creation failed with `GPU driver requires SPIR-V or MSL support`.
- Fixed the intro movie import on Windows 7 and under Wine, where it failed with
  `Cannot download temporary FFmpeg. Install FFmpeg in PATH and retry.` The download ran `curl.exe`
  and the extraction ran `tar.exe`, and neither program is present on those systems. Windows now
  downloads with WinHTTP and macOS with `NSURLSession`, and the game extracts the archive itself.
  Download errors now report the system error or the HTTP status.
- Updated the checksum of the temporary FFmpeg download on Apple Silicon macOS. The publisher
  replaced the archive with a rebuild of FFmpeg 9.0.2, and the download failed checksum
  verification.
- Stopped FFmpeg from opening a console window on Windows while it converts the intro movie
  during import.
- Stopped races from stalling when a player changed the camera. The game saved `BossRally.ini`
  and flushed the file and its directory to disk on the frame of the change. It now saves the
  camera choice when the race ends.
- Removed a periodic frame-time spike on Linux while the full statistics overlay is shown. The
  game read `/proc/self/smaps` for `GAME RAM` once a second on the main thread, and each read took
  several milliseconds. The NVML sampling thread now also samples game memory, and the thread runs
  whenever the overlay is shown.

## [0.0.3] - 2026-11-11

### Fixed

- Corrected TYPE-ES driver-side door decals on load and after painting.
- Extended Paint Shop and Data Load/Save backgrounds across wide menus.
- Returned to Car Select after leaving Paint Shop, including patched N64 builds.

## [0.0.2] - 2026-11-10

First release.
