<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.1/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Added

- `run-script` reads standard input when the filename is omitted or `-`, waiting for EOF before
  running and reporting script errors with `<stdin>:line` diagnostics.
- Installed manual pages for `coastline`, `coastline-import`, `coastline-import-pak`, and
  `coastline-run-script`.
- A saved GPU preference option selects automatic, high-performance, or low-power device selection.
- Cupra, Helmet, and Beach Ball paint and decal editing through numbered texture layers, preserving
  imported images beneath paint, with clear and undo support and tall textures fitted to the canvas.
  Bonus-car saves use `controller-pak/custom-decals` without consuming console capacity; original
  decal notes and older Milk Truck saves remain readable.
- Milk Truck paint and decal editing, with custom decals restored for races. Missing panels and
  incompatible mirrors are disabled; a separate Milk Truck save note preserves existing decal notes.
- A PC CRT shader simulates a progressive VGA monitor with faint scanlines, a fine shadow mask,
  and mild curvature.
- Saved Graphics preset choices include Custom, N64, Boss Rally, Ultra, Worst, and Low power.
  Manual graphics changes select Custom. Presets preserve language and region;
  changes apply as Options closes, restarting at logos when resource or pacing changes require it.
- `--gpu-preset n64` and `--gpu-preset br` select temporary N64 and Boss Rally looks, with explicit
  asset-pack and display arguments taking precedence.
- An Aspect ratio option offers Fill screen (default), 4:3, 16:9, 21:9, and 32:9, applying as Options
  closes. Internal frames expand to the ratio while preserving selected minimum dimensions and at
  least 320x240 pixels. Fill screen follows window resizing; explicit ratios display bars as needed.
- Edge-adaptive spatial upscaling and Consumer CRT, PVM, and Aperture grille screen presets, saved
  through Options and applied as Options closes.
- Two separate Escape presses within half a second quit from the main menu.
- PJ, TR, and MN cars are selectable outside Championship in USA and EUR regions, with Japanese
  menu statistics and matching race identities. Previews prefer imported Japanese models and fall
  back to PC models; Pure PC always uses PC previews.
- A Visual effects option saves `Game.Effects` as `PC` (default) or `N64`, applying as Options closes.
  N64 effects use imported smoke and wheel-spray PNGs, ordered particle opacity, and depth writes;
  persistent skid marks remain exclusive to PC mode. Older assets require ROM reimport for effect
  images; missing images produce a warning and fall back to PC effects.
- A Sound effects option saves `Game.SoundEffects` as `PC` (default) or `N64`. Changes reinitialise
  game resources and return to the logos after Options closes.
  N64 sounds preserve duration and are normalised against PC levels with a 98% peak limit. Missing
  N64 samples fall back to PC with a warning; taunts without console equivalents are silent.
  Menu feedback retains distinct native scroll, selection, and adjustment sounds in both modes.
- A saved `TitleScreen` option selects `regional` (default), `international`, or `japanese`
  independently of language and model region. Changes reinitialise game resources and return to the
  logos after Options closes. Available imported titles
  support custom-pack overrides; unavailable selections warn and fall back to the regional opening.
  Regional and title packs also resolve from user fallback directories over system base assets.
- A Draw distance option from 1x to 4x extends race visibility, including split-screen and
  rear-view mirrors. `Video.DrawDistance` defaults to `1`; changes apply as Options closes.
- Auto-save race progress is enabled by default and saves the next Championship checkpoint,
  records, and improved time-attack ghosts after races. Its On/Off option saves to INI and applies
  immediately. Manual saves remain available when disabled, and failed automatic saves are reported.
- Nine Options menu settings for asset packs, regional models and textures, N64 or PC CD music,
  anti-aliasing, anisotropic filtering, texture filtering, field of view, internal resolution scale,
  and FXAA, with spinning gold coin placeholders. Changes save immediately and apply as Options
  closes, reinitialising resources when required. Failed saves display an error and revert the menu
  adjustment.
- All available cars and drivable tracks are selectable outside Championship, including PC Race and
  Bonus tracks and their mirrors. Extra track records use separate INI settings and preserve the
  native Controller Pak layout.
- AppStream metadata for Linux software catalogues, with game details and asset import requirements.
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
- `--reset` removes generated `default`, `boss`, `eur`, and `jpn` packs and asset caches, preserving
  saves, decals, settings, and custom packs. `--factory-reset` deletes all configuration, data,
  state, and cache, including an explicit INI file. Both resets start the game or setup and accept
  `y` or `n` without case sensitivity; `--yes-really` or `-y` skips confirmation.
- `--config INI` reads and saves settings in an explicit INI file.
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

- Boss Rally graphics presets select PC CD music for menus and races. `--gpu-preset br` applies
  the music choice for the current run without saving it and locks Music in Options.
- N64 and Boss Rally graphics presets select 4:3. Low power requests a lower-power GPU with
  a 640x480 internal base and a 60 FPS limit, separately from Worst's unfiltered minimum quality.
- Ultra replaces High quality and `--gpu-preset best`; saved preset identifiers migrate from
  `best` to `ultra`.
  Command-line choice values accept unique prefixes.
- Options displays active command-line overrides and locks their settings for the current run.
- Full performance statistics use a larger font without changing the legacy FPS counter.
- Full performance statistics draw at output resolution after upscaling and CRT filters, using
  the full window including aspect-ratio bars. Captures include statistics at window resolution;
  disabling statistics releases their texture. Single-F FPS retains its legacy gradient and size,
  rendered at internal resolution with game filters.
- Upscaling labels nearest scaling as Off to clarify that enlargement applies without smoothing or
  edge reconstruction; existing `nearest` settings remain compatible.
- Outdated, incompatible, or incomplete imports open a graphical reimport prompt with Start setup
  and Quit choices. Quitting preserves existing assets; new installations open file setup directly.
- Each player's selected camera persists across races and game restarts, excluding automatic finish
  and replay camera changes.
- Statistics use shorter RAM and VRAM labels.
- SDL logs use action and status messages with race music filenames.
- Successful automatic saves skip post-race save prompts, returning Championship to track selection
  and time attack to the main menu. Disabled or failed automatic saves retain manual prompts.
- Timing statistics update once a second with average CPU work, frame interval, and FPS, retaining
  the highest CPU work and interval and lowest one-second FPS since statistics were enabled.
- Title screen choices use USA/Europe and Japan labels without duplicate regional choices; existing
  `regional` settings still follow the selected model region.
- Setup displays the repository URL, revision or version, and ISO 8601 UTC build date and time in
  its footer. The URL is underlined on hover and opens the default browser when clicked. The Done
  screen no longer advertises Escape as Back.
- Gamewin is excluded from regular track selection and remains available for the after-season race.
- Pure PC car previews use PC models and textures, including Type CE, and apply custom decals to
  the selected preview model. Command-line asset-pack selection also determines preview models.
- Internal resolution offers explicit sizes from 320x240 through connected display modes, defaulting
  to Window size and applying as Options closes. Resource-changing options identify game restarts.
- Graphics presets override saved internal resolution, render scale, and post-process anti-aliasing
  for consistent results, with 4x draw distance for `ultra` and 1x for `worst`.
- Setup disables Back on the Done screen after a successful import.
- Setup search results abbreviate the home directory as `~` in disc and ROM paths.
- Gold coin placeholders sit above the menu stage with soft projected shadows.
- Extra tracks without preview models show an empty stage with labels above.
- Setup uses classic text-mode Windows Setup colours, retaining red invalid ROM fields. Search and
  import progress bars centre percentages with contrasting text across the filled and empty areas.
- Zero BGM or SFX volume suspends the corresponding sequencing, decoding, and mixing, with
  independent resumption. Muting both pauses the native audio device.
- Setup asset search defaults to `~/Downloads` and preserves an edited search directory on retries.
- Settings apply after Options fades to black, with status text during the transition. Changes
  requiring new resources reinitialise the game in the same process and window, returning to logos.
- PC CD music selection includes menu track 2, with BGM volume control for menus and races. Missing
  CD audio falls back to XM with a warning. Native N64 XM music remains the default.
- Sparse `eur` and `jpn` packs store regional differences, layered between base assets and the
  selected pack. Region selection is independent of language and fonts. `--region` selects a region
  for one run; saved `Game.Region` defaults to the imported base. Strict layout validation requires
  reimporting older assets; reimport preserves mods.
- Imported byte-identical assets are stored once, with runtime paths reconstructed in cache before
  mod overrides are applied.
- CLI and setup imports require a disc and at least one ROM in a staged batch, using USA as the base
  when supplied or the first ROM otherwise. Successful imports replace generated packs and preserve
  mods; failed or cancelled conversions preserve previous assets.
- Skid-mark rendering reuses persistent GPU ring slots and uploads only changed geometry while
  preserving draw order.

### Fixed

- Performance statistics start below the lap-count HUD band, with clearance adjusted for window
  height and display scaling.
- Scripted button, key, and mouse durations avoid integer overflow near maximum frame numbers.
- Restarting a race from the pause menu restores each player's selected camera.
- Command-line validation reports errors instead of aborting for missing, empty, or
  excess option values, window dimensions outside 1..16384, unsupported anti-aliasing levels, and
  malformed aspect ratios.
- Setup starts with independent rendering and default pacing, ignoring gameplay graphics settings
  while preserving window and graphics-device selection. Ctrl+U still toggles unlocked pacing.
- Full performance statistics follow changes to window display scaling.
- Saved painted car images respect each texture's dimensions when palettes are shared, preserving
  unpainted images and matching output from GLB and separate glTF assets. Existing assets require
  reimporting disc and ROM images.
- Restored missing surfaces in mirrored Jungle, Coastline, and Desert track-selection previews.
- Japanese PJ, TR, and MN menu selections start races with the matching cars instead of RS, M3,
  and Helmet counterparts.
- Fixed Milk Truck front-right fender and fascia colours by importing every texture sharing a
  paintable palette. Existing assets require reimporting the disc and ROM images.
- Fixed PC CD music playing too fast by preserving decoded FLAC samples between audio callbacks.
- `PlaySFX=0` silences menu, intro, and race sound effects independently of music.
- Music playback respects saved BGM volume before starting and `PlayMusic=0` in menus and races,
  while sound effects remain independent.
- F12 advances N64 music modules, and restarting a race preserves the manually selected module.
- Fixed a crash when loading ghost frames from a path occupied by a directory.
- Fixed a crash when pausing or resuming CD music without an available audio device.
- Fixed inconsistent audio and crashes in sound-effect mixing after recreating the Nintendo 64 music
  player by resetting mixer positions and clearing the audio buffer.
- Patched builds zero-pad unused season save bytes instead of reading beyond the profile, with
  checksums calculated from the written payload.
- SDL builds reject `TGR_ENABLE_PATCHES=OFF` during configuration.
- Restored Japanese bitmap title display from a converted PNG without producer or title models.
  European and Japanese intros use original logo and title timings and copyright text and placement.
- Fixed a crash when quitting the game during a race by releasing race music before SDL audio
  shutdown.
- Restored Japanese paint-shop dialogue dimensions, text sizes, alignment, and button positions for
  decal loading, saving, overwrite confirmation, and Controller Pak errors.

## [0.0.1] - 2026-00-00

First version.

[unreleased]: https://github.com/Tatsh/coastline/compare/v0.0.0...HEAD
[0.0.1]: https://github.com/Tatsh/coastline/releases/tag/v0.0.0
