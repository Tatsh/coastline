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
| `eur.z64` |      819 |  819 | 100.0% |
| `jpn.z64` |      822 |  822 | 100.0% |

### Matching

A routine of a release matches when IDO 5.3 compiles its source for that release
(the game at `-O2`, libultra at `-O1`, `-O2`, or `-O3`) to the words of the
original, with the fields that relocations fill masked. Assembly sources are
assembled and compared the same way. A routine whose words differ only in the
numbers of registers is listed apart (🟡). It does not match, and its source still
differs from the original. A routine the scan does not reach is unknown.
Run the matching scan under `.wiswa-ci/tools` for each release (`--region us`, `eu`,
and `jp`) before this report to regenerate the table.

| Release   | Routines | Matching | Registers only | Not matching | Unknown | Share |
| --------- | -------: | -------: | -------------: | -----------: | ------: | ----: |
| `usa.z64` |      801 |      633 |              7 |          161 |       0 | 79.0% |
| `eur.z64` |      819 |      650 |              6 |          163 |       0 | 79.4% |
| `jpn.z64` |      822 |      651 |              5 |          166 |       0 | 79.2% |

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
| Functions in the programs                                                                          | 2,166 |
| Excluded as C runtime (MSVC runtime library)                                                       |   333 |
| Excluded as Compiler-generated (static initialisers, atexit registrations, and exception funclets) |   115 |
| Excluded as Thunk (import or jump thunk the linker generates)                                      |    73 |
| To reconstruct                                                                                     | 1,645 |
| Done                                                                                               |   888 |
| Stubbed (not counted as done)                                                                      |     4 |
| Declared only (not counted as done)                                                                |    42 |
| Share done                                                                                         | 54.0% |
| Only in BRGlide                                                                                    |    52 |

### By release

A routine counts for a release when its address in that release is tagged.

| Release       | Routines | Done | Share |
| ------------- | -------: | ---: | ----: |
| `BRD3D.dll`   |    1,593 |  885 | 55.6% |
| `BRGlide.dll` |    1,446 |  484 | 33.5% |

### By component

| Component | Routines | Done | Stubbed | Share |
| --------- | -------: | ---: | ------: | ----: |
| Game code |    1,644 |  888 |       4 | 54.0% |
| zlib      |        1 |    0 |       0 |  0.0% |

## All releases

A routine counts for a release when its address in that release is tagged. A routine of the
PC port is shared with the N64 when an N64 release has a routine of the same name, ignoring
the class of a method (`Vehicle::RespawnVehicle` pairs with `RespawnVehicle`). A shared
routine is done for the PC when the source of the N64 routine, or a PC source, tags its
address in the PC libraries.

| Release       | Group       | Routines | Done |  Share |
| ------------- | ----------- | -------: | ---: | -----: |
| `usa.z64`     | Nintendo 64 |      801 |  801 | 100.0% |
| `eur.z64`     | Nintendo 64 |      819 |  819 | 100.0% |
| `jpn.z64`     | Nintendo 64 |      822 |  822 | 100.0% |
| `BRD3D.dll`   | PC port     |    1,593 |  885 |  55.6% |
| `BRGlide.dll` | PC port     |    1,446 |  484 |  33.5% |

| Measure                                         | Count |
| ----------------------------------------------- | ----: |
| Routines of the PC port shared with the N64     |   355 |
| Shared and done in every N64 release            |   355 |
| Shared and done in every release of both groups |    67 |
| Shared, done on the N64, and not yet on the PC  |   288 |

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
| [pc-game-code-10000000.md](progress/pc-game-code-10000000.md)         | PC port     | Game code      | `0x10000000`-`0x10010000` |      246 |  192 |  78.0% |
| [pc-zlib-10000000.md](progress/pc-zlib-10000000.md)                   | PC port     | zlib           | `0x10000000`-`0x10010000` |        1 |    0 |   0.0% |
| [pc-game-code-10010000.md](progress/pc-game-code-10010000.md)         | PC port     | Game code      | `0x10010000`-`0x10020000` |      161 |  103 |  64.0% |
| [pc-game-code-10020000.md](progress/pc-game-code-10020000.md)         | PC port     | Game code      | `0x10020000`-`0x10030000` |      176 |   95 |  54.0% |
| [pc-game-code-10030000.md](progress/pc-game-code-10030000.md)         | PC port     | Game code      | `0x10030000`-`0x10040000` |      307 |  117 |  38.1% |
| [pc-game-code-10040000.md](progress/pc-game-code-10040000.md)         | PC port     | Game code      | `0x10040000`-`0x10050000` |      264 |   84 |  31.8% |
| [pc-game-code-10050000.md](progress/pc-game-code-10050000.md)         | PC port     | Game code      | `0x10050000`-`0x10060000` |       85 |   29 |  34.1% |
| [pc-game-code-10060000.md](progress/pc-game-code-10060000.md)         | PC port     | Game code      | `0x10060000`-`0x10070000` |      170 |   62 |  36.5% |
| [pc-game-code-10070000.md](progress/pc-game-code-10070000.md)         | PC port     | Game code      | `0x10070000`-`0x10080000` |      232 |  205 |  88.4% |
| [pc-game-code-10080000.md](progress/pc-game-code-10080000.md)         | PC port     | Game code      | `0x10080000`-`0x10090000` |        3 |    1 |  33.3% |
