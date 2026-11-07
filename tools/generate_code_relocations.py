#!/usr/bin/env python3
"""Record relocation bits for byte-exact IDO Release ROM reconstruction."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

from check_matching import MASKS, ROM_SHA1, binutil, digest, functions

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--region', choices=sorted(ROM_SHA1), required=True)
parser.add_argument('--rom', type=Path, required=True)
parser.add_argument('--build', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
region = args.region
build = args.build
rom = args.rom.read_bytes()
if hashlib.sha1(rom).hexdigest() != ROM_SHA1[region]:
  parser.error('The reference ROM does not match the verified regional SHA-1.')
(root / '.wiswa-ci').mkdir(exist_ok=True)
symbol_text = (root / 'tools/splat' / f'symbol_addrs.{region}.txt').read_text()
inventory = {int(a, 16): (name, int(size, 16)) for name, a, size in re.findall(
  r'^(\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+)',
  symbol_text, re.M)}
expected = defaultdict(list)
records = json.loads((root / 'tools/matching' / f'{region}.json').read_text())['functions']
for address, item in records.items():
  for source, hashes in item['sources'].items():
    for value in hashes:
      expected[value].append((int(address, 16), source))
target = re.compile(r'CMakeFiles/coastline_(?:n64|common|zlib|libultra_\w+|'
          r'common_libultra_\w+|main|vehiclestatus)\.dir/')
choices = defaultdict(list)
object_meta = {}
for obj in (build / 'src').rglob('*.obj'):
  relative = obj.relative_to(build).as_posix()
  if not target.search(relative):
    continue
  nm = subprocess.run([binutil('nm'), '-n', '--defined-only', str(obj)], check=True,
            capture_output=True, text=True).stdout
  bases = defaultdict(int)
  for offset, name in re.findall(r'^([0-9A-Fa-f]+) [Tt] (\S+)$', nm, re.M):
    for address, (original_name, _size) in inventory.items():
      if original_name == name:
        bases[address - int(offset, 16)] += 1
  object_meta[relative] = (obj, bases)
  for name, words, masks, offset in functions(obj):
    for address, source in expected[digest(words, masks)]:
      if obj.name != Path(source).name + '.obj':
        continue
      rank = 0 if name == inventory[address][0] else 1
      choices[address].append((rank, -bases[address - offset], relative, offset,
                  len(words) * 4, name))

if set(choices) != set(inventory):
  raise SystemExit(f'Missing digest matches: {set(inventory) - set(choices)}')
selected = {address: sorted(candidates)[0] for address, candidates in choices.items()}
groups = defaultdict(list)
for address, candidate in selected.items():
  groups[(candidate[2], candidate[3], candidate[4], candidate[5])].append(address)

relocations = {}
texts = {}
for relative, (obj, _bases) in object_meta.items():
  if relative not in {candidate[2] for candidate in selected.values()}:
    continue
  with tempfile.NamedTemporaryFile(dir=root / '.wiswa-ci') as text_file:
    subprocess.run([binutil('objcopy'), '-O', 'binary', '--only-section=.text',
                    str(obj), text_file.name], check=True)
    texts[relative] = Path(text_file.name).read_bytes()
  relocations[relative] = {}
  in_text = False
  for line in subprocess.run([binutil('readelf'), '-rW', str(obj)], check=True,
               capture_output=True, text=True).stdout.splitlines():
    if line.startswith('Relocation section'):
      in_text = "'.rel.text'" in line or "'.rela.text'" in line
      continue
    parts = line.split()
    if in_text and len(parts) >= 3 and parts[2] in MASKS:
      relocations[relative][int(parts[0], 16)] = MASKS[parts[2]]

entries = []
problems = []
for (relative, offset, length, name), addresses in sorted(groups.items()):
  first = min(addresses)
  for address in sorted(addresses):
    function_name, size = inventory[address]
    source_offset = offset + address - first
    available = max(0, min(size, offset + length - source_offset))
    expected_bytes = rom[address - 0x80200000 + 0x1000:
              address - 0x80200000 + 0x1000 + size]
    code = texts[relative][source_offset:source_offset + available]
    if len(code) != available or any(expected_bytes[available:]):
      problems.append((function_name, hex(address), 'size or padding'))
      continue
    patches = []
    for i in range(0, available, 4):
      word = struct.unpack_from('>I', code, i)[0]
      original = struct.unpack_from('>I', expected_bytes, i)[0]
      mask = relocations[relative].get(source_offset + i, 0xffffffff)
      if word & mask != original & mask:
        problems.append((function_name, hex(address), hex(i), hex(word), hex(original)))
        break
      if mask != 0xffffffff:
        patches.append([i, original & ~mask])
    else:
      entries.append([address, relative, source_offset, size,
                      ''.join(f'{position:08x}{bits:08x}' for position, bits in patches)])
if problems:
  parser.error(f'Compiled code differs from the verified ROM: {problems[:3]}.')
if len(entries) != len(inventory):
  parser.error('The generated manifest does not cover every function.')
manifest = {'region': region, 'functions': sorted(entries)}
with tempfile.TemporaryDirectory(dir=root / '.wiswa-ci') as temporary:
  temporary_path = Path(temporary)
  candidate = temporary_path / 'manifest.json'
  candidate.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
  subprocess.run([sys.executable, str(root / 'tools/assemble_code_image.py'),
                  '--region', region, '--build', str(build), '--manifest', str(candidate),
                  '--output', str(temporary_path / 'code-only.z64')], check=True)
args.output.parent.mkdir(parents=True, exist_ok=True)
middle = len(entries) // 2
for index, part in enumerate((entries[:middle], entries[middle:])):
  path = args.output.with_name(f'{args.output.stem}.{index}.json')
  path.write_text(json.dumps({'functions': part, 'region': region}, indent=2,
                             sort_keys=True) + '\n')
  print(f'Recorded {len(part)} verified functions in {path}.')
