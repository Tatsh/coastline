# Data and saves

## Directories

The game reads `BossRally.ini` from `~/.config/coastline`, its data from `~/.local/share/coastline`,
and writes `game.log` to `~/.local/state/coastline`. The XDG Base Directory variables move the
configuration, data, and state directories. Windows uses `Coastline` in the roaming and local
application data folders. macOS uses `Coastline` in Application Support, Caches, and Logs in the
library of the user.

Before user assets, the game searches the installation data directory, including relocated
`bin/../share/coastline`, `/usr/local/share/coastline`, and `/usr/share/coastline` on Unix.
macOS also searches bundle resources and `/Library/Application Support/Coastline`. MSYS2 and
Windows use the data directory relative to the executable and the configured installation prefix.
The fallback is the user's data directory, such as `~/.local/share/coastline` on Linux or
`~/Library/Application Support/Coastline/data` on macOS. Every data root uses `assets/default`
and sibling asset packs. System packs take priority; a user mod can layer over system defaults
in the user cache. Configurations, saves, screenshots, and cache remain in user directories.
`--data-dir` explicitly selects a data root and bypasses system asset lookup.

`--config INI` reads and saves settings in the specified INI file instead of the default
`BossRally.ini`. Relative paths resolve from the launch directory. Data, cache, state, and controller
bindings use their normal directories. Combining `--factory-reset` with `--config` also deletes the
specified INI file.

The first release uses version 1 of the imported asset formats. Pre-release installations require
`coastline --factory-reset`; settings and data are not migrated.

## Portable installations

Portable file lookup defaults to enabled, except in installed-asset builds, and is enabled in CI
packages. Coastline checks for `BossRally.ini` and `assets` beside the executable, then in
`CoastlineData` under the launch working directory, then in the working directory itself.
Configuration and assets are selected independently. On macOS, adjacent files
reside in `CoastlineData` beside `Coastline.app`, outside the bundle. AppImages use `CoastlineData`
beside the original AppImage, rather than its temporary mount. Local assets precede system assets.
Cache and state reside under the local asset root, or the local configuration root when assets use
platform data.
Absent local files use platform directories; unwritable selected locations produce an error.

The portable-installation checkbox of [setup](first-run.md) is unchecked for new installations.
Selecting it imports assets and saves settings beside the application distribution, with local
cache and state directories. Explicit `--config` or `--data-dir` overrides disable the checkbox.
Moving a portable installation preserves setup paths stored relative to its directory. Configure
with `-DCOASTLINE_PORTABLE=OFF` to disable portable lookup and installation.

Use `--portable` (`-P`) or `COASTLINE_PORTABLE=1` to force adjacent storage without existing files.
`COASTLINE_PORTABLE=0` disables automatic local lookup, and `--portable` overrides the environment
setting. The environment also accepts `true`/`false`, `yes`/`no`, and `on`/`off`, ignoring case.
Explicit `--config` and `--data-dir` paths take precedence for configuration and assets; cache and
state remain local when portable mode is forced.

Distributions with a `bin` directory use the complete package directory for portable storage.
Windows ZIP packages include `Coastline.lnk` beside `bin`, with a relative target of
`bin\coastline.exe`. Move the complete directory to relocate a portable installation.

Use `coastline migrate-portable [DIRECTORY]` to move portable assets, settings, saves, screenshots,
and state into normal platform directories. `DIRECTORY` can be `CoastlineData` or its parent.
Omit `DIRECTORY` to use automatic portable lookup.
Add `--copy` to preserve the portable installation. Existing destination entries are never
overwritten, and copying failures preserve the source. Cache is regenerated. After copying, use
`COASTLINE_PORTABLE=0` to select platform storage while portable files remain adjacent.

Use `coastline make-portable [--copy] [DIRECTORY]` to export normal platform assets, settings,
saves, screenshots, and state. Windows and macOS create `CoastlinePortable` in the launch working
directory by default. Windows exports the installed `bin`, library, and share directories with data
at the package root. macOS exports the application bundle with an adjacent `CoastlineData` directory.
Source program files are copied and preserved. Linux exports data only, defaulting to
`CoastlineData` in the working directory. The executable may reside in a read-only installation.
Without `--copy`, source data is removed after publishing the completed export. Move Linux exports
beside an AppImage, or launch another portable-enabled build from the exported data directory.

## Saves and the Controller Pak

The saves of the game are the notes of a Controller Pak, a file for each note in `controller-pak`
of the data directory (`~/.local/share/coastline/controller-pak`), with the frames of the ghost
car in `ghost-frames.bin` beside it. The note of the options menu, `TOP GEAR OPTIONS`, is a setting
and is stored in `controller-pak` of the configuration directory
(`~/.config/coastline/controller-pak`), and the pak counts it with the other notes.
Internal note files and ghost-pose sidecars have a versioned integrity header, payload length, and
checksum. Header and payload are replaced atomically under a process lock. Loading a damaged save
attempts to remove the invalid file and uses defaults. Controller Pak capacity counts native note
bytes, excluding the integrity header. Pre-release save formats are not migrated.
`export-pak` writes them to a Controller Pak image (`.mpk`) for a console or an emulator, with the
codes of the release `--release` chooses: `usa` (the default), `eur`, or `jpn`. Each release
reads only the notes with its codes.

```shell
coastline export-pak --release eur pak.mpk
```

`import-pak` reads the notes of any release from an image.

```shell
coastline import-pak pak.mpk
```

## Region

`Region` in `[Game]` selects `usa`, `eur`, or `jpn`, defaulting to the imported base. `--region`
selects a region for a single run. An unavailable region falls back to the base. Assets are layered
in the order `default`, the regional difference pack, and the selected `AssetPack` (`boss` or a mod).
Each optional pack falls back directly to `default`; packs do not declare inherited packs.

```ini
[Game]
Region = jpn
AssetPack = boss
Language = en-us
```

## Resets

`coastline --reset` removes generated asset packs (`default`, `boss`, `eur`, and `jpn`) and asset
caches before starting setup again. Saves, decals, settings, and custom asset packs are preserved.
With `--data-dir`, reset clears generated packs in the selected data directory.

`coastline --factory-reset` deletes all Coastline configuration, imported assets, mods, saved games,
decals, cache, and state before starting again. Both reset options request terminal confirmation
with `y` or `n`, without case sensitivity. Add `--yes-really` or `-y` to skip confirmation.
