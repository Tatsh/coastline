#!/usr/bin/env python3
"""Extract a range of IDO ELF text while retaining its relocations and symbols."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('source', type=Path)
  parser.add_argument('start', type=lambda value: int(value, 0))
  parser.add_argument('end', type=lambda value: int(value, 0))
  parser.add_argument('output', type=Path)
  parser.add_argument('--keep-other-sections', action='store_true')
  args = parser.parse_args()

  data = bytearray(args.source.read_bytes())
  if data[:6] != b'\x7fELF\x01\x02' or len(data) < 52:
    parser.error('Expected a big-endian ELF32 object.')
  table = struct.unpack_from('>I', data, 32)[0]
  entry_size, count, names_index = struct.unpack_from('>HHH', data, 46)
  if entry_size != 40 or table + entry_size * count > len(data) or names_index >= count:
    parser.error('Invalid ELF section table.')

  def header(index: int) -> tuple[int, ...]:
    return struct.unpack_from('>IIIIIIIIII', data, table + index * entry_size)

  _, _, _, _, names_offset, names_size, *_ = header(names_index)
  names = bytes(data[names_offset:names_offset + names_size])

  def section_name(index: int) -> bytes:
    offset = header(index)[0]
    if offset >= len(names):
      parser.error('Invalid ELF section name.')
    stop = names.find(0, offset)
    if stop < 0:
      parser.error('Unterminated ELF section name.')
    return names[offset:stop]

  by_name = {section_name(index): index for index in range(count)}
  if not all(name in by_name for name in (b'.text', b'.symtab', b'.rel.text')):
    parser.error('Expected text, symbol, and text relocation sections.')
  text_index = by_name[b'.text']
  symbols_index = by_name[b'.symtab']
  relocs_index = by_name[b'.rel.text']
  _, text_kind, _, _, text_offset, text_size, *_ = header(text_index)
  _, symbols_kind, _, _, symbols_offset, symbols_size, *_ = header(symbols_index)
  _, relocs_kind, _, _, relocs_offset, relocs_size, *_ = header(relocs_index)
  if text_kind != 1 or symbols_kind != 2 or relocs_kind != 9:
    parser.error('Unexpected ELF section types.')
  if not (0 <= args.start < args.end <= text_size) or args.start % 4 or args.end % 4:
    parser.error('The text range must use complete words within the section.')
  if args.keep_other_sections and args.end != text_size:
    parser.error('The slice with auxiliary sections must include the end of text.')
  if symbols_size % 16 or relocs_size % 8:
    parser.error('Invalid symbol or relocation entries.')

  original_text = bytes(data[text_offset + args.start:text_offset + args.end])
  symbol_count = symbols_size // 16
  symbols = []
  for index in range(symbol_count):
    offset = symbols_offset + index * 16
    _, value, size = struct.unpack_from('>III', data, offset)
    kind = data[offset + 12] & 0xf
    binding = data[offset + 12] >> 4
    section = struct.unpack_from('>H', data, offset + 14)[0]
    symbols.append((value, size, kind, binding, section))

  selected = []
  for at in range(0, relocs_size, 8):
    offset, info = struct.unpack_from('>II', data, relocs_offset + at)
    if not args.start <= offset < args.end:
      continue
    symbol = info >> 8
    if symbol >= symbol_count:
      parser.error('A relocation has an invalid symbol index.')
    value, size, kind, binding, section = symbols[symbol]
    if kind == 3 and section == text_index:
      parser.error('A text relocation uses a section-relative addend.')
    if binding == 0 and section == text_index and not args.start <= value < args.end:
      parser.error('A text relocation crosses a local symbol boundary.')
    if not args.keep_other_sections and binding == 0 and section not in (0, text_index):
      parser.error('A text relocation uses a local symbol from a removed section.')
    selected.append(struct.pack('>II', offset - args.start, info))

  if args.keep_other_sections:
    for index in range(count):
      if index == relocs_index or header(index)[1] not in (4, 9):
        continue
      _, _, _, _, offset, size, _, linked_section, _, entry_length = header(index)
      if linked_section == text_index or not size:
        continue
      if entry_length not in (8, 12) or size % entry_length:
        parser.error('Invalid auxiliary relocation section.')
      for at in range(0, size, entry_length):
        info = struct.unpack_from('>I', data, offset + at + 4)[0]
        symbol = info >> 8
        if symbol >= symbol_count:
          parser.error('An auxiliary relocation has an invalid symbol index.')
        value, _, _, binding, section = symbols[symbol]
        if binding == 0 and section == text_index and not args.start <= value < args.end:
          parser.error('An auxiliary relocation crosses a local text symbol boundary.')

  for index, (value, size, kind, binding, section) in enumerate(symbols):
    offset = symbols_offset + index * 16
    if section == text_index:
      if kind == 3:
        struct.pack_into('>II', data, offset + 4, 0, args.end - args.start)
      elif args.start <= value < args.end and value + size <= args.end:
        struct.pack_into('>I', data, offset + 4, value - args.start)
      elif value < args.start < value + size or value < args.end < value + size:
        parser.error('A text symbol crosses the slice boundary.')
      else:
        struct.pack_into('>II', data, offset + 4, 0, 0)
        struct.pack_into('>H', data, offset + 14, 0)
        if binding == 0:
          struct.pack_into('>I', data, offset, 0)
          data[offset + 12] = 0
    elif not args.keep_other_sections and section not in (0, 0xfff1, 0xfff2):
      struct.pack_into('>II', data, offset + 4, 0, 0)
      struct.pack_into('>H', data, offset + 14, 0)
      if binding == 0:
        struct.pack_into('>I', data, offset, 0)
        data[offset + 12] = 0

  struct.pack_into('>II', data, table + text_index * entry_size + 16,
                   text_offset + args.start, args.end - args.start)
  struct.pack_into('>I', data, table + text_index * entry_size + 32, 4)
  if not args.keep_other_sections:
    for index in range(count):
      if section_name(index) not in (b'', b'.shstrtab', b'.strtab', b'.symtab',
                                     b'.text', b'.rel.text', b'.options', b'.reginfo'):
        struct.pack_into('>I', data, table + index * entry_size + 20, 0)

  relocated = b''.join(selected)
  relocations_start = (len(data) + 3) & ~3
  data.extend(b'\0' * (relocations_start - len(data)))
  data.extend(relocated)
  struct.pack_into('>II', data, table + relocs_index * entry_size + 16,
                   relocations_start, len(relocated))
  if data[text_offset + args.start:text_offset + args.end] != original_text:
    parser.error('Text changed while slicing the object.')
  args.output.write_bytes(data)
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
