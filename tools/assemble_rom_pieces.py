#!/usr/bin/env python3
"""Assemble verified IDO code with the original ROM's non-code assets."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

import yaml

from check_matching import ROM_SHA1

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--region', choices=sorted(ROM_SHA1), required=True)
  parser.add_argument('--reference', type=Path, required=True)
  parser.add_argument('--pieces', type=Path, required=True)
  parser.add_argument('--output', type=Path, required=True)
  args = parser.parse_args()

  reference = args.reference.read_bytes()
  expected_sha1 = ROM_SHA1[args.region]
  if hashlib.sha1(reference).hexdigest() != expected_sha1:
    parser.error('The reference ROM does not match the verified regional SHA-1.')
  config = yaml.safe_load((ROOT / 'tools' / 'splat' / f'{args.region}.yaml').read_text())
  main_segment = next(segment for segment in config['segments'] if segment.get('name') == 'main')
  code_start = main_segment['start']
  code_end = next(part[0] for part in main_segment['subsegments']
                  if isinstance(part, list) and part[1] == 'data')
  image_base = main_segment['vram']
  bss_start = next(part['vram'] for part in main_segment['subsegments']
                   if isinstance(part, dict) and part['type'] == 'bss')
  program_end = bss_start - image_base + code_start
  if not code_start < code_end < program_end <= len(reference):
    parser.error('The regional program boundaries are invalid.')

  symbols = (ROOT / 'tools' / 'splat' / f'symbol_addrs.{args.region}.txt').read_text()
  functions = [(name, int(address, 16), int(size, 16)) for name, address, size in
               re.findall(r'^(\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+)',
                          symbols, re.M)]
  declared = bytearray(code_end - code_start)
  for name, address, size in functions:
    start = address - image_base
    if size <= 0 or start < 0 or start + size > len(declared) or any(declared[start:start + size]):
      parser.error(f'Function {name} has an invalid or overlapping code range.')
    declared[start:start + size] = b'\1' * size
  if any(value and not declared[index]
         for index, value in enumerate(reference[code_start:code_end])):
    parser.error('The function inventory omits nonzero code bytes.')

  # shortcut: Use reference non-code bytes until extracted asset inputs feed this builder.
  assembled = bytearray(reference)
  assembled[code_start:code_end] = bytes(code_end - code_start)
  claimed = bytearray(program_end - code_start)
  code_claimed = bytearray(code_end - code_start)
  pieces = sorted(args.pieces.glob('*/piece.json'))
  if not pieces:
    parser.error('No verified pieces were found.')
  for manifest in pieces:
    piece = json.loads(manifest.read_text())
    if piece['region'] != args.region or piece['rom_sha1'] != expected_sha1:
      parser.error(f'{manifest} describes another ROM.')
    for section, item in piece['sections'].items():
      if section not in ('text', 'data', 'rodata'):
        parser.error(f'{manifest} has an unsupported section.')
      address, size = item['address'], item['size']
      offset = address - image_base + code_start
      if size <= 0 or not code_start <= offset < offset + size <= program_end:
        parser.error(f'{manifest} has an invalid {section} range.')
      if (section == 'text' and offset + size > code_end or
          section != 'text' and offset < code_end):
        parser.error(f'{manifest} places {section} outside its region.')
      start = offset - code_start
      if any(claimed[start:start + size]):
        parser.error(f'{manifest} overlaps an earlier piece.')
      content = (manifest.parent / f'{section}.bin').read_bytes()
      if len(content) != size or content != reference[offset:offset + size]:
        parser.error(f'{manifest} differs from the verified ROM.')
      claimed[start:start + size] = b'\1' * size
      if section == 'text':
        code_claimed[start:start + size] = b'\1' * size
      assembled[offset:offset + size] = content

  missing = [(name, address) for name, address, size in functions
             if not all(code_claimed[address - image_base:address - image_base + size])]
  print(f'{len(functions) - len(missing)}/{len(functions)} functions have exact linked code.')
  print(f'{sum(code_claimed[index] for index, value in enumerate(declared) if value)}/'
        f'{sum(declared)} function bytes have exact linked code.')
  if missing:
    print('Missing:', ', '.join(f'{name}@{address:#x}' for name, address in missing[:10]))
    return 1
  if hashlib.sha1(assembled).hexdigest() != expected_sha1:
    parser.error('The assembled ROM differs from the verified original.')
  args.output.parent.mkdir(parents=True, exist_ok=True)
  args.output.write_bytes(assembled)
  print(f'Wrote byte-exact ROM to {args.output}.')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
