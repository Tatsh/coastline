#!/usr/bin/env python3
"""
Finish a splat split of one release for the build.

Symbols the assembly defines itself are removed from the absolute symbol files splat writes. A
linker script assignment would override the labels once the code moves.

A word of the data or the rodata that points into the program image but that spimdisasm emitted as
a number would retain its old value when the code moves. Each such word becomes a reference to the
label at its target, or to the enclosing label plus an offset. A target inside a routine with no
label gets one.

The assembly sources are then listed in address order for CMake.
"""
from __future__ import annotations

import argparse
import bisect
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
LABEL = re.compile(r'^\s*(?:glabel|dlabel|alabel|jlabel)\s+(\S+)', re.M)
# A label line, then the first line with an address comment that follows it.
LABEL_LINE = re.compile(r'^\s*(?:glabel|dlabel|alabel|jlabel)\s+(\S+)\s*$')
LOCAL_LINE = re.compile(r'^\s*(\.L[0-9A-F]{8}):\s*$')
ADDRESS = re.compile(r'/\* [0-9A-F]+ ([0-9A-F]{8}) ')
RAW_WORD = re.compile(r'^(\s*/\* [0-9A-F]+ [0-9A-F]{8} [0-9A-F]{8} \*/ \.word )0x([0-9A-F]{8})\s*$')
RAW_BYTE = re.compile(r'^\s*/\* ([0-9A-F]+) ([0-9A-F]{8}) \*/ \.byte 0x([0-9A-F]{2})\s*$')


def join_pointer_bytes(lines: list[str], exact: dict[int, str]) -> list[str]:
  """Join four bytes at an aligned address into a word when they point at an exact label."""
  out = []
  index = 0
  while index < len(lines):
    group = [RAW_BYTE.match(line) for line in lines[index:index + 4]]
    if len(group) == 4 and all(group):
      addresses = [int(match.group(2), 16) for match in group]
      value = int(''.join(match.group(3) for match in group), 16)
      if (addresses[0] % 4 == 0 and addresses == list(range(addresses[0], addresses[0] + 4))
          and value in exact):
        out.append(f'    /* {group[0].group(1)} {group[0].group(2)} {value:08X} */ .word 0x{value:08X}')
        index += 4
        continue
    out.append(lines[index])
    index += 1
  return out
IMAGE = (0x80200000, 0x802AC400)


def collect_labels(sources: list[Path]) -> tuple[list[tuple[int, str]], dict[int, tuple[Path, int]]]:
  """Return every named label with its address, and the line of every text address."""
  labels: list[tuple[int, str]] = []
  lines_at: dict[int, tuple[Path, int]] = {}
  for source in sources:
    pending: list[str] = []
    for number, line in enumerate(source.read_text(encoding='utf-8').splitlines()):
      if match := LABEL_LINE.match(line):
        pending.append(match.group(1))
        continue
      if match := ADDRESS.search(line):
        address = int(match.group(1), 16)
        labels.extend((address, name) for name in pending)
        pending = []
        lines_at.setdefault(address, (source, number))
  labels.sort()
  return labels, lines_at


def symbolize(data_sources: list[Path], text_sources: list[Path]) -> int:
  labels, lines_at = collect_labels(text_sources + data_sources)
  exact = {address: name for address, name in labels}
  starts = [address for address, _ in labels]
  insertions: dict[Path, dict[int, str]] = {}
  rewritten = 0
  for source in data_sources:
    out = []
    for line in join_pointer_bytes(source.read_text(encoding='utf-8').splitlines(), exact):
      match = RAW_WORD.match(line)
      value = int(match.group(2), 16) if match else 0
      if not match or not IMAGE[0] <= value < IMAGE[1]:
        out.append(line)
        continue
      if value in exact:
        target = exact[value]
      elif value in lines_at and lines_at[value][0] in text_sources:
        target = f'L_{value:08X}'
        file, number = lines_at[value]
        insertions.setdefault(file, {})[number] = f'  alabel {target}'
        exact[value] = target
      else:
        index = bisect.bisect_right(starts, value) - 1
        if index < 0:
          out.append(line)
          continue
        base, name = labels[index]
        target = f'{name} + 0x{value - base:X}'
      out.append(f'{match.group(1)}{target}')
      rewritten += 1
    source.write_text('\n'.join(out) + '\n', encoding='utf-8')
  for file, at in insertions.items():
    lines = file.read_text(encoding='utf-8').splitlines()
    for number in sorted(at, reverse=True):
      lines.insert(number, at[number])
    file.write_text('\n'.join(lines) + '\n', encoding='utf-8')
  return rewritten


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('region', choices=('us', 'jp', 'eu'), help='release to finish')
  args = parser.parse_args()
  asm = ROOT / 'asm' / args.region
  text_sources = sorted(asm.glob('[0-9A-F]*.s'))
  data_sources = [asm / 'data' / name for name in ('data.data.s', 'rodata.rodata.s')]
  sources = text_sources + data_sources + [asm / 'data' / 'bss.bss.s']
  print(f'{symbolize(data_sources, text_sources)} raw pointer words now reference symbols.')
  defined = set()
  for source in sources + [asm / 'entry.s']:
    defined |= set(LABEL.findall(source.read_text(encoding='utf-8')))
  for name in ('undefined_syms.ld', 'undefined_funcs.ld'):
    path = asm / name
    lines = path.read_text(encoding='utf-8').splitlines()
    kept = [line for line in lines if line.split(' = ')[0] not in defined]
    path.write_text(''.join(f'{line}\n' for line in kept), encoding='utf-8')
  listing = [
    f'# Assembly of the {args.region} release, generated by splat from tools/splat/{args.region}.yaml.',
    '# A routine is removed from its file when it is reconstructed in C, and a file is removed',
    '# from this list when it is empty. The order is the address order of the original image.',
    f'set(COASTLINE_ASM_{args.region.upper()}',
  ]
  listing += [f'    {source.relative_to(ROOT).as_posix()}' for source in sources]
  listing[-1] += ')'
  (asm / 'sources.cmake').write_text('\n'.join(listing) + '\n', encoding='utf-8')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
