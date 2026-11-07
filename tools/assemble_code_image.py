#!/usr/bin/env python3
"""Assemble an IDO Release code image from object text and recorded relocations."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import subprocess
import tempfile
from pathlib import Path

from check_matching import MASKS, ROM_SHA1, binutil

ROOT = Path(__file__).resolve().parent.parent


def object_code(path: Path, work: Path) -> tuple[bytes, dict[int, int]]:
  text_path = work / 'text.bin'
  subprocess.run([binutil('objcopy'), '-O', 'binary', '--only-section=.text',
                  str(path), str(text_path)], check=True)
  code = text_path.read_bytes()
  relocations = {}
  in_text = False
  result = subprocess.run([binutil('readelf'), '-rW', str(path)], check=True,
                          capture_output=True, text=True)
  for line in result.stdout.splitlines():
    if line.startswith('Relocation section'):
      in_text = "'.rel.text'" in line or "'.rela.text'" in line
      continue
    parts = line.split()
    if in_text and len(parts) >= 3 and parts[2] in MASKS:
      relocations[int(parts[0], 16)] = MASKS[parts[2]]
  return code, relocations


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--region', choices=sorted(ROM_SHA1), required=True)
  parser.add_argument('--build', type=Path, required=True)
  parser.add_argument('--manifest', type=Path)
  parser.add_argument('--reference', type=Path)
  parser.add_argument('--output', type=Path, required=True)
  args = parser.parse_args()

  manifest_paths = ([args.manifest] if args.manifest else sorted(
      (ROOT / 'tools' / 'matching').glob(f'code_relocations.{args.region}.*.json')))
  if not manifest_paths:
    parser.error('No relocation manifests were found.')
  manifests = [json.loads(path.read_text()) for path in manifest_paths]
  if any(manifest['region'] != args.region for manifest in manifests):
    parser.error('A relocation manifest describes another region.')
  baseline = json.loads((ROOT / 'tools' / 'matching' / 'code_image.json').read_text())[args.region]
  code_start = baseline['code_start']
  code_end = baseline['code_end']
  image_base = baseline['vram']
  inventory = {
      int(address, 16): (name, int(size, 16))
      for name, address, size in re.findall(
          r'^(\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+)',
          (ROOT / 'tools' / 'splat' / f'symbol_addrs.{args.region}.txt').read_text(), re.M)
  }
  entries = [entry for manifest in manifests for entry in manifest['functions']]
  if {address: size for address, _object, _offset, size, _patches in entries} != {
      address: size for address, (_name, size) in inventory.items()
  } or len(entries) != len(inventory):
    parser.error('The relocation manifest does not cover the function inventory exactly.')

  image = bytearray(baseline['rom_size'])
  claimed = bytearray(code_end - code_start)
  objects = {}
  (ROOT / '.wiswa-ci').mkdir(exist_ok=True)
  with tempfile.TemporaryDirectory(dir=ROOT / '.wiswa-ci') as temporary:
    work = Path(temporary)
    for address, relative, offset, size, patches in entries:
      if relative not in objects:
        path = (args.build / relative).resolve()
        if not path.is_relative_to(args.build.resolve()):
          parser.error(f'Object path escapes the build directory: {relative}.')
        objects[relative] = object_code(path, work)
      code, relocations = objects[relative]
      if offset < 0 or offset > len(code) or offset % 4 or size <= 0 or size % 4:
        parser.error(f'Invalid object text range for {inventory[address][0]}.')
      available = min(size, len(code) - offset)
      data = bytearray(code[offset:offset + available] + bytes(size - available))
      if len(patches) % 16:
        parser.error(f'Invalid relocation data for {inventory[address][0]}.')
      patch_map = {int(patches[i:i + 8], 16): int(patches[i + 8:i + 16], 16)
                   for i in range(0, len(patches), 16)}
      actual = {i for i in range(0, available, 4) if offset + i in relocations}
      if len(patch_map) * 16 != len(patches) or set(patch_map) != actual:
        parser.error(f'Relocations changed for {inventory[address][0]}.')
      for position, bits in patch_map.items():
        mask = relocations[offset + position]
        if bits & mask:
          parser.error(f'Relocation overlaps an instruction in {inventory[address][0]}.')
        word = struct.unpack_from('>I', data, position)[0]
        struct.pack_into('>I', data, position, (word & mask) | bits)
      start = address - image_base
      if start < 0 or code_start + start + size > code_end or any(claimed[start:start + size]):
        parser.error(f'Invalid or overlapping ROM range for {inventory[address][0]}.')
      claimed[start:start + size] = b'\1' * size
      image[code_start + start:code_start + start + size] = data

  code_sha256 = hashlib.sha256(image).hexdigest()
  if code_sha256 != baseline['sha256']:
    parser.error('The assembled code image differs from the verified SHA-256 baseline.')
  if args.reference:
    original = bytearray(args.reference.read_bytes())
    if len(original) != len(image) or hashlib.sha1(original).hexdigest() != ROM_SHA1[args.region]:
      parser.error('The reference ROM does not match the verified regional SHA-1.')
    original[code_start:code_end] = image[code_start:code_end]
    if hashlib.sha1(original).hexdigest() != ROM_SHA1[args.region]:
      parser.error('The assembled ROM differs from the verified original.')
    image = original
  args.output.parent.mkdir(parents=True, exist_ok=True)
  args.output.write_bytes(image)
  print(f'Verified {len(entries)} functions, code SHA-256 {code_sha256}.')
  print(f'Wrote {args.output}.')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
