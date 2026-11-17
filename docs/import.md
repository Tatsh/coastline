# Importing game data

The data comes from a Boss Rally disc image and a Top Gear Rally cartridge image. The
[setup wizard](first-run.md) runs the import on the first start, and the `import` subcommand runs
the same import from a terminal. The import checks the cartridge against the SHA-1 of each release,
and converts the CD audio of a CUE sheet to FLAC. An ISO image has no CD audio. The import writes
converted packs under `assets` in the data directory, and exits.

```shell
coastline import --cd 'Boss Rally.cue' --rom usa.z64 --rom jpn.z64
```

| Nintendo 64 release | ROM SHA-1                                  |
| ------------------- | ------------------------------------------ |
| USA                 | `bfc51f086ebdb78904dc24ad0c3a2d946e26d022` |
| Asia                | `7e36a98f6fafe3ff7e72d14e0fae16418458da47` |
| Europe              | `9fae5d98fe3464f00d59034690df90118646eb95` |
| Japan               | `e0415b87c5a5b69ac581e79137406bdaa79b354a` |

## Packs and regions

Supply the disc and every ROM image in the same command. A successful import replaces generated
packs (`default`, `boss`, `eur`, and `jpn`) and preserves mod packs. Failed or cancelled conversions
preserve the previous assets. USA is the base when supplied; otherwise the first ROM is the base.
`assets/default/pack.json` records the base in `baseRegion`. Every import creates `default` and `boss`.
Additional cartridges produce `eur` and `jpn` packs with only differences from the base. Cartridge
assets use `n64/base` within each pack. Fonts and strings from every imported language remain
available in `default` independently of the selected region.

Import generates simplified menu models for Race Track and Bonus Track, shared with their mirrors,
as `tracks/race-menu.gltf` and `tracks/bonus-menu.gltf`. Re-import older packs to generate menu models.
The conversion closes convex planar openings and adds sides beneath exposed terrain edges.

## Disc images

The `--cd` option takes a CUE sheet (`.cue`), an ISO image (`.iso`), a BIN image (`.bin`), CloneCD
metadata (`.ccd` with its `.img`), Alcohol metadata (`.mds` with its `.mdf` files), a Nero image
(`.nrg`), a DiscJuggler image (`.cdi`), or BlindWrite metadata (`.bwt`/`.b5t`/`.b6t` with its image
files), and a ROM image with each `--rom`. A BIN first uses an adjacent CUE sheet with the same
basename, ignoring case, or another adjacent sheet referencing the BIN. Without a CUE sheet, its
whole-file SHA-1 selects the track offsets of a verified dump when available. Otherwise, sector
sync and headers locate the data boundary, checked against the ISO volume size, and runs of digital
silence identify audio pregaps. The verified 13-track Boss Rally BIN has SHA-1
`391609a19eaa44cf470c0b8b2110122381a87f25`; its CUE sheet is identified by track layout,
without a CUE SHA-1. Audio discovery requires all 12 music tracks and sufficient track lengths;
uncertain layouts import data only. A complete set of `Track NN.bin` files also imports without a
CUE sheet. CloneCD uses its track metadata; `.sub` data is not required. Nero supports both
32-bit and 64-bit track metadata, recorded disc-at-once or track-at-once. Alcohol supports split
image files and UTF-16 filenames. Both formats preserve track boundaries and exclude pregaps;
interleaved subchannel bytes are discarded during conversion.

DiscJuggler supports versions 2, 3, and 3.5, stored pregaps, multiple sessions, and variable-length
index and CD-TEXT metadata. Its P-Q and P-W subchannels are discarded during conversion.
BlindWrite 5/6 supports split images, UTF-16 filenames, multiple sessions, stored pregaps, and
interleaved subchannels. Selecting `.b5i` or `.b6i` resolves the matching metadata file.
BlindWrite 4 uses `.bwt` metadata with `.bwi` data files. It supports separate track files and
stored pregaps; `.bws` subchannels are optional. Selecting `.bwi` or `.bws` resolves matching
`.bwt` metadata. Filenames resolve as UTF-8 or Windows-1252.

Unencrypted ISZ images support zero-filled, uncompressed, zlib, and bzip2 chunks, split segments,
and checksums. Decoding uses temporary storage until import finishes. ISO payloads provide data;
raw-sector payloads also use BIN track discovery for CD music.
Pinnacle InstantCopy PDI images support cooked data or raw data and audio, including scrambled data
and interleaved subchannels. PDI decoding requires temporary disk space for the headerless image.

The `--cd` option also accepts a directory, including a mounted physical disc or a copy of its
files. Select the disc root with `BossRally.pod` directly inside it. In setup, enter its path or
browse into the disc and select **Use this directory**. Recursive asset search also detects disc
directories. Mounted directories provide data files without CD audio; use CUE/BIN for CD music.

## Validation

Import validates required disc files before conversion and publishes generated packs after every
conversion succeeds. A completed inventory records file sizes, a file count, and an inventory
checksum. Startup rejects incomplete inventories and missing or truncated imported files, and
offers re-import or quit before loading game assets.

CD audio encoding and independent regional ROM conversions use up to four worker threads, limited
by available CPU concurrency. Cancellation stops pending work and waits for active conversions
before discarding staging files.

Each import records conversion versions in `assets/default/import.ini` and the paths and sizes
of generated files in `assets/default/import-files.txt`. Missing files, different sizes, old
conversion versions, an unsupported layout, or an incomplete import prevent the game from starting
and report the import command. Import old assets again.

## Intro movie

`COASTLINE_VIDEO=ON` enables intro movie playback; `OFF` excludes it. Linux uses libav for playback
and for converting the original Indeo movie to H.264/AAC MP4. With vcpkg, select manifest features
`sdl;video` to build libav and OpenH264. macOS and Windows play MP4 through AVFoundation and Media
Foundation without linking libav. During import, macOS and Windows use FFmpeg from `PATH`, or download
a temporary copy and delete it afterwards. Windows uses Gyan builds; Apple Silicon uses OSXExperts,
and Intel macOS uses Evermeet. Downloaded tools use FFmpeg 9.0.2 from versioned URLs and pinned
archive SHA-256 checksums.
