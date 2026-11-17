# Asset packs

Import deduplicates generated asset files. `asset-aliases.txt` records paths that share another
file, and the game recreates alias paths in its cache before applying packs. Mod packs use normal
files and directories; they do not need alias tables.
After import, empty subdirectories are removed from generated packs. On Unix, existing read and
execute permissions are preserved and all write bits are cleared; Windows files receive the
read-only attribute. Reset and re-import handle protected packs automatically. Mod packs retain
their permissions.

## Branding

The game draws the images of the PC release, except where the PC release removed the branding of
Nintendo, Top Gear, or Kemco. There, the default pack has the image of the cartridge, such as the
NINTENDO and TOP GEAR RALLY decals of the cars, the Kemco livery of the Japanese car `pj`, the TOP
GEAR RALLY banners of the desert, and the Nintendo banners of the season winner scene. The import
lists the branded images in `src/sdl/data/branded-textures.json`, and moves each PC image it
replaces to the pack `boss` in `assets`. A data directory imported before the packs existed takes
them once the disc and the cartridges are imported again.

## Selecting a pack

`AssetPack` in the `[Game]` section selects the pack, `default` or `boss` for the images of the PC
release, and `--asset-pack NAME` selects one for a single run. A mod pack is a separate directory
in `assets`, selected by its directory name. A file of the selected pack replaces the file of the
same path in `assets/default`, and every other file is read from `assets/default`. The game links
the files of both packs into `assets/NAME` in the cache directory on each start.

## Models

Models may use separate `.gltf`, buffer, and PNG files or a single `.glb` with embedded buffers
and PNG images. A reference to `model.gltf` also finds `model.glb`. When both formats exist for
the same model in a pack, `.gltf` takes precedence; an invalid `.gltf` reports an error. A model
in the selected pack replaces the default model regardless of format. Paint-shop models retain
their `coastlineModel` and `coastlinePaint` extras when consolidated.

## Pack settings

A pack may have a `pack.json` of settings. Each key of the selected pack replaces the key of
`assets/default/pack.json`. The import writes `assets/default/pack.json`.

```json
{
  "version": 1,
  "xmMusicGain": 1.5
}
```

`version` is the version of the file, 1. `xmMusicGain` is the gain of the music of the XM modules.

## Intro sequence

A pack can replace regional splash screens with an ordered `intro` array. Pure PC uses the
converted Boss Rally movie, followed by its legal image, then the main menu. Other packs retain
regional splash screens when `intro` is absent. An empty array opens the menu immediately.

```json
{
  "intro": [
    { "file": "intro/brally.mp4", "type": "video" },
    { "duration": 4, "file": "intro/legal.png", "type": "image" }
  ],
  "introOnce": true,
  "mainMenuAttractDemo": true,
  "mainMenuIdleSeconds": 90,
  "mainMenuTitle": "BOSS RALLY",
  "version": 1,
  "videoControls": {
    "progressBar": true,
    "progressBottom": [190, 0, 0],
    "progressTop": [255, 245, 0],
    "seekSeconds": 2
  }
}
```

Each entry has a `type` (`image`, `video`, or `model`) and a `file` relative to prepared assets.
Image and model entries have a positive `duration` in seconds (default 2, maximum 3600).
Video entries play to their end unless a positive `duration` limits playback.
`skippable` defaults to true; a skip button advances to the next entry. Static model entries
accept `scale` in (0, 1000] (default 0.03125) and `depth` in [-100000, -1) (default -50).
Missing media and unsupported videos are skipped.

During a movie, Left and Right seek backward and forward by `videoControls.seekSeconds`
(default 2, positive and at most 3600).
Space pauses or resumes playback. A paused movie is dimmed to greyscale with a central pause symbol.
Shift+Left restarts the current movie. `videoControls.progressBar` enables the bottom progress bar
(default true). `progressTop` and `progressBottom` specify its RGB gradient, yellow to red by default.

`introOnce` plays the sequence once per game restart (default false). `mainMenuTitle` replaces the
main-menu heading (default `TOP GEAR RALLY`). `mainMenuIdleSeconds` sets its inactivity timeout from
0 through 3600 seconds, with 0 disabling the timeout (default 20). `mainMenuAttractDemo` starts a
demonstration directly after the timeout (default false), returning to the menu afterwards.
Pure PC uses the settings above. N64 packs return to the title sequence after 20 seconds; an attract
demo can follow from the title sequence.
