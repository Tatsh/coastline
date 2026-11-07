#!/usr/bin/env python3
"""Verify recorded IDO Release object pieces against a regional ROM."""
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
  parser.add_argument('--rom', type=Path, required=True)
  parser.add_argument('--build', type=Path, required=True)
  parser.add_argument('--output', type=Path, required=True)
  args = parser.parse_args()

  recipes = json.loads((ROOT / 'tools' / 'rom_piece_recipes.json').read_text())
  objects = args.build / 'src/n64/CMakeFiles/coastline_n64.dir'
  for name, recipe in recipes[args.region].items():
    command = [sys.executable, str(ROOT / 'tools' / 'link_rom_piece.py'),
               '--region', args.region, '--rom', str(args.rom),
               '--object', str(objects / recipe['object']),
               '--output', str(args.output / name), *recipe['args']]
    subprocess.run(command, check=True)
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
