#!/usr/bin/env python3
"""Link one IDO object at its original addresses and verify its ROM bytes."""
from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from check_matching import ROM_SHA1, binutil

ROOT = Path(__file__).resolve().parent.parent
IMAGE_BASE = 0x80200000
ROM_BASE = 0x1000


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--region', choices=sorted(ROM_SHA1), required=True)
  parser.add_argument('--rom', type=Path, required=True)
  parser.add_argument('--object', type=Path, required=True)
  parser.add_argument('--text', required=True, help='First text symbol in the object.')
  parser.add_argument('--text-bytes', type=lambda value: int(value, 0))
  parser.add_argument('--data', help='First data symbol in the object.')
  parser.add_argument('--rodata', help='First read-only data symbol in the object.')
  parser.add_argument('--rodata-bytes', type=lambda value: int(value, 0))
  parser.add_argument('--bss', help='First BSS symbol in the object.')
  parser.add_argument('--output', type=Path, required=True, help='Directory for verified sections.')
  args = parser.parse_args()

  rom = args.rom.read_bytes()
  if hashlib.sha1(rom).hexdigest() != ROM_SHA1[args.region]:
    parser.error('The ROM does not match the verified regional SHA-1.')
  symbols = {
      name: int(address, 16)
      for name, address in re.findall(
          r'^(\S+) = 0x([0-9A-Fa-f]+);',
          (ROOT / 'tools' / 'splat' / f'symbol_addrs.{args.region}.txt').read_text(),
          re.M)
  }
  undefined = [line.split()[-1] for line in subprocess.run(
      [binutil('nm'), '-u', str(args.object)], check=True, capture_output=True,
      text=True).stdout.splitlines()]
  selected = {section: symbol for section in ('text', 'data', 'rodata', 'bss')
              if (symbol := getattr(args, section))}
  missing = sorted(set([*selected.values(), *undefined]) - symbols.keys())
  if missing:
    parser.error(f'Missing original addresses: {", ".join(missing)}.')
  headers = subprocess.run([binutil('readelf'), '-SW', str(args.object)], check=True,
                           capture_output=True, text=True).stdout
  sizes = {section[1:]: int(size, 16) for section, size in re.findall(
      r'\[\s*\d+\]\s+(\.(?:text|data|rodata|bss))\s+\S+\s+\S+\s+\S+\s+'
      r'([0-9A-Fa-f]+)\s', headers)}
  for section in ('text', 'data', 'rodata', 'bss'):
    if bool(sizes.get(section, 0)) != (section in selected):
      parser.error(f'The {section} section and its original address must both be present.')
  trims = {'text': args.text_bytes, 'rodata': args.rodata_bytes}
  for section, length in trims.items():
    if length is not None and not 0 < length <= sizes.get(section, 0):
      parser.error(f'{section} byte count is outside the object section.')

  (ROOT / '.wiswa-ci').mkdir(exist_ok=True)
  with tempfile.TemporaryDirectory(dir=ROOT / '.wiswa-ci') as temporary:
    work = Path(temporary)
    script = work / 'piece.ld'
    script.write_text('OUTPUT_ARCH(mips)\nSECTIONS\n{\n' + ''.join(
        f'    . = {section.upper()}_START;\n'
        f'    .{section} : SUBALIGN(4) '
        f'{{ __piece_{section}_start = .; *(.{section}) }}\n'
        for section in selected) + '}\n')
    elf = work / 'piece.elf'
    command = [binutil('ld'), '-T', str(script), '-e', args.text]
    for section, symbol in selected.items():
      command.append(f'--defsym={section.upper()}_START={symbols[symbol]:#x}')
    command.extend(f'--defsym={name}={symbols[name]:#x}' for name in undefined)
    subprocess.run([*command, '-o', str(elf), str(args.object)], check=True)
    linked_symbols = {
        name: int(address, 16)
        for address, _kind, name in re.findall(
            r'^([0-9A-Fa-f]+)\s+(\S)\s+(\S+)$',
            subprocess.run([binutil('nm'), '-n', str(elf)], check=True, capture_output=True,
                           text=True).stdout, re.M)
    }
    for section, symbol in selected.items():
      if linked_symbols.get(f'__piece_{section}_start') != symbols[symbol]:
        parser.error(f'Linked {section} is not at its original address.')
    for symbol in linked_symbols.keys() & symbols.keys():
      if linked_symbols[symbol] != symbols[symbol]:
        parser.error(f'Linked {symbol} differs from its original address.')

    sections = {}
    for section, symbol in selected.items():
      if section == 'bss':
        continue
      path = work / f'{section}.bin'
      subprocess.run([binutil('objcopy'), '-O', 'binary', f'--only-section=.{section}',
                      str(elf), str(path)], check=True)
      contents = path.read_bytes()
      length = trims.get(section)
      if length is not None:
        if any(contents[length:]):
          parser.error(f'Removed {section} bytes must be zero padding.')
        contents = contents[:length]
      offset = symbols[symbol] - IMAGE_BASE + ROM_BASE
      if not contents or contents != rom[offset:offset + len(contents)]:
        parser.error(f'Linked {section} differs from the verified ROM at {symbols[symbol]:#x}.')
      sections[section] = contents

    args.output.mkdir(parents=True, exist_ok=True)
    shutil.copy2(elf, args.output / 'piece.elf')
    for section in ('text', 'data', 'rodata'):
      if section not in sections:
        (args.output / f'{section}.bin').unlink(missing_ok=True)
    for section, contents in sections.items():
      (args.output / f'{section}.bin').write_bytes(contents)
      print(f'{section}: {len(contents)} exact bytes at {symbols[selected[section]]:#x}')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
