#!/usr/bin/env python3
"""
Prepare the splat symbol file of one release from the symbols Ghidra exports.

A label at a word of the data that points into the middle of a routine is a jump table. Ghidra
sizes the label as one pointer, and spimdisasm would read a one-entry table. The label is marked as
a jump table instead. A label inside a table would split the table and is dropped.
"""
from __future__ import annotations

import argparse
import bisect
import re
import struct
from pathlib import Path
from typing import NamedTuple

LINE = re.compile(r'(\S+) = 0x([0-9A-F]+);(.*)')
FUNCTION = re.compile(r'type:func size:0x([0-9A-F]+)')
IMAGE_BASE = 0x80200000
ROM_BASE = 0x1000


class Release(NamedTuple):
  """The layout of one release in RAM, and its program in the Ghidra project."""
  program: str
  data_start: int
  bss_start: int
  bss_end: int


REGIONS = {
  'us': Release('usa.z64', 0x8026EA00, 0x802AC400, 0x80382BB0),
  'jp': Release('jpn.z64', 0x80270480, 0x802ACB30, 0x803837A0),
  'eu': Release('eur.z64', 0x802709D0, 0x802AE0F0, 0x80384B30),
}


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('--region', choices=tuple(REGIONS), default='us', help='release to prepare')
  parser.add_argument('exported', type=Path, help='symbol file Ghidra exported')
  parser.add_argument('rom', type=Path, help='cartridge image of the release, in .z64 order')
  parser.add_argument('output', type=Path, help='symbol file to write for splat')
  args = parser.parse_args()
  release = REGIONS[args.region]
  data = (release.data_start, release.bss_start)
  rom = args.rom.read_bytes()
  entries = [LINE.match(line) for line in args.exported.read_text(encoding='utf-8').splitlines()]
  routines = sorted((int(match.group(2), 16), int(match.group(2), 16) + int(size.group(1), 16))
                    for match in entries if match and (size := FUNCTION.search(match.group(3))))
  starts = [start for start, _ in routines]

  def inside_routine(value: int) -> bool:
    index = bisect.bisect_right(starts, value) - 1
    return index >= 0 and routines[index][0] < value < routines[index][1]

  def word(address: int) -> int | None:
    if not data[0] <= address < data[1]:
      return None
    return struct.unpack_from('>I', rom, address - IMAGE_BASE + ROM_BASE)[0]

  out = []
  for match in entries:
    if not match:
      continue
    name, address = match.group(1), int(match.group(2), 16)
    value, previous = word(address), word(address - 4)
    if value is not None and inside_routine(value):
      if previous is not None and inside_routine(previous):
        continue
      out.append(f'{name} = 0x{address:08X}; // type:jtbl')
      continue
    out.append(match.group(0))
  args.output.write_text('\n'.join(out) + '\n', encoding='utf-8')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
