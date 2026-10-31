# Progress

Matching status of Top Gear Rally. A routine matches when the compiler of the original
compiles its source to the words of the original, with the fields that relocations fill
masked. The compiler is IDO 5.3 for the three Nintendo 64 releases (the game at `-O2`,
libultra at `-O1`, `-O2`, or `-O3`) and MSVC 6.0 for the two PC libraries. Assembly
sources are assembled and compared the same way. A routine whose words differ only in the
numbers of registers is listed apart (🟡). It does not match, and
its source still differs from the original. A routine the scan does not reach is unknown
(❔).

Routines the compiler or the linker supplies are excluded, as are zlib (open-source code
vendored under `3rdparty`) and the PIF ROM (the console boot ROM, not part of the
cartridge). The N64 SDK (libultra) and the IPL3 boot code are counted.

Run the matching scan under `.wiswa-ci/tools` for each release (`--region us`, `eu`, and
`jp`), then regenerate the report with the Ghidra project open and the HTTP bridge running.
The scan of the PC libraries with MSVC 6.0 does not exist yet, and their routines are
unknown until it writes `matching-brd3d.json` and `matching-brglide.json`.

```shell
python3 .wiswa-ci/progress/progress.py
```

## All releases

| Release       | Compiler | Routines | Matching | Registers only | Not matching | Unknown | Share |
| ------------- | -------- | -------: | -------: | -------------: | -----------: | ------: | ----: |
| `usa.z64`     | IDO 5.3  |      774 |      607 |              7 |          160 |       0 | 78.4% |
| `eur.z64`     | IDO 5.3  |      792 |      624 |              6 |          162 |       0 | 78.8% |
| `jpn.z64`     | IDO 5.3  |      795 |      625 |              5 |          165 |       0 | 78.6% |
| `BRD3D.dll`   | MSVC 6.0 |    1,573 |        0 |              0 |            0 |   1,573 |  0.0% |
| `BRGlide.dll` | MSVC 6.0 |    1,441 |        0 |              0 |            0 |   1,441 |  0.0% |

## Nintendo 64

The programs are `usa.z64`, `eur.z64`, `jpn.z64`, compiled
by IDO 5.3. A routine counts by the result of the first
program that has it.

| Measure                                                                           | Count |
| --------------------------------------------------------------------------------- | ----: |
| Functions in the programs                                                         |   836 |
| Excluded as Exception vector (copied from the SDK exception preamble at run time) |     1 |
| Excluded as IPL3 boot code (copied from the cartridge by the ROM builder)         |     7 |
| Excluded as PIF ROM (console boot ROM, not part of the cartridge)                 |     5 |
| Excluded as zlib (provided by open-source zlib, vendored when modified)           |    27 |
| To match                                                                          |   796 |
| Only in EUR                                                                       |     1 |
| Only in JPN                                                                       |     4 |

### By component

| Component      | Routines | Matching | Registers only | Not matching | Unknown |  Share |
| -------------- | -------: | -------: | -------------: | -----------: | ------: | -----: |
| Game code      |      588 |      419 |              7 |          162 |       0 |  71.3% |
| SDK (libultra) |      208 |      208 |              0 |            0 |       0 | 100.0% |

## PC port

The programs are `BRD3D.dll`, `BRGlide.dll`, compiled
by MSVC 6.0. A routine counts by the result of the first
program that has it.

| Measure                                                                                                                  | Count |
| ------------------------------------------------------------------------------------------------------------------------ | ----: |
| Functions in the programs                                                                                                | 2,155 |
| Excluded as C runtime (MSVC runtime library)                                                                             |   336 |
| Excluded as Compiler-generated (static initialisers, atexit registrations, deleting destructors, and exception funclets) |   131 |
| Excluded as Thunk (import or jump thunk the linker generates)                                                            |    73 |
| Excluded as zlib (provided by open-source zlib, vendored when modified)                                                  |     1 |
| To match                                                                                                                 | 1,614 |
| Only in BRGlide                                                                                                          |    41 |

### By component

| Component | Routines | Matching | Registers only | Not matching | Unknown | Share |
| --------- | -------: | -------: | -------------: | -----------: | ------: | ----: |
| Game code |    1,614 |        0 |              0 |            0 |   1,614 |  0.0% |

## Parts

Each part lists the routines of one component in a 64 KiB address
block of the master program.

| Part                                                                  | Group       | Component      | Range                     | Routines | Matching |  Share |
| --------------------------------------------------------------------- | ----------- | -------------- | ------------------------- | -------: | -------: | -----: |
| [n64-game-code-80200000.md](progress/n64-game-code-80200000.md)       | Nintendo 64 | Game code      | `0x80200000`-`0x80210000` |       45 |       34 |  75.6% |
| [n64-game-code-80210000.md](progress/n64-game-code-80210000.md)       | Nintendo 64 | Game code      | `0x80210000`-`0x80220000` |      134 |       98 |  73.1% |
| [n64-game-code-80220000.md](progress/n64-game-code-80220000.md)       | Nintendo 64 | Game code      | `0x80220000`-`0x80230000` |      169 |      136 |  80.5% |
| [n64-game-code-80230000.md](progress/n64-game-code-80230000.md)       | Nintendo 64 | Game code      | `0x80230000`-`0x80240000` |       45 |       20 |  44.4% |
| [n64-game-code-80240000.md](progress/n64-game-code-80240000.md)       | Nintendo 64 | Game code      | `0x80240000`-`0x80250000` |       53 |       31 |  58.5% |
| [n64-game-code-80250000.md](progress/n64-game-code-80250000.md)       | Nintendo 64 | Game code      | `0x80250000`-`0x80260000` |      140 |       99 |  70.7% |
| [n64-game-code-80260000.md](progress/n64-game-code-80260000.md)       | Nintendo 64 | Game code      | `0x80260000`-`0x80270000` |        2 |        1 |  50.0% |
| [n64-sdk-libultra-80260000.md](progress/n64-sdk-libultra-80260000.md) | Nintendo 64 | SDK (libultra) | `0x80260000`-`0x80270000` |      208 |      208 | 100.0% |
| [pc-game-code-10000000.md](progress/pc-game-code-10000000.md)         | PC port     | Game code      | `0x10000000`-`0x10010000` |      246 |        0 |   0.0% |
| [pc-game-code-10010000.md](progress/pc-game-code-10010000.md)         | PC port     | Game code      | `0x10010000`-`0x10020000` |      158 |        0 |   0.0% |
| [pc-game-code-10020000.md](progress/pc-game-code-10020000.md)         | PC port     | Game code      | `0x10020000`-`0x10030000` |      174 |        0 |   0.0% |
| [pc-game-code-10030000.md](progress/pc-game-code-10030000.md)         | PC port     | Game code      | `0x10030000`-`0x10040000` |      299 |        0 |   0.0% |
| [pc-game-code-10040000.md](progress/pc-game-code-10040000.md)         | PC port     | Game code      | `0x10040000`-`0x10050000` |      261 |        0 |   0.0% |
| [pc-game-code-10050000.md](progress/pc-game-code-10050000.md)         | PC port     | Game code      | `0x10050000`-`0x10060000` |       79 |        0 |   0.0% |
| [pc-game-code-10060000.md](progress/pc-game-code-10060000.md)         | PC port     | Game code      | `0x10060000`-`0x10070000` |      168 |        0 |   0.0% |
| [pc-game-code-10070000.md](progress/pc-game-code-10070000.md)         | PC port     | Game code      | `0x10070000`-`0x10080000` |      228 |        0 |   0.0% |
| [pc-game-code-10080000.md](progress/pc-game-code-10080000.md)         | PC port     | Game code      | `0x10080000`-`0x10090000` |        1 |        0 |   0.0% |
