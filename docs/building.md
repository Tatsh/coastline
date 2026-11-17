# Building

The build presets are `tgr-us`, `tgr-asia`, `tgr-jp`, and `tgr-eu` for the Nintendo 64
releases, `boss-rally` for the PC game, and `coastline` for the SDL game.

## SDL game

The `sdl` platform builds `coastline`, a stand-alone 64-bit game for every operating system SDL 3
supports, from Windows 7 and macOS 10.15 on x86_64 and arm64. It draws through the GPU API of SDL
(Vulkan, Metal, or Direct3D 12). It builds with GCC, Clang, MSVC of Visual Studio 2022, and MinGW.

The `coastline` preset takes the libraries from vcpkg, found through `VCPKG_ROOT`.

```shell
VCPKG_ROOT=/path/to/vcpkg cmake --workflow --preset coastline
```

```shell
cmake --install build/coastline --prefix ~/.local
```

Installation includes four manual pages under `${CMAKE_INSTALL_MANDIR}/man1`. CMake inserts the
version and UTC date at configuration time; `SOURCE_DATE_EPOCH` supplies the date for reproducible
builds.
Use `-DCOASTLINE_MAN_DATE=YYYY-MM-DD` to supply an explicit manual-page date.

### Libraries

Without vcpkg, each library and tool comes from a pinned release that is downloaded and built with
the game. The `USE_SYSTEM_<NAME>` options take each from the system instead, without a
download: `USE_SYSTEM_SDL3`, `USE_SYSTEM_ARGPARSE`, `USE_SYSTEM_FMT`, `USE_SYSTEM_FLAC`,
`USE_SYSTEM_MESHOPTIMIZER`, `USE_SYSTEM_NLOHMANN_JSON`, `USE_SYSTEM_ZLIB`, `USE_SYSTEM_GLSLANG`,
`USE_SYSTEM_SPIRV_CROSS`, and `USE_SYSTEM_DXC`.
A cross build takes glslang and SPIRV-Cross from the system or vcpkg, as it cannot run the tools it
builds.
Windows builds compile the Direct3D 12 shaders with DXC. The pinned DXC release is a prebuilt
Windows (x64 and arm64) or Linux x86_64 program. Other hosts require `USE_SYSTEM_DXC=ON` with a
DXC that includes the DXIL signing library.

```shell
cmake -S . -B build/coastline -G Ninja -DTGR_PLATFORM=sdl -DUSE_SYSTEM_SDL3=ON -DUSE_SYSTEM_ZLIB=ON
```

On Linux, the build of SDL needs the development headers of ALSA, EGL, Wayland, xkbcommon, and
the X11 extensions (Xcursor, Xfixes, Xi, Xrandr, Xss, Xtst), and autoconf-archive for vcpkg.

`COASTLINE_VIDEO` enables playback and conversion of the intro movie, and
[Importing game data](import.md) lists its libraries.

### Installed assets

To build with assets for system installation, supply a Boss Rally disc image and at least one
Top Gear Rally ROM image. Multiple ROM paths use a semicolon-separated CMake list.

```shell
cmake --preset coastline -DCOASTLINE_INSTALL_ASSETS=ON \
  -DCOASTLINE_DISC_IMAGE='/path/to/Boss Rally.cue' \
  '-DCOASTLINE_ROM_IMAGES=/path/to/usa.z64;/path/to/jpn.z64' \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build/coastline
DESTDIR=/path/to/package cmake --install build/coastline
```

The `coastline_assets` target imports images into the build directory using the import CLI.
Installation copies the generated `assets` tree to `${CMAKE_INSTALL_DATADIR}/coastline`, normally
`share/coastline`. macOS bundles use `Coastline.app/Contents/Resources/coastline`. The original
disc and ROM images are not installed. The import does not create user configuration or logs.
Image changes, CUE track changes, and importer rebuilds trigger conversion again. Cross builds
require `COASTLINE_ASSET_IMPORTER` pointing to a host Coastline executable, or a configured
`CMAKE_CROSSCOMPILING_EMULATOR`.

With `COASTLINE_INSTALL_ASSETS=ON`, the setup wizard, `--reset`, `--factory-reset`, `--yes-really`,
and `-y` are omitted. Missing or outdated assets produce an error requiring installation of usable
data. Normal builds retain the wizard and reset options.

### Windows installer

On Windows, `cpack --config build/coastline/CPackConfig.cmake` builds an NSIS installer that
installs the game for the current user, with shortcuts and an option to add it to `PATH`.
Its final page starts the game unless **Run Coastline** is cleared.
MSVC builds require the
[Microsoft Visual C++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)
for the architecture of the game. Packages omit the runtime DLLs, and the installer offers a
download of the redistributable when the installed runtime is missing or older than the build
toolset.

### macOS bundle

On macOS the install is the application bundle `Coastline.app`, signed without an identity, with
every library the game links that macOS does not provide inside it. The build workflow uploads it
as a disk image. `open` passes the options of the command line after `--args` to the game. A game
started by `open` runs in `/`, where a relative path does not resolve. Give absolute paths.
`-W` waits for the import to finish, and `-n` starts a new instance, as `open` brings a running
game forward without passing it the options. The output does not appear in the terminal, and
`game.log` in `~/Library/Logs/Coastline` records the result.

```shell
open -n -W Coastline.app --args import --cd "$PWD/Boss Rally.cue" --rom "$PWD/usa.z64"
```

### Snap and Flatpak

The build workflow uploads a debug and a release build of the game for each platform, an AppImage
of each Linux release build, a snap for amd64 and arm64 built from `snapcraft.yaml`, and a
Flatpak bundle for x86_64 and aarch64 built from `sh.tat.coastline.yml`.

```shell
snapcraft pack
```

```shell
flatpak-builder --user --install build/flatpak sh.tat.coastline.yml
```

Both packages import disc and cartridge images from the home directory and from removable
media, and from nowhere else. The snap reads removable media once the interface is connected.
The snap stores its data in `~/snap/coastline/common`, and the Flatpak in
`~/.var/app/sh.tat.coastline`.

```shell
sudo snap connect coastline:removable-media
```

### Releases

A tag such as `v1.2.3` identifies the build. The game shows the tag in place of the commit, and the
build workflow attaches every package to a draft release of the tag. The release workflow
publishes the draft once every workflow of the commit succeeds and every package is attached.

### Feral GameMode

On Linux, Coastline requests Feral GameMode at startup when its client library and service are
available, and releases the request on exit. Missing clients or failed requests do not prevent
play. The config-only setting `GameMode=0` under `[Game]` in `BossRally.ini` disables the request;
the default is `1`. Internal game restarts preserve the registration. Other platforms ignore the
setting. GameMode is not requested by import or Controller Pak commands.

### Discord Rich Presence

Discord Rich Presence is optional. Build with `-DCOASTLINE_DISCORD=ON` and
`-DCOASTLINE_DISCORD_APPLICATION_ID=YOUR_APPLICATION_ID`. CI enables support; its application ID
defaults to empty until an ID is supplied. Empty IDs disable IPC and its worker thread.
Presence reports the intro, attract demos, menus, racing, and credits through the local Discord
client. Communication runs asynchronously, retries missing clients, and coalesces updates at
15-second intervals. Import and Controller Pak commands do not start presence.
Flatpak permits native and Flatpak Discord sockets. Strict Snap confinement can prevent access to
Discord IPC; presence remains inactive when the socket is inaccessible.

## Nintendo 64 releases

The ROM image of an N64 release is read from the `TGR_ROM` environment variable, and its assets are
extracted into the build directory on every build. The image is checked against the SHA-1 of the
unmodified release before anything is read from it. [Importing game data](import.md) lists the
SHA-1 of each release.

```shell
TGR_ROM=/path/to/usa.z64 cmake --workflow --preset tgr-us
```

The Nintendo 64 compiler is libdragon's GCC, found through `N64_INST`, and no other part of
libdragon is used. Only `-Os` is supported for libdragon-GCC ROM builds. CMake applies `-Os` after
build-type flags; changing the final optimisation level can exceed the ROM code area or cause
runtime faults.

Each Nintendo 64 release builds a cartridge image, such as `coastline-us.z64` in the build
directory, whenever `TGR_ROM` is set. Routines not yet reconstructed in C link from the assembly of
the original under `src/n64/asm`, generated by splat from the region's YAML under `tools/splat`.
A routine is removed from the assembly of a release when its C body compiles for the release.

Patched N64 builds use the original screen resolutions by default. Experimental Expansion Pak
support remains unavailable in supported builds.

## Matching

Set `-DENFORCE_MATCHING=ON` on an N64 build to verify the reconstructed code during every build.
Matching needs IDO 5.3 at `IDO_ROOT` and the libdragon cross toolchain at `N64_INST`. The verifier
compares instruction digests against `tools/matching/{us,asia,jp,eu}.json`, generated from verified
original ROM images. CI runs it in a separate job for each region, without storing ROM images.
`tools/ido_flags.txt` records alternative IDO flags where the first build choice does not match.
An IDO Release build with patches disabled also provides a `rom` target. It assembles the compiled
objects into `coastline-<region>-code-only.z64`, with non-code bytes set to zero, and checks the
regional SHA-256. Set `COASTLINE_REFERENCE_ROM` to a verified original ROM when configuring to
produce `coastline-<region>.z64` with the original header and assets. `coastline-<region>.z64`
must match the original ROM's SHA-1. These commands build the USA image.

```shell
cmake -S . -B .wiswa-ci/build/ido-us -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/n64-ido-toolchain.cmake -DTGR_REGION=us \
  -DCMAKE_BUILD_TYPE=Release -DTGR_ENABLE_PATCHES=OFF \
  -DCOASTLINE_REFERENCE_ROM=/path/to/usa.z64
cmake --build .wiswa-ci/build/ido-us --target rom
```

To regenerate a region's digest file from a verified ROM, run:

```shell
.wiswa-ci/build/ido-us/tools/check-matching/check-matching --source-root . --region us \
  --rom usa.z64 \
  > .wiswa-ci/us-matching.json
```

Review the result before replacing `tools/matching/us.json`. Pass `--encoder` with the built
`encode-text` tool for the Japanese ROM.

## Windows libraries

The Windows compiler is an i686 mingw-w64 GCC, found through `MINGW_ROOT` or on the path. The
Windows build is 32-bit only, and its `BRD3D.dll` and `BRGlide.dll` are meant as drop-in
replacements for the files of the installed game. Every routine compiles into an archive. The
`boss-rally` build produces both renderer DLLs, their launchers, and
`build/boss-rally/BossRally-Setup.exe`. The installer requests the original disc directory and
copies its game data into the original `SouthPeak Interactive/Boss Rally` installation layout.
Mount the disc image or use an extracted disc directory; the installer does not open image files.
The uninstaller retains the copied game data, including saves and paint files.

## Host tools

The host tools under `tools/` are built with the host compiler during the configure step. They
need argparse, nlohmann JSON, spdlog, and zlib, from vcpkg, the system, or a pinned download.

## Tests

The tests under `tests/` build the reconstructed C for the host with the host compiler and check
it against the branches and command words of the disassembly.

```shell
cmake -S . -B build/tests -G Ninja -DTGR_TESTS=ON
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

The test of zlib inflates the assets extracted from a cartridge image and compares them with the
extractor's output. It runs only when `TGR_TEST_ASSETS` identifies the asset directory of a build.

```shell
cmake -S . -B build/tests -G Ninja -DTGR_TESTS=ON \
  -DTGR_TEST_ASSETS="$PWD/build/tgr-us/assets"
cmake --build build/tests && ctest --test-dir build/tests --output-on-failure
```

The offsets of the structures the assembly reads depend on the 32-bit pointers of the console.
`tests/layout.c` is therefore compiled by the cross compiler as part of each Nintendo 64 build,
where a wrong offset fails the build.
