# Vendored sources

Each directory here is an upstream release, imported with the top directory name of its archive.
The game's changes are made in place and are listed below. The formatting and checking tools skip
this directory, and upstream code retains its original style.

| Directory     | Upstream archive                                   | SHA-256 of the archive                                             | Files imported            |
| ------------- | -------------------------------------------------- | ------------------------------------------------------------------ | ------------------------- |
| `zlib-1.0.4/` | <https://www.zlib.net/fossils/zlib-1.0.4.tar.gz> | `e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24` | 19 of the release's files |
| `glide-2f226f0f9225ce8ee83e6a4a7042981e719d19ee/` | <https://github.com/sezero/glide/archive/2f226f0f9225ce8ee83e6a4a7042981e719d19ee.tar.gz> | `bc57c4e51e6bd0a3c8f464cb693ca62dd5ad0bd9f8cca6d99933fdd702c1b1e9` | 6 of the archive's files |

## zlib 1.0.4

The game decompresses every asset with zlib's `uncompress()`, which calls `inflateInit_()` with
the version string `1.0.4`. The imported files are `README`, `ChangeLog`, the public headers
`zlib.h` and `zconf.h`, the inflate side (`inflate.c`, `infblock.c`, `infcodes.c`, `inffast.c`,
`inftrees.c`, `infutil.c` and their headers), `adler32.c`, `uncompr.c` for `uncompress()`, and
`zutil.c` with `zutil.h` for `zcalloc()` and `zcfree()`. The compression side, the gzip layer, and
the examples are omitted.

The game links `inflate.c`, `infblock.c`, `infcodes.c`, `inffast.c`, `inftrees.c`, `infutil.c`,
`adler32.c`, `uncompr.c`, and `zutil.c`. The routines the ROM has from zlib are listed under the zlib component
of [PROGRESS.md](../PROGRESS.md). Each definition carries the USA address of its routine.

### Game changes

- **The default allocator.** `inflateInit2_()` in `inflate.c` gives zlib a bump allocator over the
  heap the game sets in `g_pInflateHeapBase` and `g_pInflateHeapEnd`. Without that heap, it blanks
  the display and uses both frame buffers, then sets `g_dwScreenUnblankDelay` to 2.
- **The allocator itself.** `zcalloc()` in `zutil.c` returns 8-byte aligned memory that it does not
  clear, and stops the game with a message when the heap runs out. `zcfree()` does nothing.
- **The data.** The tables and state of `inftrees.c`, `infblock.c`, `infutil.c`, and `zutil.c` are
  the game's data in the assembly, which the headers declare. The game's tables hold the upstream
  values. The fixed tables of `inftrees.c` live in the game's BSS, which C may not extend.

The rest of the code is upstream. Its calls match the original routine by routine, and it inflates
every asset of the USA release to the same bytes as the host zlib.

## Glide 2.x

BRGlide.dll draws through the Glide 2.x API of 3dfx and imports 38 of its routines from
`glide2x.dll`. The archive is the open-source Glide of 3dfx, as kept in the sezero/glide fork of the
SourceForge repository. The imported files are the public headers of Glide 2.x for the Voodoo 2
(`glide2x/cvg/glide/src/glide.h`, `glidesys.h`, and `glideutl.h`, and `glide2x/cvg/incsrc/sst1vid.h`),
the `swlibs/fxmisc/3dfx.h` they include, and `glide2x/glide_license.txt`, the 3DFX GLIDE Source Code
General Public License they are distributed under. The headers are not changed. The import library
of `glide2x.dll` is built from `src/platform/win/glide/glide2x.def`.

## Verification

Download an archive, check its SHA-256, extract it, and compare each imported file. For example,
`cmp zlib-1.0.4/inflate.c 3rdparty/zlib-1.0.4/inflate.c` reports the first difference in that file.
