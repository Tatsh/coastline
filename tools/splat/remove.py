#!/usr/bin/env python3
"""
Remove routines from the assembly of a release once their C bodies exist.

Each routine's block, from its `nonmatching` marker to its `endlabel`, is deleted from its file. A
jump table of the rodata that points at the labels of a removed routine is the table the C body no
longer uses, and its entries become zero. A file left without a routine leaves sources.cmake.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
LOCAL_LABEL = re.compile(r'^\s*(\.L[0-9A-F]{8}):\s*$')


def remove_routine(lines: list[str], name: str) -> tuple[list[str], set[str]] | None:
  """Delete a routine's block and return the remaining lines and the local labels it defined."""
  start = next((index for index, line in enumerate(lines)
                if re.match(rf'^nonmatching {re.escape(name)}\b', line)), None)
  if start is None:
    return None
  end = next(index for index in range(start, len(lines))
             if re.match(rf'^endlabel {re.escape(name)}\s*$', lines[index]))
  labels = {match.group(1) for line in lines[start:end] if (match := LOCAL_LABEL.match(line))}
  aliases = [line.split()[1] for line in lines[start:end] if line.strip().startswith('alabel ')]
  if aliases:
    raise SystemExit(f'{name} has the alternative entries {", ".join(aliases)}. The C body must '
                     'define them before the routine leaves the assembly.')
  while end + 1 < len(lines) and not lines[end + 1].strip():
    end += 1
  return lines[:start] + lines[end + 1:], labels


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('region', choices=('us', 'jp', 'eu'), help='release to edit')
  parser.add_argument('routines', nargs='+', help='names of the routines to remove')
  args = parser.parse_args()
  asm = ROOT / 'asm' / args.region
  removed_labels: set[str] = set()
  for name in args.routines:
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
      raise SystemExit(f'{name} is not in the assembly of {args.region}.')
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
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
