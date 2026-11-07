#!/usr/bin/env python3
"""Link recorded IDO Release object pieces, optionally verifying a regional ROM."""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

from check_matching import ROM_SHA1

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--region', choices=sorted(ROM_SHA1), required=True)
  parser.add_argument('--rom', type=Path)
  parser.add_argument('--build', type=Path, required=True)
  parser.add_argument('--output', type=Path, required=True)
  args = parser.parse_args()

  recipes = json.loads((ROOT / 'tools' / 'rom_piece_recipes.json').read_text())
  regional = recipes[args.region]
  automatic = json.loads((ROOT / 'tools' / 'rom_piece_auto_recipes' /
                          f'{args.region}.json').read_text())
  if regional.keys() & automatic.keys():
    parser.error('Recorded ROM piece names overlap.')
  objects = args.build / 'src/n64/CMakeFiles/coastline_n64.dir'
  for name, recipe in (regional | automatic).items():
    command = [sys.executable, str(ROOT / 'tools' / 'link_rom_piece.py'),
               '--region', args.region,
               '--object', str(objects / recipe['object']),
               '--output', str(args.output / name), *recipe['args']]
    if args.rom:
      command.extend(('--rom', str(args.rom)))
    subprocess.run(command, check=True)
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
