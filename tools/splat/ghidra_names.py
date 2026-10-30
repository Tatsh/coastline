#!/usr/bin/env python3
"""
Apply a batch of label renames, deletions, and creations to a program of the Ghidra project.

The batch file has one action per line: `rename OLD NEW`, `delete NAME`, or `create ADDRESS NAME`.
Addresses of the existing labels come from the exported symbol file. The program is saved after
the batch.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import urllib.request
from pathlib import Path

BRIDGE = f'http://127.0.0.1:{os.environ.get("GHIDRA_PORT", "8090")}'


def post(endpoint: str, program: str, body: dict) -> dict:
  request = urllib.request.Request(f'{BRIDGE}/{endpoint}?program={program}',
                                   data=json.dumps(body).encode(),
                                   headers={'Content-Type': 'application/json'})
  with urllib.request.urlopen(request, timeout=120) as response:
    return json.loads(response.read().decode())


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('program', help='program of the Ghidra project, for example usa.z64')
  parser.add_argument('symbols', type=Path, help='symbol file the program exported')
  parser.add_argument('batch', type=Path, help='file of actions')
  args = parser.parse_args()
  addresses = {match.group(1): match.group(2)
               for match in re.finditer(r'^(\S+) = 0x([0-9A-F]+);', args.symbols.read_text(), re.M)}
  failures = 0
  deletions = []
  for line in args.batch.read_text(encoding='utf-8').splitlines():
    fields = line.split()
    if not fields or fields[0].startswith('#'):
      continue
    action = fields[0]
    if action == 'rename':
      old, new = fields[1], fields[2]
      result = post('rename_label', args.program,
                    {'address': f'0x{addresses[old]}', 'old_name': old, 'new_name': new})
      if 'Label not found' in result.get('error', ''):
        # The name belongs to a function, which the bridge renames by address.
        result = post('rename_function_by_address', args.program,
                      {'function_address': f'0x{addresses[old]}', 'new_name': new})
    elif action == 'create':
      result = post('create_label', args.program, {'address': fields[1], 'name': fields[2]})
    elif action == 'delete':
      deletions.append({'address': f'0x{addresses[fields[1]]}', 'name': fields[1]})
      continue
    else:
      raise SystemExit(f'Unknown action {action}.')
    if result.get('status') != 'success':
      failures += 1
      print(f'{line}: {json.dumps(result)[:200]}')
  if deletions:
    result = post('batch_delete_labels', args.program, {'labels': deletions})
    if result.get('errors_count'):
      failures += 1
      print(f'delete: {json.dumps(result)[:300]}')
  urllib.request.urlopen(urllib.request.Request(f'{BRIDGE}/save_all_programs', method='POST'),
                         timeout=120).read()
  print(f'{failures} actions failed.')
  return 1 if failures else 0


if __name__ == '__main__':
  raise SystemExit(main())
