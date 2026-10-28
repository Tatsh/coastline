#!/usr/bin/env python
"""Write the definitions of the data of the Nintendo 64 front end for the SDL game.

The data is read from the cartridge at start-up, so each definition takes the type of its
declaration in the headers. An array declared without an extent takes the length of the symbol in
the assembly of the USA release, up to the next symbol splat did not make up.
"""
from __future__ import annotations

from pathlib import Path
import argparse
import re

DATA_FILES = ('asm/us/data/data.data.s', 'asm/us/data/rodata.rodata.s', 'asm/us/data/bss.bss.s')
LABEL = re.compile(r'^dlabel (\w+)$')
ADDRESS = re.compile(r'/\* (?:[0-9A-F]+ )?(8[0-9A-F]{7})(?: [0-9A-F]+)? \*/ \.(\w+) ?(\S*)')
WIDTHS = {'word': 4, 'float': 4, 'half': 2, 'short': 2, 'byte': 1}


def symbol_words(root: Path) -> dict[int, str]:
    """Return the operand of each word of the data, by its address."""
    words: dict[int, str] = {}
    for line in (root / DATA_FILES[0]).read_text(encoding='utf-8').splitlines():
        if (match := ADDRESS.search(line)) and match.group(2) == 'word':
            words[int(match.group(1), 16)] = match.group(3)
    return words


def null_terminated_length(words: dict[int, str], address: int) -> int | None:
    """Return the bytes of an array of pointers up to and with its null pointer."""
    end = address
    while end in words:
        if int(words[end], 16) == 0 if words[end].startswith('0x') else False:
            return end + 4 - address
        end += 4
    return None


def symbol_run_length(words: dict[int, str], address: int, labels: set[int]) -> int:
    """Return the bytes of the words from an address that each name a symbol, such as the
    pointers of an array of strings, up to a number or the start of another symbol."""
    end = address
    while end in words and not words[end].startswith('0x') and (end == address or
                                                                 end not in labels):
        end += 4
    return end - address


def symbol_lengths(root: Path) -> dict[str, tuple[int, bool, int]]:
    """Return the bytes of each symbol of the assembly, and whether it is BSS."""
    lengths: dict[str, tuple[int, bool, int]] = {}
    for name in DATA_FILES:
        bss = name.endswith('bss.s')
        labels: list[tuple[str, int]] = []
        current: str | None = None
        end = 0
        for line in (root / name).read_text(encoding='utf-8').splitlines():
            if match := LABEL.match(line.strip()):
                current = match.group(1)
                continue
            if (match := ADDRESS.search(line)) is None:
                continue
            address = int(match.group(1), 16)
            directive, operand = match.group(2), match.group(3)
            if current is not None:
                labels.append((current, address))
                current = None
            if directive == 'space':
                end = address + int(operand, 0)
            elif directive == 'asciz':
                end = address + len(operand.encode()) + 1
            else:
                end = address + WIDTHS.get(directive, 4)
        real = [(label, address) for label, address in labels if not label.startswith('D_')]
        for index, (label, address) in enumerate(real):
            following = real[index + 1][1] if index + 1 < len(real) else end
            lengths[label] = (following - address, bss, address)
    return lengths


DECLARATION = r'extern\s+((?:const\s+)?[\w\s\*]+?)\s*\b{}\b\s*((?:\[[^\]]*\])*)\s*;'


def find_declaration(root: Path, symbol: str) -> tuple[str, str, str] | None:
    """Return the header, the type, and the extents of the declaration of a symbol."""
    pattern = re.compile(DECLARATION.format(re.escape(symbol)))
    for header in sorted((root / 'src').rglob('*.h')):
        if 'win32' in header.parts or 'sdl' in header.parts:
            continue
        if match := pattern.search(header.read_text(encoding='utf-8')):
            return (str(header.relative_to(root / 'src')), ' '.join(match.group(1).split()),
                    match.group(2))
    return None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('symbols', type=Path, help='file of the names of the symbols, one a line')
    parser.add_argument('output', type=Path, help='C++ source to write')
    parser.add_argument('--root', type=Path, default=Path(), help='root of the repository')
    parser.add_argument('--extents',
                        type=Path,
                        help='file of lines "symbol [extent]" for arrays whose length splat '
                        'does not know, which replace the extents of the declaration')
    args = parser.parse_args()
    lengths = symbol_lengths(args.root)
    words = symbol_words(args.root)
    label_addresses = {address for _, _, address in lengths.values()}
    overrides: dict[str, str] = {}
    if args.extents is not None:
        for line in args.extents.read_text(encoding='utf-8').splitlines():
            if line.strip() and not line.startswith('#'):
                symbol, extent = line.split(maxsplit=1)
                overrides[symbol] = extent.strip()
    headers: set[str] = set()
    definitions: list[str] = []
    reads: list[tuple[str, bool]] = []
    # The front end renames some symbols with N64 appended, and the definitions take the names of
    # the declarations, which the same renames reach.
    symbols = {name.removesuffix('N64') for name in args.symbols.read_text(encoding='utf-8').split()}
    # The data a datum addresses is defined too, so that its pointers have somewhere to go, and
    # the routines it addresses are recorded for its pointers to routines.
    routines_of_release = {
        line.split(' = ')[0]
        for line in (args.root / 'tools/splat/symbol_addrs.us.txt').read_text(
            encoding='utf-8').splitlines() if 'type:func' in line
    }
    routines: set[str] = set()
    pending = list(symbols)
    while pending:
        symbol = pending.pop()
        if symbol not in lengths:
            continue
        length, bss, address = lengths[symbol]
        for offset in range(0, length if not bss else 0, 4):
            operand = words.get(address + offset, '0x0')
            if operand in routines_of_release:
                routines.add(operand)
            elif not operand.startswith('0x') and operand not in symbols:
                symbols.add(operand)
                pending.append(operand)
    for symbol in sorted(symbols):
        declaration = find_declaration(args.root, symbol)
        if declaration is None or (symbol not in lengths and symbol not in overrides):
            definitions.append(f'// {symbol}: no declaration or no length in the assembly.')
            continue
        header, kind, extents = declaration
        # A symbol outside the data of the assembly, given an extent, is zero like the BSS.
        length, bss, address = lengths.get(symbol, (0, True, 0))
        headers.add(header)
        if symbol in overrides and extents.startswith('[]'):
            extents = overrides[symbol] + extents[2:]
        elif extents == '[]' and kind == 'MenuItem *' and (
                terminated := null_terminated_length(words, address)) is not None:
            extents = f'[{terminated // 4}]'
        elif extents == '[]' and kind == 'const char *' and (run := symbol_run_length(
                words, address, label_addresses)) > 0:
            extents = f'[{run // 4}]'
        elif extents.startswith('[]'):
            element = f'{kind}{extents[2:]}'
            extents = f'[{length} / coastline::ConsoleSizeOf<{element}>()]{extents[2:]}'
        if kind.startswith('const ') and '*' not in kind:
            # A constant object of bytes, such as a table of characters, cannot be read into at
            # start-up, so it takes the bytes of the cartridge as its initializer.
            data = b''.join(
                int(words[address + offset], 16).to_bytes(4, 'big')
                for offset in range(0, length, 4))[:length].rstrip(b'\0')
            definitions.append(f'{kind} {symbol} = {{{{{", ".join(hex(b) for b in data)}}}}};')
            continue
        definitions.append(f'{kind} {symbol}{extents};')
        reads.append((symbol, bss))
    lines = [
        '// Generated by tools/sdl/n64frontenddata.py. Do not edit.',
        '',
        *(f'#include "{header}"' for header in sorted(headers)),
        '#include "sdl/cartridgedata.h"',
        '#include "sdl/n64frontendlayouts.h"',
        '',
        *definitions,
        '',
        'void RegisterN64FrontEndRoutines(void);',
        '',
        'void ReadN64FrontEndData(void) {',
        '    RegisterN64FrontEndRoutines();',
        # The BSS is zero and is only addressed by code, so only the data the cartridge
        # initializes is recorded and read.
        *(f'    coastline::RegisterCartridgeSymbol("{s}", {s});' for s, bss in reads if not bss),
        *(f'    coastline::ReadCartridgeSymbol("{s}", {s});' for s, bss in reads if not bss),
        '}',
        '',
    ]
    args.output.write_text('\n'.join(lines), encoding='utf-8')
    # The routines are declared in a source of their own, which includes no header, so that a
    # declaration here cannot conflict with one there.
    routine_lines = [
        '// Generated by tools/sdl/n64frontenddata.py. Do not edit.',
        '',
        '#include "sdl/cartridgedata.h"',
        '',
        'extern "C" {',
        *(f'void {r}(void);' for r in sorted(routines)),
        '}',
        '',
        'void RegisterN64FrontEndRoutines(void) {',
        *(f'    coastline::CartridgeData::RegisterRoutine("{r}", {r});' for r in sorted(routines)),
        '}',
        '',
    ]
    args.output.with_name('n64frontendroutines.cpp').write_text('\n'.join(routine_lines),
                                                                 encoding='utf-8')


if __name__ == '__main__':
    main()
