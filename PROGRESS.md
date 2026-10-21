# Progress

Reconstruction status of Top Gear Rally. The three Nintendo 64 releases form one group,
keyed on the USA release, and the two PC port libraries form another, keyed on the Direct3D
library. A routine is done when a source file under `src` or `3rdparty` tags its address, for example
`@ghidraAddress USA: 0x80234abc` in a Doxygen block or `// USA: 0x80234abc` on a definition.
A routine marked as a stub (an `@stub` tag in its Doxygen block, or `(stub)` after its
address) is shown with :construction: and is not counted as done. A routine tagged in a
header with no body in the tree yet is shown with :memo: and is not counted as done.

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
3. Port each routine to the JPN and EUR releases, and reconstruct the routines only those
   releases have.
4. Reconstruct the PC port from the two libraries, reusing the N64 sources where the port
   retained them.

## Nintendo 64

The programs are `usa.z64`, `jpn.z64`, `eur.z64`.

| Measure                                                                           | Count |
| --------------------------------------------------------------------------------- | ----: |
| Functions in the programs                                                         |   860 |
| Excluded as Exception vector (copied from the SDK exception preamble at run time) |     1 |
| Excluded as PIF ROM (console boot ROM, not part of the cartridge)                 |     5 |
| To reconstruct                                                                    |   854 |
| Done                                                                              |   223 |
| Stubbed (not counted as done)                                                     |     0 |
| Declared only (not counted as done)                                               |    40 |
| Share done                                                                        | 26.1% |
| Only in JPN                                                                       |     6 |
| Only in EUR                                                                       |     2 |

### By component

| Component      | Routines | Done | Stubbed |  Share |
| -------------- | -------: | ---: | ------: | -----: |
| Game code      |      614 |   30 |       0 |   4.9% |
| IPL3 boot code |        7 |    0 |       0 |   0.0% |
| SDK (libultra) |      208 |  168 |       0 |  80.8% |
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

### By component

| Component | Routines | Done | Stubbed | Share |
| --------- | -------: | ---: | ------: | ----: |
| Game code |    1,639 |    2 |       0 |  0.1% |
| zlib      |        1 |    0 |       0 |  0.0% |

## Parts

Each part lists the routines of one component in a 64 KiB address
block of the master program.

| Part                                                                      | Group       | Component      | Range                     | Routines | Done |  Share |
| ------------------------------------------------------------------------- | ----------- | -------------- | ------------------------- | -------: | ---: | -----: |
| [n64-game-code-80200000.md](progress/n64-game-code-80200000.md)           | Nintendo 64 | Game code      | `0x80200000`-`0x80210000` |       47 |    6 |  12.8% |
| [n64-game-code-80210000.md](progress/n64-game-code-80210000.md)           | Nintendo 64 | Game code      | `0x80210000`-`0x80220000` |      149 |    7 |   4.7% |
| [n64-game-code-80220000.md](progress/n64-game-code-80220000.md)           | Nintendo 64 | Game code      | `0x80220000`-`0x80230000` |      169 |    0 |   0.0% |
| [n64-game-code-80230000.md](progress/n64-game-code-80230000.md)           | Nintendo 64 | Game code      | `0x80230000`-`0x80240000` |       45 |    0 |   0.0% |
| [n64-zlib-80230000.md](progress/n64-zlib-80230000.md)                     | Nintendo 64 | zlib           | `0x80230000`-`0x80240000` |        7 |    7 | 100.0% |
| [n64-game-code-80240000.md](progress/n64-game-code-80240000.md)           | Nintendo 64 | Game code      | `0x80240000`-`0x80250000` |       59 |    9 |  15.3% |
| [n64-zlib-80240000.md](progress/n64-zlib-80240000.md)                     | Nintendo 64 | zlib           | `0x80240000`-`0x80250000` |       18 |   18 | 100.0% |
| [n64-game-code-80250000.md](progress/n64-game-code-80250000.md)           | Nintendo 64 | Game code      | `0x80250000`-`0x80260000` |      141 |    6 |   4.3% |
| [n64-game-code-80260000.md](progress/n64-game-code-80260000.md)           | Nintendo 64 | Game code      | `0x80260000`-`0x80270000` |        4 |    2 |  50.0% |
| [n64-sdk-libultra-80260000.md](progress/n64-sdk-libultra-80260000.md)     | Nintendo 64 | SDK (libultra) | `0x80260000`-`0x80270000` |      208 |  168 |  80.8% |
| [n64-ipl3-boot-code-a4000000.md](progress/n64-ipl3-boot-code-a4000000.md) | Nintendo 64 | IPL3 boot code | `0xa4000000`-`0xa4010000` |        7 |    0 |   0.0% |
| [pc-game-code-10000000.md](progress/pc-game-code-10000000.md)             | PC port     | Game code      | `0x10000000`-`0x10010000` |      246 |    0 |   0.0% |
| [pc-zlib-10000000.md](progress/pc-zlib-10000000.md)                       | PC port     | zlib           | `0x10000000`-`0x10010000` |        1 |    0 |   0.0% |
| [pc-game-code-10010000.md](progress/pc-game-code-10010000.md)             | PC port     | Game code      | `0x10010000`-`0x10020000` |      158 |    0 |   0.0% |
| [pc-game-code-10020000.md](progress/pc-game-code-10020000.md)             | PC port     | Game code      | `0x10020000`-`0x10030000` |      176 |    0 |   0.0% |
| [pc-game-code-10030000.md](progress/pc-game-code-10030000.md)             | PC port     | Game code      | `0x10030000`-`0x10040000` |      307 |    0 |   0.0% |
| [pc-game-code-10040000.md](progress/pc-game-code-10040000.md)             | PC port     | Game code      | `0x10040000`-`0x10050000` |      263 |    0 |   0.0% |
| [pc-game-code-10050000.md](progress/pc-game-code-10050000.md)             | PC port     | Game code      | `0x10050000`-`0x10060000` |       84 |    0 |   0.0% |
| [pc-game-code-10060000.md](progress/pc-game-code-10060000.md)             | PC port     | Game code      | `0x10060000`-`0x10070000` |      169 |    0 |   0.0% |
| [pc-game-code-10070000.md](progress/pc-game-code-10070000.md)             | PC port     | Game code      | `0x10070000`-`0x10080000` |      233 |    1 |   0.4% |
| [pc-game-code-10080000.md](progress/pc-game-code-10080000.md)             | PC port     | Game code      | `0x10080000`-`0x10090000` |        3 |    1 |  33.3% |
