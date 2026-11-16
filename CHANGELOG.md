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
- Removed file writes and reads from the 3D car preview of Paint Shop. About four times a second
  while painting, the game encoded each car image as PNG, wrote the images and a `model.gltf` to
  `paint-shop` in the cache directory, and read and decoded the files again. The game now builds
  the glTF document in memory, with each PNG in a buffer view checked by decoding it. Paint Shop
  no longer writes `paint-shop` in the cache directory.
- Renamed the unlockable GAMEWIN course of track selection to Season Champion.

### Fixed

- Fixed the Season Champion code of the main menu (Up, Up, Down, Down, Left, Right, Left, Right, L,
  and R) on keyboards without a numeric keypad. The code read directions from the directional pad alone,
  and the keyboard mapped the pad only to the numeric keypad. The code now also accepts the analogue
  stick and the arrow keys. On the keyboard, A and S enter L and R.
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
- Fixed `cmake --install` of builds with pinned dependencies, including the Flatpak, AppImage, snap,
  and system installations, installing the static library, pkg-config file, CMake package files,
  and headers of FLAC and FLAC++ into the prefix (such as `/app/lib/libFLAC.a` and
  `/app/include/FLAC`). Pinned dependency releases are excluded from the default build, and their
  install rules do not run. The build requires CMake 3.28 or later.
- Stopped FFmpeg from opening a console window on Windows while it converts the intro movie
  during import.
- Stopped races from stalling when a player changed the camera. The game saved `BossRally.ini`
  and flushed the file and its directory to disk on the frame of the change. It now saves the
  camera choice when the race ends.
- Removed a periodic frame-time spike on Linux while the full statistics overlay is shown. The
  game read `/proc/self/smaps` for `GAME RAM` once a second on the main thread, and each read took
  several milliseconds. The NVML sampling thread now also samples game memory, and the thread runs
  whenever the overlay is shown.
- Removed a stall on every screen, races included, when F9, Print Screen, or a script saves a
  frame. The game encoded and wrote PNG files on the main thread after waiting for the GPU,
  taking tens to hundreds of milliseconds at high resolutions. The game now copies the frame into
  memory, and a thread encodes and writes the file and logs `Saved frame`. Up to four saves are
  pending at once, and a fifth waits for the oldest. Pending saves finish before the window is
  destroyed, and a script that quits after a screenshot writes complete files.
- Removed a stall at the switch to the results screen when `Auto-save race progress` is on. The
  game wrote the season note, the track records in `BossRally.ini`, and, in Time Attack, the ghost
  note and `ghost-frames.bin` on the main thread, and flushed each file and its directory to disk.
  A thread now writes the files, and the results screen shows the auto-save warning when the
  thread reports a failure. Leaving the results screen or opening the save menu waits for the
  automatic save, and a manual save is never overwritten by an older automatic one. Track records
  that fail to save remain in the settings and are saved again at exit.
- Removed a stall on the car select screen when a player changes the paint colour or look of a
  car, including every second step while Up or Down is pressed. The game read the glTF document
  and buffers of the car again, parsed the JSON, decoded every PNG, and uploaded the vertices and
  images to the GPU. The game now reads the document of each car once and paints and uploads only
  the material images for a new colour. The document retains the textures of each colour and is
  read again after 16 colours.
- Removed a stall of about 16 ms on each settings change in Options (every Left or Right press,
  SFX volume, and Units), on the controller preset screen, at a car choice, and at a track-record
  reset. The game wrote `BossRally.ini` on the main thread and flushed the file and its directory
  to disk. A thread now writes settings, and a newer copy replaces a copy not yet written. A
  button pressed down writes only the latest settings. The race camera and the automatic save
  after a race also write settings through the thread. Options shows
  `Settings could not be saved.` when the last write failed. A failed write no longer restores the
  previous setting in memory; the setting remains and is written again at the next save or at exit.
  A manual save of records from the save menu still waits for the write and reports the result.
- Removed a freeze of the setup window when a disc or ROM path is typed, pasted, or browsed, and
  when the Files screen opens. The wizard checked each path on the main thread. A ROM check read
  the whole image (8 to 64 MB), and a disc check of an ISZ or PDI image decoded the whole disc
  (about 650 MB) to a temporary file, taking seconds to tens of seconds. A thread now checks each
  path, and the field shows `Checking...` with **Next** disabled until the check finishes. A newer
  path replaces a pending check. After a directory search, the focus moves to the disc field when
  the search found no disc, to the first ROM field when it found no base ROM, and otherwise to
  **Next**.
- Removed a possible gap in the music with `Music=CD` when the disk responds slowly. The FLAC
  decoder read the file and opened the next track inside the audio callback of SDL, on the thread
  of the audio device. A thread now decodes the CD audio and retains about half a second queued in
  the SDL stream, decoding a tenth of a second at a time. A track change (F12 or a new race) clears
  the queue, and the new track starts at once. Pausing and a BGM volume of zero still pause the
  audio device.
- Shortened the loading of tracks, cars, menu models, and splash models. The game read and decoded
  the PNG images of a glTF model one at a time on the main thread. On the jungle track with the
  asset files not cached, the images took 97 ms of a 134 ms track load (41 ms of 78 ms when cached).
  Up to eight threads now read and decode the images of a model, and the main thread creates and
  uploads the textures in order. The images now take about 30 ms with the files not cached and 22
  to 28 ms when cached. The race view loads in 165 to 198 ms with the files not cached (from 227 to
  251 ms) and in 123 to 157 ms when cached (RTX 4090, NVMe).

## [0.0.3] - 2026-11-11

### Fixed

- Corrected TYPE-ES driver-side door decals on load and after painting.
- Extended Paint Shop and Data Load/Save backgrounds across wide menus.
- Returned to Car Select after leaving Paint Shop, including patched N64 builds.

## [0.0.2] - 2026-11-10

First release.
