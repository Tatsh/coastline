#!/usr/bin/env python3
"""
Remove routines and data from the assembly of a release once their C definitions exist.

Each routine's block, from its `nonmatching` marker to its `endlabel`, is deleted from its file. A
jump table of the rodata that points at the labels of a removed routine is the table the C body no
longer uses, and its entries become zero. A file with no remaining routine is removed from
sources.cmake. A datum, given as NAME:SIZE, loses every block of the data and BSS files from its
label to its size, the labels splat placed inside it included.

Every removed routine and datum is recorded in converted.txt, and postprocess.py removes them again
after a new split.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
LOCAL_LABEL = re.compile(r'^\s*(?:jlabel\s+)?(\.L[0-9A-F]{8}):?\s*$')


def remove_routine(lines: list[str], name: str) -> tuple[list[str], set[str]] | None:
  """Delete a routine's block and return the remaining lines and the local labels it defined."""
  start = next((index for index, line in enumerate(lines)
                if re.match(rf'^nonmatching {re.escape(name)}\b', line)), None)
  if start is None:
    return None
  end = next(index for index in range(start, len(lines))
             if re.match(rf'^endlabel {re.escape(name)}\s*$', lines[index]))
  labels = {match.group(1) for line in lines[start:end] if (match := LOCAL_LABEL.match(line))}
  # spimdisasm names the target of a jump within a hand-written routine func_ADDRESS. Such a target
  # is internal to the routine, and a reference from elsewhere would fail the link.
  aliases = [line.split()[1] for line in lines[start:end] if line.strip().startswith('alabel ')
             and not re.fullmatch(r'func_[0-9A-F]{8}', line.split()[1])]
  if aliases:
    raise SystemExit(f'{name} has the alternative entries {", ".join(aliases)}. The C body must '
                     'define them before the routine is removed from the assembly.')
  while end + 1 < len(lines) and not lines[end + 1].strip():
    end += 1
  return lines[:start] + lines[end + 1:], labels


ADDRESS = re.compile(r'/\*\s*(?:[0-9A-F]+\s+)?(80[0-9A-F]{6})\b')


def block_address(lines: list[str], start: int) -> int | None:
  """The address of the first word or space of the block that starts at a nonmatching line."""
  for line in lines[start + 1:]:
    if line.startswith('nonmatching '):
      return None
    if match := ADDRESS.search(line):
      return int(match.group(1), 16)
  return None


def remove_datum(lines: list[str], name: str, size: int) -> list[str] | None:
  """
  Delete the blocks of a datum that size bytes from its label span, inner labels included.

  splat may split one datum into several labelled blocks, such as the fields of a structure, so
  every block whose first address lies within the datum goes.
  """
  start = next((index for index, line in enumerate(lines)
                if re.match(rf'^nonmatching {re.escape(name)}\b', line)), None)
  if start is None:
    return None
  first = block_address(lines, start)
  if first is None:
    raise SystemExit(f'{name}: no address in its block.')
  end = start + 1
  while end < len(lines):
    if lines[end].startswith('nonmatching '):
      address = block_address(lines, end)
      if address is None or address >= first + size:
        break
    end += 1
  return lines[:start] + lines[end:]


def remove(region: str, routines: list[str], *, record: bool = True) -> None:
  """
  Remove routines and data from the assembly of a release and, unless told not to, record them.

  A datum is given as NAME:SIZE (the size in bytes, decimal or 0x hex) and is removed from the data
  and BSS files. Anything else is a routine.
  """
  asm = ROOT / 'asm' / region
  data = [entry for entry in routines if ':' in entry]
  routines_only = [entry for entry in routines if ':' not in entry]
  data_sources = sorted((asm / 'data').glob('*.s'))
  # Check every name first, so that a mistake leaves the assembly and converted.txt untouched.
  text = '\n'.join(source.read_text(encoding='utf-8') for source in asm.glob('[0-9A-F]*.s'))
  data_text = '\n'.join(source.read_text(encoding='utf-8') for source in data_sources)
  missing = [name for name in routines_only
             if not re.search(rf'^nonmatching {re.escape(name)}\b', text, re.M)]
  missing += [entry for entry in data
              if not re.search(rf'^nonmatching {re.escape(entry.split(":")[0])}\b', data_text,
                               re.M)]
  if missing:
    raise SystemExit(f'{", ".join(missing)} not in the assembly of {region}.')
  for entry in data:
    name, size = entry.split(':')
    for source in data_sources:
      result = remove_datum(source.read_text(encoding='utf-8').splitlines(), name, int(size, 0))
      if result is not None:
        source.write_text('\n'.join(result) + '\n', encoding='utf-8')
        print(f'{name}: removed {size} bytes from {source.name}.')
        break
  removed_labels: set[str] = set()
  for name in routines_only:
    for source in sorted(asm.glob('[0-9A-F]*.s')):
      result = remove_routine(source.read_text(encoding='utf-8').splitlines(), name)
      if result is None:
        continue
      lines, labels = result
      removed_labels |= labels
      if any(line.startswith('glabel ') for line in lines):
        source.write_text('\n'.join(lines) + '\n', encoding='utf-8')
      else:
        source.unlink()
      print(f'{name}: removed from {source.name}.')
      break
    else:
      raise SystemExit(f'{name} is not in the assembly of {region}.')
  rodata = asm / 'data' / 'rodata.rodata.s'
  text = rodata.read_text(encoding='utf-8')
  for label in removed_labels:
    text = re.sub(rf'\.word {re.escape(label)}\b', '.word 0', text)
  rodata.write_text(text, encoding='utf-8')
  sources = asm / 'sources.cmake'
  kept = [line for line in sources.read_text(encoding='utf-8').splitlines()
          if not line.strip().startswith('asm/') or (ROOT / line.strip().rstrip(')')).exists()]
  if not kept[-1].endswith(')'):
    kept[-1] += ')'
  sources.write_text('\n'.join(kept) + '\n', encoding='utf-8')
  if record:
    converted = asm / 'converted.txt'
    names = set(converted.read_text(encoding='utf-8').split()) if converted.exists() else set()
    converted.write_text(''.join(f'{name}\n' for name in sorted(names | set(routines))),
                         encoding='utf-8')


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('region', choices=('us', 'jp', 'eu'), help='release to edit')
  parser.add_argument('routines', nargs='+',
                      help='names of the routines to remove, and data as NAME:SIZE')
  args = parser.parse_args()
  remove(args.region, args.routines)
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
