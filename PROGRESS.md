# Progress

Reconstruction status of Top Gear Rally. The three Nintendo 64 releases form one group,
keyed on the USA release, and the two PC port libraries form another, keyed on the Direct3D
library. A routine is done when a source file under `src` or `3rdparty` tags its address, for example
`@ghidraAddress USA: 0x80234abc` in a Doxygen block or `// USA: 0x80234abc` on a definition.
A routine marked as a stub (an `@stub` tag in its Doxygen block) is shown with
🚧 and is not counted as done. `@stub{EUR JPN}` marks only the
releases it lists, for a routine whose code in those releases differs from the body the
tree has. A routine tagged in a header with no body in the tree yet is shown with
📝 and is not counted as done.

Routines the compiler or the linker supplies are counted and excluded. The N64 SDK
(libultra) and the IPL3 boot code are reconstructed and counted. zlib is imported under
`3rdparty` at the matching release, edited in place where the game differs, and counted. The
PIF ROM is the console boot ROM, is not part of the cartridge, and is excluded.

Regenerate after every merged change, with the Ghidra project open and the HTTP bridge
running:

```shell
python3 .wiswa-ci/progress/progress.py
```

## Work outline

1. Choose the N64 toolchain and add the CMake toolchain file.
2. Reconstruct the USA ROM, one subsystem directory under `src` at a time, in the order of
   the parts below (largest routines first).
3. Port each routine to the EUR and JPN releases, and reconstruct the routines only those
   releases have.
4. Reconstruct the PC port from the two libraries, reusing the N64 sources where the port
   retained them.

## Nintendo 64

The programs are `usa.z64`, `eur.z64`, `jpn.z64`.

| Measure                                                                           |  Count |
| --------------------------------------------------------------------------------- | -----: |
| Functions in the programs                                                         |    836 |
| Excluded as Exception vector (copied from the SDK exception preamble at run time) |      1 |
| Excluded as IPL3 boot code (copied from the cartridge by the ROM builder)         |      7 |
| Excluded as PIF ROM (console boot ROM, not part of the cartridge)                 |      5 |
| To reconstruct                                                                    |    823 |
| Done                                                                              |    823 |
| Stubbed (not counted as done)                                                     |      0 |
| Declared only (not counted as done)                                               |      0 |
| Share done                                                                        | 100.0% |
| Only in EUR                                                                       |      1 |
| Only in JPN                                                                       |      4 |

### By release

A routine counts for a release when its address in that release is tagged.

| Release   | Routines | Done |  Share |
| --------- | -------: | ---: | -----: |
| `usa.z64` |      801 |  801 | 100.0% |
| `eur.z64` |      819 |  732 |  89.4% |
| `jpn.z64` |      822 |  734 |  89.3% |

### Matching

A routine of a release matches when IDO 5.3 compiles its source for that release
(the game at `-O2`, libultra at `-O1`, `-O2`, or `-O3`) to the words of the
original, with the fields that relocations fill masked. Assembly sources are
assembled and compared the same way. A routine the scan does not reach is unknown.
Run the matching scan under `.wiswa-ci/tools` for each release (`--region us`, `eu`,
and `jp`) before this report to regenerate the table.

| Release   | Routines | Matching | Not matching | Unknown | Share |
| --------- | -------: | -------: | -----------: | ------: | ----: |
| `usa.z64` |      801 |      627 |          174 |       0 | 78.3% |
| `eur.z64` |      819 |      640 |          179 |       0 | 78.1% |
| `jpn.z64` |      822 |      640 |          182 |       0 | 77.9% |

### By component

| Component      | Routines | Done | Stubbed |  Share |
| -------------- | -------: | ---: | ------: | -----: |
| Game code      |      590 |  590 |       0 | 100.0% |
| SDK (libultra) |      208 |  208 |       0 | 100.0% |
| zlib           |       25 |   25 |       0 | 100.0% |

## PC port

The programs are `BRD3D.dll`, `BRGlide.dll`.

| Measure                                                                                            | Count |
| -------------------------------------------------------------------------------------------------- | ----: |
| Functions in the programs                                                                          | 2,160 |
| Excluded as C runtime (MSVC runtime library)                                                       |   333 |
| Excluded as Compiler-generated (static initialisers, atexit registrations, and exception funclets) |   114 |
| Excluded as Thunk (import or jump thunk the linker generates)                                      |    73 |
| To reconstruct                                                                                     | 1,640 |
| Done                                                                                               |     2 |
| Stubbed (not counted as done)                                                                      |     0 |
| Declared only (not counted as done)                                                                |    15 |
| Share done                                                                                         |  0.1% |
| Only in BRGlide                                                                                    |    47 |

### By release

A routine counts for a release when its address in that release is tagged.

| Release       | Routines | Done | Share |
| ------------- | -------: | ---: | ----: |
| `BRD3D.dll`   |    1,593 |    2 |  0.1% |
| `BRGlide.dll` |    1,445 |    2 |  0.1% |

### By component

| Component | Routines | Done | Stubbed | Share |
| --------- | -------: | ---: | ------: | ----: |
| Game code |    1,639 |    2 |       0 |  0.1% |
| zlib      |        1 |    0 |       0 |  0.0% |

## Parts

Each part lists the routines of one component in a 64 KiB address
block of the master program.

| Part                                                                  | Group       | Component      | Range                     | Routines | Done |  Share |
| --------------------------------------------------------------------- | ----------- | -------------- | ------------------------- | -------: | ---: | -----: |
| [n64-game-code-80200000.md](progress/n64-game-code-80200000.md)       | Nintendo 64 | Game code      | `0x80200000`-`0x80210000` |       45 |   45 | 100.0% |
| [n64-game-code-80210000.md](progress/n64-game-code-80210000.md)       | Nintendo 64 | Game code      | `0x80210000`-`0x80220000` |      134 |  134 | 100.0% |
| [n64-game-code-80220000.md](progress/n64-game-code-80220000.md)       | Nintendo 64 | Game code      | `0x80220000`-`0x80230000` |      169 |  169 | 100.0% |
| [n64-game-code-80230000.md](progress/n64-game-code-80230000.md)       | Nintendo 64 | Game code      | `0x80230000`-`0x80240000` |       45 |   45 | 100.0% |
| [n64-zlib-80230000.md](progress/n64-zlib-80230000.md)                 | Nintendo 64 | zlib           | `0x80230000`-`0x80240000` |        7 |    7 | 100.0% |
| [n64-game-code-80240000.md](progress/n64-game-code-80240000.md)       | Nintendo 64 | Game code      | `0x80240000`-`0x80250000` |       55 |   55 | 100.0% |
| [n64-zlib-80240000.md](progress/n64-zlib-80240000.md)                 | Nintendo 64 | zlib           | `0x80240000`-`0x80250000` |       18 |   18 | 100.0% |
| [n64-game-code-80250000.md](progress/n64-game-code-80250000.md)       | Nintendo 64 | Game code      | `0x80250000`-`0x80260000` |      140 |  140 | 100.0% |
| [n64-game-code-80260000.md](progress/n64-game-code-80260000.md)       | Nintendo 64 | Game code      | `0x80260000`-`0x80270000` |        2 |    2 | 100.0% |
| [n64-sdk-libultra-80260000.md](progress/n64-sdk-libultra-80260000.md) | Nintendo 64 | SDK (libultra) | `0x80260000`-`0x80270000` |      208 |  208 | 100.0% |
| [pc-game-code-10000000.md](progress/pc-game-code-10000000.md)         | PC port     | Game code      | `0x10000000`-`0x10010000` |      246 |    0 |   0.0% |
| [pc-zlib-10000000.md](progress/pc-zlib-10000000.md)                   | PC port     | zlib           | `0x10000000`-`0x10010000` |        1 |    0 |   0.0% |
| [pc-game-code-10010000.md](progress/pc-game-code-10010000.md)         | PC port     | Game code      | `0x10010000`-`0x10020000` |      158 |    0 |   0.0% |
| [pc-game-code-10020000.md](progress/pc-game-code-10020000.md)         | PC port     | Game code      | `0x10020000`-`0x10030000` |      176 |    0 |   0.0% |
| [pc-game-code-10030000.md](progress/pc-game-code-10030000.md)         | PC port     | Game code      | `0x10030000`-`0x10040000` |      307 |    0 |   0.0% |
| [pc-game-code-10040000.md](progress/pc-game-code-10040000.md)         | PC port     | Game code      | `0x10040000`-`0x10050000` |      263 |    0 |   0.0% |
| [pc-game-code-10050000.md](progress/pc-game-code-10050000.md)         | PC port     | Game code      | `0x10050000`-`0x10060000` |       84 |    0 |   0.0% |
| [pc-game-code-10060000.md](progress/pc-game-code-10060000.md)         | PC port     | Game code      | `0x10060000`-`0x10070000` |      169 |    0 |   0.0% |
| [pc-game-code-10070000.md](progress/pc-game-code-10070000.md)         | PC port     | Game code      | `0x10070000`-`0x10080000` |      233 |    1 |   0.4% |
| [pc-game-code-10080000.md](progress/pc-game-code-10080000.md)         | PC port     | Game code      | `0x10080000`-`0x10090000` |        3 |    1 |  33.3% |
