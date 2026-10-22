# Game code, PC port, `0x10080000` to `0x10090000`

1 of 3 routines done.

Sorted by length, then reference count, then status (remaining first), then name.
Signatures are the current Ghidra prototypes and are preliminary.

| Name                          | Status | # xref | Length | BRD3D        | BRGlide      | Signature                                 |
| ----------------------------- | :----: | -----: | -----: | ------------ | ------------ | ----------------------------------------- |
| `BuildLocaleTimeFormatString` |        |      1 |    246 | `0x10088bd0` |              | `undefined BuildLocaleTimeFormatString()` |
| `CPtoLCID`                    |        |      2 |     56 | `0x10086980` |              | `undefined CPtoLCID()`                    |
| `DllMain`                     |   ✅   |      1 |      8 | `0x10086b90` | `0x10074b00` | `undefined DllMain()`                     |
