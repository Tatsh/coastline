# IPL3 boot code, Nintendo 64, `0xa4000000` to `0xa4010000`

0 of 7 routines done.

Sorted by length, then reference count, then status (remaining first), then name.
Signatures are the current Ghidra prototypes and are preliminary.

| Name                    | Status | # xref | Length | USA          | JPN          | EUR          | Signature                           |
| ----------------------- | :----: | -----: | -----: | ------------ | ------------ | ------------ | ----------------------------------- |
| `bootMain`              |        |      2 |   1144 | `0xa4000040` | `0xa4000040` | `0xa4000040` | `undefined bootMain()`              |
| `RspIplCalibratePllAvg` |        |      1 |    264 | `0xa4000778` | `0xa4000778` | `0xa4000778` | `undefined RspIplCalibratePllAvg()` |
| `RspIplBinarySearchPll` |        |      1 |    188 | `0xa4000980` | `0xa4000980` | `0xa4000980` | `undefined RspIplBinarySearchPll()` |
| `RspIplReadPllStatus`   |        |      2 |    156 | `0xa4000ad0` | `0xa4000ad0` | `0xa4000ad0` | `undefined RspIplReadPllStatus()`   |
| `RspIplSetPllConfig`    |        |      5 |    144 | `0xa4000a40` | `0xa4000a40` | `0xa4000a40` | `undefined RspIplSetPllConfig()`    |
| `RspIplMeasurePllDelay` |        |      1 |    136 | `0xa4000880` | `0xa4000880` | `0xa4000880` | `undefined RspIplMeasurePllDelay()` |
| `RspIplCountBitErrors`  |        |      1 |    116 | `0xa400090c` | `0xa400090c` | `0xa400090c` | `undefined RspIplCountBitErrors()`  |
