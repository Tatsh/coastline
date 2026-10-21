#!/usr/bin/env python3
"""
Carry the renames and deletions of name batches from the USA release to the other programs.

Each batch is a file of `ghidra_names.py` actions. Renames and deletions apply by name, so they
reach the programs that share the old name. Creations are tied to USA addresses and are skipped.
Without `--apply`, the script lists what it would change in each program.
"""
from __future__ import annotations

import argparse
import json
import urllib.request
from pathlib import Path

BRIDGE = 'http://127.0.0.1:8090'
PROGRAMS = ('jpn.z64', 'eur.z64', 'BRD3D.dll', 'BRGlide.dll')
SCRIPT = Path(__file__).with_name('SyncNames.java')


def run_script(program: str, args: str) -> str:
  request = urllib.request.Request(f'{BRIDGE}/run_script_inline?program={program}',
                                   data=json.dumps({
                                       'code': SCRIPT.read_text(encoding='utf-8'),
                                       'args': args
                                   }).encode(),
                                   headers={'Content-Type': 'application/json'})
  with urllib.request.urlopen(request, timeout=600) as response:
    return response.read().decode().split('--- SCRIPT OUTPUT ---', 1)[-1]


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('batches', type=Path, nargs='+', help='batch files, oldest first')
  parser.add_argument('--apply', action='store_true', help='change the programs')
  args = parser.parse_args()
  actions: dict[str, str] = {}
  for batch in args.batches:
    for line in batch.read_text(encoding='utf-8').splitlines():
      fields = line.split()
      if fields and fields[0] == 'rename':
        actions[fields[1]] = fields[2]
      elif fields and fields[0] == 'delete':
        actions[fields[1]] = '-'
  # A later batch that renames a new name again replaces the earlier target.
  for old, new in list(actions.items()):
    while new in actions and actions[new] != '-':
      new = actions.pop(new)
    actions[old] = new
  pairs = ';'.join(f'{old}:{new}' for old, new in actions.items())
  failures = 0
  for program in PROGRAMS:
    output = run_script(program, f'{int(args.apply)} {pairs}')
    if 'SCRIPT EXECUTION ERROR' in output:
      failures += 1
    changes = [line for line in output.splitlines() if line.startswith(('REN ', 'DEL '))]
    print(f'{program}: {len(changes)} changes')
    for change in changes:
      print(f'  {change}')
  if args.apply:
    urllib.request.urlopen(urllib.request.Request(f'{BRIDGE}/save_all_programs', method='POST'),
                           timeout=120).read()
  return 1 if failures else 0


if __name__ == '__main__':
  raise SystemExit(main())
