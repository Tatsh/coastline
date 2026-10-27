# Reconstruction methodology

This tree reconstructs Top Gear Rally from its binaries: the three Nintendo 64 releases (C, built
with IDO) and the two Windows libraries BRD3D.dll and BRGlide.dll (C++, built with MSVC 6). The
Ghidra project holds all five as `usa.z64`, `jpn.z64`, `eur.z64`, `BRD3D.dll`, and `BRGlide.dll`,
reached through the HTTP bridge on port 8090. The USA release is the naming master and the current
target. These rules govern how to get from the binaries to faithful source. The coding style of the
source lives in [general.md](general.md) and `.clang-format`; this file is about correctness.

## Addresses and status

- Tie a reconstructed routine or datum to the binary with one `@ghidraAddress` Doxygen line per
  program, in the form `@ghidraAddress USA: 0x8021e5c4`, with `JPN`, `EUR`, `BRD3D`, or `BRGlide`
  for the others. Put the lines in the Doxygen block of the declaration in the header. A datum that
  has no header declaration takes the same form as a line comment above its definition. The progress
  report reads exactly this form.
- A routine whose body is only a placeholder carries `@stub` in the same Doxygen block, and is not
  counted as done. Remove the tag when the body is reconstructed.
- Keep the binary's names. Globals keep their Ghidra names (`g_` prefix and all). Ghidra placeholder
  names (`FUN_*`, `DAT_*`, `PTR_*`, `UNK_*`, `switchD_*`) are never identifiers in reconstructed
  code: name them descriptively, in Ghidra first, then in the source.

## Signatures and types

- The decompile is a usually incorrect guide, not the source of truth. Work from the disassembly,
  and use the decompile to read structure, never to copy. A signature change comes from the
  disassembly only.
- Recover the true signature before anything else. A decompile that uses `in_*` pseudo-variables
  (`in_a0`, `in_v1`, `in_stack_*`) or `extraout_*` is missing parameters or a return. Never model a
  routine with fewer arguments than the register and stack reads prove. Scan for these whenever a
  prototype looks empty or returns `undefined`.
- Scrutinise return values as hard as arguments. Confirm the real return type and whether the value
  is used, discarded, or is `this`. A `bool` in a wider register is common.
- A signature fix is complete only when every caller agrees with it: fix the prototype in Ghidra so
  every call site decompiles cleanly, then every reconstructed caller.
- Model real types. Use real struct fields (never `field2_0x14`), real pointer, enum, and `bool`
  types (never `void *` for a typed pointer, an `int` that holds a pointer, `undefined4`, or
  `dword`). On the N64, use the libultra types `u8`, `s32`, `f32`, and the rest, never the Ghidra
  `byte`, `uint`, and `dword` names.
- `ptr + offset` arithmetic into an object is banned. `*(int *)(p + 0x28)` means a field exists at
  that offset: create or correct the struct in Ghidra, then write `p->nCount`.
- A struct layout must match the binary byte for byte. The N64 and the Windows libraries are both
  32-bit, and the original data segment is linked into the build until it is converted, so a
  mismatched layout reads the wrong bytes. Record a still-unknown member with a trailing
  `/* +0xNN */` comment only where the layout has a gap. A settled struct needs no offset comments,
  and no `#pragma pack` or layout assertion.
- Fix the Ghidra program as you go: give every parameter, local, and return a real type, rename
  every auto-named variable (`iVar1`, `uVar2`, `pcVar3`), name and type every placeholder global,
  and create the real struct types. The reconstructed source must agree with Ghidra when you finish
  a routine.

## C and C++

- The N64 code is C89 as IDO accepted it. No `//` comments, declarations at the top of a block, no
  mixed declarations and code. The Windows code is the C++ of MSVC 6 without exceptions or RTTI.
- On the N64, a routine whose first parameter is a struct pointer stays a free function taking that
  pointer, as the original is. On Windows, a routine Ghidra places in a class namespace is a member
  of that class, `__thiscall` with `this` first. A free function on Windows that reads and writes
  the fields of one pointer argument is a candidate for a method of that class; move it in Ghidra
  first, then in the source. Class names come from the Ghidra namespaces the typing pass assigned.
- `reinterpret_cast`, `dynamic_cast`, and C-style casts between object types are smells that hide a
  type or signature bug. Replace them with real types at the source.
- Flag surprising but faithful behaviour with a short comment so a reader does not take it for a
  bug: a discarded return, a call kept only for effect, a deliberate off-by-one, a value that looks
  wrong but matches the binary. One line.
- For a large body, save the decompiler and disassembler output under `.wiswa-ci/`, then break the
  body into helpers at the repeated or logically distinct blocks and reconstruct with those helpers.
  Define free-function helpers in `.c` or `.cpp` files. Declare helpers used by another translation
  unit in an associated header. A helper used within one translation unit has internal linkage.
  Do not define static free functions in headers.

## Files and build units

- Put all `#include` directives at the top of the file, before declarations and definitions.
- A routine that differs substantially between targets has one translation unit per target, named
  with the target suffix (`winmain-d3d.cpp`, `winmain-glide.cpp`), and CMake selects the unit. A
  difference of a few lines stays in one unit under `#if`. One header serves every target, with
  `#if` in a header only where a prototype differs. The `TGR_PLATFORM`, `TGR_REGION`, and
  `TGR_RENDERER` macros exist for that and nothing else.
- When a source needs a routine, datum, or type this source does not define, include its header,
  never write a local declaration. If the header does not exist yet, start it: the declaration goes
  there with its `@ghidraAddress` lines, and the progress report counts it as declared.
- Group routines into source files by the original file boundaries where they are known (IDO pads
  the text of each file to 16 bytes), and name the file after what it does. Each subsystem
  directory under `src` has a `CMakeLists.txt` that lists its sources explicitly and joins
  `COASTLINE_SUBSYSTEMS`.
- libultra is reconstructed under `src/libultra`, with the SDK's own file and header names. zlib is
  imported under `3rdparty` at the matching release and edited in place where the game differs.
  Nothing the compiler or the linker supplies is reconstructed.
- The data and BSS of the original image are linked from the assembly Ghidra exports until they are
  converted to C. A datum is converted by defining it in C with its Ghidra name and removing it from
  the exported assembly, never by defining it twice.

## Closing a unit of work

- Compile the unit with the real toolchain (`cmake --workflow --preset n64-us`) before calling it
  done, regenerate the progress report, run `yarn format` and `yarn qa`, commit through `/ci`, and
  push.
