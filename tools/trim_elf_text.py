#!/usr/bin/env python3
"""Trim verified zero padding from an IDO ELF text section without changing relocations."""
from __future__ import annotations

import argparse
import struct
from pathlib import Path


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('source', type=Path)
  parser.add_argument('original_size', type=lambda value: int(value, 0))
  parser.add_argument('new_size', type=lambda value: int(value, 0))
  parser.add_argument('output', type=Path)
  args = parser.parse_args()

  data = bytearray(args.source.read_bytes())
  if data[:6] != b'\x7fELF\x01\x02' or len(data) < 52:
    parser.error('Expected a big-endian ELF32 object.')
  section_table = struct.unpack_from('>I', data, 32)[0]
  section_size, section_count, strings_index = struct.unpack_from('>HHH', data, 46)
  if section_size != 40 or section_table + section_size * section_count > len(data):
    parser.error('Invalid ELF section table.')
  if strings_index >= section_count:
    parser.error('Invalid ELF section-name table.')

  def header(index: int) -> tuple[int, int, int, int, int, int]:
    return struct.unpack_from('>IIIIII', data, section_table + index * section_size)

  _, _, _, _, names_offset, names_size = header(strings_index)
  names = data[names_offset:names_offset + names_size]

  def section_name(index: int) -> bytes:
    start = header(index)[0]
    if start >= len(names):
      parser.error('Invalid ELF section name.')
    end = names.find(0, start)
    return names[start:end]

  text_indices = [index for index in range(section_count) if section_name(index) == b'.text']
  if len(text_indices) != 1:
    parser.error('Expected exactly one text section.')
  text_index = text_indices[0]
  _, kind, _, _, text_offset, text_size = header(text_index)
  if kind != 1 or text_size != args.original_size or not 0 <= args.new_size <= text_size:
    parser.error('Unexpected text section size or type.')
  if args.new_size % 4 or any(data[text_offset + args.new_size:text_offset + text_size]):
    parser.error('Removed text must consist of complete zero words.')

  for index in range(section_count):
    _, kind, _, _, offset, size = header(index)
    if section_name(index) == b'.rel.text':
      if kind != 9 or size % 8:
        parser.error('Invalid text relocations.')
      if any(struct.unpack_from('>I', data, offset + at)[0] >= args.new_size
             for at in range(0, size, 8)):
        parser.error('A relocation references the removed text.')
    if kind == 2:
      if size % 16:
        parser.error('Invalid symbol table.')
      for at in range(0, size, 16):
        _, value, length = struct.unpack_from('>III', data, offset + at)
        symbol_type = data[offset + at + 12] & 0xF
        section = struct.unpack_from('>H', data, offset + at + 14)[0]
        if section == text_index and symbol_type == 3 and value == 0 and length == text_size:
          struct.pack_into('>I', data, offset + at + 8, args.new_size)
          continue
        if section == text_index and value + length > args.new_size:
          parser.error('A symbol extends into the removed text.')

  struct.pack_into('>I', data, section_table + text_index * section_size + 20, args.new_size)
  args.output.write_bytes(data)
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
