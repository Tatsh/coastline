# Vendored sources

Each directory here is an upstream release, imported with the top directory name of its archive.
The game's changes are made in place and are listed below. The formatting and checking tools skip
this directory, and upstream code retains its original style.

| Directory     | Upstream archive                                   | SHA-256 of the archive                                             | Files imported            |
| ------------- | -------------------------------------------------- | ------------------------------------------------------------------ | ------------------------- |
| `zlib-1.0.4/` | <https://www.zlib.net/fossils/zlib-1.0.4.tar.gz> | `e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24` | 18 of the release's files |

## zlib 1.0.4

The game decompresses every asset with zlib, and its `DecompressBuffer()` calls `inflateInit_()`
with the version string `1.0.4`. The imported files are `README`, `ChangeLog`, the public headers
`zlib.h` and `zconf.h`, the inflate side (`inflate.c`, `infblock.c`, `infcodes.c`, `inffast.c`,
`inftrees.c`, `infutil.c` and their headers), `adler32.c`, and `zutil.c` with `zutil.h` for
`zcalloc()` and `zcfree()`. The compression side, the gzip layer, and the examples are omitted.

The game links `inflate.c`, `infblock.c`, `infcodes.c`, `inffast.c`, `inftrees.c`, `infutil.c`,
`adler32.c`, and `zutil.c`. The port does not change these files yet. The 27 routines the ROM has
from zlib are listed under the zlib component of [PROGRESS.md](../PROGRESS.md).

## Verification

Download an archive, check its SHA-256, extract it, and compare each imported file. For example,
`cmp zlib-1.0.4/inflate.c 3rdparty/zlib-1.0.4/inflate.c` reports the first difference in that file.
