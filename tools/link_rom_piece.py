#!/usr/bin/env python3
"""Link one IDO object at its original addresses and verify its ROM bytes."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

import yaml

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
  parser.add_argument('--text-skip', type=lambda value: int(value, 0), default=0)
  parser.add_argument('--text-bytes', type=lambda value: int(value, 0))
  parser.add_argument('--data', help='First data symbol in the object.')
  parser.add_argument('--rodata', help='First read-only data symbol in the object.')
  parser.add_argument('--rodata-bytes', type=lambda value: int(value, 0))
  parser.add_argument('--bss', help='First BSS symbol in the object.')
  parser.add_argument('--text-only', action='store_true')
  for section in ('data', 'rodata', 'bss'):
    parser.add_argument(f'--{section}-split', action='append', default=[],
                        metavar='OFFSET:SYMBOL',
                        help=f'Original symbol for a relocated {section} fragment.')
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
  linker_script = (ROOT / 'src' / 'n64' / f'coastline-{args.region}.ld').read_text()
  aliases = dict(re.findall(r'^([A-Za-z_]\w*)\s*=\s*([^;]+);', linker_script, re.M))
  assembly = (ROOT / 'src' / 'n64' / 'asm' / args.region / 'data' / 'data.data.s').read_text()
  aliases.update(re.findall(r'^\s*\.set\s+([A-Za-z_]\w*),\s*([A-Za-z_]\w*)\s*$',
                            assembly, re.M))
  for name, address in re.findall(
      r'^dlabel ([A-Za-z_]\w*)\n\s*/\*\s*[0-9A-Fa-f]+\s+([0-9A-Fa-f]{8})\s', assembly, re.M):
    symbols.setdefault(name, int(address, 16))
  config = yaml.safe_load((ROOT / 'tools' / 'splat' / f'{args.region}.yaml').read_text())
  main_segment = next(segment for segment in config['segments'] if segment.get('name') == 'main')
  symbols['_bss_start'] = next(part['vram'] for part in main_segment['subsegments']
                               if isinstance(part, dict) and part['type'] == 'bss')
  symbols['_bss_size'] = main_segment['bss_size']

  def resolve(name: str, visited: frozenset[str] = frozenset()) -> int | None:
    if name in symbols:
      return symbols[name]
    if name in visited or name not in aliases:
      return None
    match = re.fullmatch(r'([A-Za-z_]\w*|0x[0-9A-Fa-f]+|\d+)'
                         r'(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?', aliases[name].strip())
    if not match:
      return None
    base, delta = match.groups()
    value = (int(base, 0) if base[0].isdigit() else resolve(base, visited | {name}))
    if value is None:
      return None
    symbols[name] = value + (int(delta, 0) if delta else 0)
    return symbols[name]

  undefined = [line.split()[-1] for line in subprocess.run(
      [binutil('nm'), '-u', str(args.object)], check=True, capture_output=True,
      text=True).stdout.splitlines()]
  defined = {
      name: (int(offset, 16), kind)
      for offset, kind, name in re.findall(
          r'^([0-9A-Fa-f]+)\s+([A-Za-z])\s+(\S+)$',
          subprocess.run([binutil('nm'), '-n', '--defined-only', str(args.object)], check=True,
                         capture_output=True, text=True).stdout, re.M)
  }
  selected = {section: value for section in ('text', 'data', 'rodata', 'bss')
              if (value := getattr(args, section))}
  types = {'text': 'Tt', 'data': 'DdVv', 'rodata': 'Rr', 'bss': 'Bb'}
  addresses = {}
  for section, value in selected.items():
    if re.fullmatch(r'0x[0-9A-Fa-f]+', value):
      addresses[section] = int(value, 16)
    elif value in defined and defined[value][1] in types[section] and resolve(value) is not None:
      addresses[section] = resolve(value) - defined[value][0]
    else:
      addresses[section] = None
  missing = sorted([value for section, value in selected.items() if addresses[section] is None] +
                   [name for name in undefined if resolve(name) is None])
  if missing:
    parser.error(f'Missing original addresses: {", ".join(missing)}.')
  if re.fullmatch(r'0x[0-9A-Fa-f]+', args.text):
    parser.error('Text requires a symbol defined by the object.')
  headers = subprocess.run([binutil('readelf'), '-SW', str(args.object)], check=True,
                           capture_output=True, text=True).stdout
  sizes = {section[1:]: int(size, 16) for section, size in re.findall(
      r'\[\s*\d+\]\s+(\.(?:text|data|rodata|bss))\s+\S+\s+\S+\s+\S+\s+'
      r'([0-9A-Fa-f]+)\s', headers)}
  splits = {}
  for section in ('data', 'rodata', 'bss'):
    entries = []
    for spec in getattr(args, f'{section}_split'):
      offset_text, separator, name = spec.partition(':')
      if not separator or not re.fullmatch(r'0x[0-9A-Fa-f]+', offset_text) or name not in symbols:
        parser.error(f'Invalid {section} split: {spec}.')
      split_offset = int(offset_text, 16)
      if not 0 < split_offset < sizes.get(section, 0):
        parser.error(f'{section} split is outside the section: {spec}.')
      entries.append((split_offset, symbols[name] - split_offset))
    if entries and (not args.text_only or entries != sorted(set(entries))):
      parser.error(f'{section} splits require text-only output and unique ordered offsets.')
    splits[section] = entries
  for section in ('text', 'data', 'rodata', 'bss'):
    if bool(sizes.get(section, 0)) != (section in selected):
      parser.error(f'The {section} section and its original address must both be present.')
  if args.text_skip < 0 or args.text_skip >= sizes['text'] or args.text_skip % 4:
    parser.error('The text skip must be aligned and inside the text section.')
  trims = {'text': args.text_bytes, 'rodata': args.rodata_bytes}
  for section, length in trims.items():
    available = sizes.get(section, 0) - (args.text_skip if section == 'text' else 0)
    if length is not None and not 0 < length <= available:
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
    for section, address in addresses.items():
      command.append(f'--defsym={section.upper()}_START={address:#x}')
    command.extend(f'--defsym={name}={symbols[name]:#x}' for name in undefined)
    subprocess.run([*command, '-o', str(elf), str(args.object)], check=True)
    linked_symbols = {
        name: int(address, 16)
        for address, _kind, name in re.findall(
            r'^([0-9A-Fa-f]+)\s+(\S)\s+(\S+)$',
            subprocess.run([binutil('nm'), '-n', str(elf)], check=True, capture_output=True,
                           text=True).stdout, re.M)
    }
    for section, address in addresses.items():
      if linked_symbols.get(f'__piece_{section}_start') != address:
        parser.error(f'Linked {section} is not at its original address.')
    text_end = args.text_skip + (args.text_bytes or sizes['text'] - args.text_skip)
    for symbol in linked_symbols.keys() & symbols.keys():
      if symbol in defined and defined[symbol][1] in 'Tt' and not (
          args.text_skip <= defined[symbol][0] < text_end):
        continue
      if linked_symbols[symbol] != symbols[symbol]:
        parser.error(f'Linked {symbol} differs from its original address.')

    sections = {}
    relocations = []
    internal_calls = []
    if any(splits.values()) or args.text_only:
      relocation_text = subprocess.run([binutil('readelf'), '-rW', str(args.object)],
                                       check=True, capture_output=True, text=True).stdout
      in_text = False
      highs = {section: [] for section in splits}
      for line in relocation_text.splitlines():
        if line.startswith('Relocation section'):
          in_text = "'.rel.text'" in line
        if not in_text:
          continue
        call = re.match(r'^([0-9A-Fa-f]+)\s+\S+\s+R_MIPS_26\s+\S+\s+(\S+)$', line)
        if call and args.text_only and call[2] in defined and defined[call[2]][1] in 'Tt' \
            and call[2] in symbols:
          internal_calls.append((int(call[1], 16), call[2]))
        match = re.match(r'^([0-9A-Fa-f]+)\s+\S+\s+R_MIPS_(HI16|LO16)\s+\S+\s+'
                         r'\.(data|rodata|bss)$', line)
        if not match:
          continue
        offset, kind, section = int(match[1], 16), match[2], match[3]
        if not splits[section]:
          continue
        if kind == 'HI16':
          highs[section].append(offset)
        elif highs[section]:
          relocations.append((section, highs[section].pop(), offset))
        else:
          parser.error(f'{section} relocation has no matching high half.')
      if any(highs.values()) or not relocations:
        if any(splits.values()):
          parser.error('Section split has unmatched or no relocations.')
      raw_text = work / 'original-text.bin'
      subprocess.run([binutil('objcopy'), '-O', 'binary', '--only-section=.text',
                      str(args.object), str(raw_text)], check=True)
      original_text = raw_text.read_bytes()
    written_addresses = dict(addresses)
    written_addresses['text'] += args.text_skip
    for section, address in written_addresses.items():
      if section == 'bss' or (args.text_only and section != 'text'):
        continue
      path = work / f'{section}.bin'
      subprocess.run([binutil('objcopy'), '-O', 'binary', f'--only-section=.{section}',
                      str(elf), str(path)], check=True)
      contents = path.read_bytes()
      if section == 'text':
        if any(splits.values()) or internal_calls:
          patched = bytearray(contents)
          for relocated_section, high_offset, low_offset in relocations:
            high = struct.unpack_from('>H', original_text, high_offset + 2)[0]
            low = struct.unpack_from('>h', original_text, low_offset + 2)[0]
            addend = (high << 16) + low
            base = addresses[relocated_section]
            for split_offset, split_base in splits[relocated_section]:
              if addend >= split_offset:
                base = split_base
            target = base + addend
            struct.pack_into('>H', patched, high_offset + 2, ((target + 0x8000) >> 16) & 0xffff)
            struct.pack_into('>H', patched, low_offset + 2, target & 0xffff)
          for call_offset, name in internal_calls:
            instruction = struct.unpack_from('>I', original_text, call_offset)[0]
            target = symbols[name] + ((instruction & 0x03ffffff) << 2)
            struct.pack_into('>I', patched, call_offset,
                             (instruction & 0xfc000000) | ((target >> 2) & 0x03ffffff))
          contents = bytes(patched)
        contents = contents[args.text_skip:]
      length = trims.get(section)
      if length is not None:
        if section != 'text' and any(contents[length:]):
          parser.error(f'Removed {section} bytes must be zero padding.')
        contents = contents[:length]
      offset = address - IMAGE_BASE + ROM_BASE
      expected = rom[offset:offset + len(contents)]
      if not contents or contents != expected:
        first = next((index for index, (actual, original) in enumerate(zip(contents, expected))
                      if actual != original), 0)
        parser.error(f'Linked {section} differs from the verified ROM at '
                     f'{address + first:#x} ({contents[first:first + 4].hex()} versus '
                     f'{expected[first:first + 4].hex()}).')
      sections[section] = contents

    args.output.mkdir(parents=True, exist_ok=True)
    shutil.copy2(elf, args.output / 'piece.elf')
    for section in ('text', 'data', 'rodata'):
      if section not in sections:
        (args.output / f'{section}.bin').unlink(missing_ok=True)
    for section, contents in sections.items():
      (args.output / f'{section}.bin').write_bytes(contents)
      print(f'{section}: {len(contents)} exact bytes at {written_addresses[section]:#x}')
    (args.output / 'piece.json').write_text(json.dumps({
        'region': args.region,
        'rom_sha1': ROM_SHA1[args.region],
        'sections': {
            section: {'address': written_addresses[section], 'size': len(contents)}
            for section, contents in sections.items()
        },
    }, indent=2, sort_keys=True) + '\n')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
