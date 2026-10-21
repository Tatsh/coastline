# coastline

<!-- WISWA-GENERATED-README:START -->

[![C](https://img.shields.io/badge/C-00599C?logo=c)](<https://en.wikipedia.org/wiki/C_(programming_language)>)
[![GitHub tag (with filter)](https://img.shields.io/github/v/tag/Tatsh/coastline)](https://github.com/Tatsh/coastline/tags)
[![License](https://img.shields.io/github/license/Tatsh/coastline)](https://github.com/Tatsh/coastline/blob/master/LICENSE.txt)
[![GitHub commits since latest release (by SemVer including pre-releases)](https://img.shields.io/github/commits-since/Tatsh/coastline/v0.0.0/master)](https://github.com/Tatsh/coastline/compare/v0.0.0...master)
[![Dependabot](https://img.shields.io/badge/Dependabot-enabled-blue?logo=dependabot)](https://github.com/dependabot)
[![Stargazers](https://img.shields.io/github/stars/Tatsh/coastline?logo=github&style=flat)](https://github.com/Tatsh/coastline/stargazers)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/Tatsh/coastline/master.svg)](https://results.pre-commit.ci/latest/github/Tatsh/coastline/master)
[![CMake](https://img.shields.io/badge/CMake-6E6E6E?logo=cmake)](https://cmake.org/)
[![Prettier](https://img.shields.io/badge/Prettier-black?logo=prettier)](https://prettier.io/)

[![@Tatsh](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor=did%3Aplc%3Auq42idtvuccnmtl57nsucz72&query=%24.followersCount&label=Follow+%40Tatsh&logo=bluesky&style=social)](https://bsky.app/profile/Tatsh.bsky.social)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Tatsh-black?logo=buymeacoffee)](https://buymeacoffee.com/Tatsh)
[![Libera.Chat](https://img.shields.io/badge/Libera.Chat-Tatsh-black?logo=liberadotchat)](irc://irc.libera.chat/Tatsh)
[![Mastodon Follow](https://img.shields.io/mastodon/follow/109370961877277568?domain=hostux.social&style=social)](https://hostux.social/@Tatsh)
[![Patreon](https://img.shields.io/badge/Patreon-Tatsh2-F96854?logo=patreon)](https://www.patreon.com/Tatsh2)

<!-- WISWA-GENERATED-README:STOP -->

Reconstructed source of the Nintendo 64 game Top Gear Rally.

No copyrighted material is in this repository. You must provide a cartridge image of the game for
its assets. Building this source alone will not result in a playable game.

## Building

Each release is a variant: `n64-us`, `n64-jp`, and `n64-eu` for the Nintendo 64 releases, and
`win-d3d` and `win-glide` for the two Windows libraries. A workflow preset configures and builds
one variant. The cartridge image of the release is read from the `TGR_ROM` environment variable,
and its assets are extracted into the build directory on every build. The image is checked
against the SHA-1 of the unmodified release before anything is read from it.

```shell
TGR_ROM=/path/to/usa.z64 cmake --workflow --preset n64-us
```

The `all` preset builds every variant in one step. Each Nintendo 64 variant takes its image from
`TGR_ROM_US`, `TGR_ROM_JP`, or `TGR_ROM_EU`.

```shell
TGR_ROM_US=usa.z64 TGR_ROM_JP=jpn.z64 TGR_ROM_EU=eur.z64 cmake --workflow --preset all
```

The Nintendo 64 compiler is libdragon's GCC, found through `N64_INST`, and no other part of
libdragon is used. The Windows compiler is an i686 mingw-w64 GCC, found through `MINGW_ROOT` or
on the path. The Windows build is 32-bit only, and its `BRD3D.dll` and `BRGlide.dll` are meant
as drop-in replacements for the files of the installed game. Every routine compiles into an
archive. The final link is off until every routine has a body, and `TGR_LINK=ON` turns it on.

The host tools under `tools/` are built with the host compiler during the configure step. They
need argparse, spdlog, and zlib, from vcpkg, the system, or a pinned download.
