# PC port game code from 0x10080000 to 0x10090000

1 of 1 routines done.

Sorted by length, then reference count, then status (remaining first), then name.
Signatures are the current Ghidra prototypes and are preliminary.
The N64 column gives the status and master address of the N64 routine of
the same name, for a routine the port retained from the N64 game.

| Name      | Status | N64 | # xref | Length | BRD3D        | BRGlide      | Signature             |
| --------- | :----: | --- | -----: | -----: | ------------ | ------------ | --------------------- |
| `DllMain` |   ✅   |     |      1 |      8 | `0x10086b90` | `0x10074b00` | `undefined DllMain()` |
