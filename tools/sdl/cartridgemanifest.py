"""Write the manifest of the assets of a cartridge image, one line per asset.

Each line is the offset of the asset on the cartridge, its size there, ``zlib`` or ``raw``, and its
name. The assets of the tables of the game take names from the table, the rest from the label of
the data that addresses them, a segment of the linker script, or a constant of a source. The lines
cover every byte from the first asset to the end of the image.
"""
from __future__ import annotations

from pathlib import Path
import argparse
import re
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parents[2]
VRAM = 0x80200000
TEXT_ROM = 0x1000
RELEASES = {'us': ('USA', 'TGR_REGION_US'), 'jp': ('JPN', 'TGR_REGION_JP'),
            'eu': ('EUR', 'TGR_REGION_EU')}
CAR_STRIDE = 0x60
TRACK_STRIDE = 0x17C
TRACK_COUNT = 11
SOUND_EFFECTS = 16
SOUND_EFFECT_STRIDE = 0x18
MUSIC_ENTRIES = 12


def big32(rom: bytes, at: int) -> int:
    return struct.unpack_from('>I', rom, at)[0]


def string_at(rom: bytes, address: int) -> str:
    at = address - VRAM + TEXT_ROM
    return rom[at:rom.index(b'\0', at)].decode('ascii')


def slug(text: str) -> str:
    return re.sub(r'[^a-z0-9]+', '-', text.lower()).strip('-')


def ghidra_address(name: str, program: str) -> int:
    """The address of a datum in a release, from the Doxygen block of its declaration."""
    pattern = re.compile(r'/\*\*((?:(?!\*/).)*?)\*/\s*extern[^;]*?\b' + re.escape(name) + r'\b',
                         re.S)
    for header in (ROOT / 'src').rglob('*.h'):
        match = pattern.search(header.read_text(encoding='utf-8'))
        if match:
            found = re.search(rf'@ghidraAddress {program}: (0x[0-9a-fA-F]+)', match.group(1))
            if found:
                return int(found.group(1), 16)
    raise SystemExit(f'{name} has no {program} address')


def scan(rom: bytes, start: int) -> list[tuple[int, int]]:
    """The zlib assets, each a header of the length and the inflated length before zlib chunks."""
    assets = []
    position = start
    while position + 12 < len(rom):
        total = big32(rom, position)
        if (16 <= total <= len(rom) - position and big32(rom, position + 4) != 0
                and rom[position + 12] == 0x78 and (rom[position + 12] << 8 | rom[position + 13])
                % 31 == 0):
            try:
                zlib.decompressobj().decompress(rom[position + 12:position + total])
            except zlib.error:
                position += 2
                continue
            if total % 2:
                # The importer writes the asset alone and discards a byte of padding after it.
                raise SystemExit(f'the asset at {position:#x} has an odd size')
            assets.append((position, total))
            position += (total + 1) & ~1
        else:
            position += 2
    return assets


def data_labels(release: str) -> list[tuple[int, str, list[tuple[int, int]]]]:
    """Each label of the data segment, its address, and its words as (address, value)."""
    labels: list[tuple[int, str, list[tuple[int, int]]]] = []
    for line in (ROOT / 'asm' / release / 'data' / 'data.data.s').read_text().splitlines():
        if line.startswith('dlabel '):
            labels.append((0, line.split()[1], []))
            continue
        word = re.match(r'\s*/\* [0-9A-F]+ ([0-9A-F]{8}) ([0-9A-F]{8}) \*/ \.(word|float)', line)
        if word and labels:
            address, value = int(word.group(1), 16), int(word.group(2), 16)
            if not labels[-1][2]:
                labels[-1] = (address, labels[-1][1], labels[-1][2])
            labels[-1][2].append((address, value))
    return labels


def linker_segments(release: str) -> dict[int, str]:
    segments = {}
    for line in (ROOT / 'src' / 'n64' / f'coastline-{release}.ld').read_text().splitlines():
        match = re.match(r'_(\w+)SegmentRomStart = (0x[0-9A-Fa-f]+);', line)
        if match:
            segments[int(match.group(2), 16)] = match.group(1)
    return segments


def source_constants(release: str) -> dict[int, str]:
    """The ROM_ constants of the sources, by the region of their #if block."""
    region = RELEASES[release][1]
    constants: dict[int, str] = {}
    for source in (ROOT / 'src' / 'n64').glob('*.c'):
        active = True
        matched = False
        for line in source.read_text(encoding='utf-8').splitlines():
            condition = re.match(r'#(if|elif) TGR_REGION == (\w+)', line)
            if condition:
                matched = matched if condition.group(1) == 'elif' else False
                active = not matched and condition.group(2) == region
                matched = matched or active
            elif line.startswith('#else'):
                active = not matched
            elif line.startswith('#endif'):
                active = True
                matched = False
            match = re.match(r'#define ROM_(\w+) (0x[0-9A-Fa-f]+)\b', line)
            if match and active:
                constants.setdefault(int(match.group(2), 16), match.group(1).lower())
    return constants


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('release', choices=sorted(RELEASES))
    parser.add_argument('rom', type=Path)
    parser.add_argument('first', type=lambda text: int(text, 0), help='offset of the first asset')
    arguments = parser.parse_args()
    rom = arguments.rom.read_bytes()
    program = RELEASES[arguments.release][0]
    names: dict[int, str] = {}
    ends: dict[int, int] = {}

    def name(offset: int, text: str, end: int | None = None) -> None:
        names.setdefault(offset, text)
        if end is not None:
            ends.setdefault(offset, end)

    cars = ghidra_address('g_aCarModels', program) - VRAM + TEXT_ROM
    for car in range(big32(rom, ghidra_address('g_dwCarCount', program) - VRAM + TEXT_ROM)):
        entry = cars + car * CAR_STRIDE
        label = slug(string_at(rom, big32(rom, entry)))
        if big32(rom, entry + 0x18):
            name(big32(rom, entry + 0x14), f'cars/{label}/model')
        name(big32(rom, entry + 0x20), f'cars/{label}/dashboard', big32(rom, entry + 0x24))

    tracks = ghidra_address('g_aTrackDescriptors', program) - VRAM + TEXT_ROM
    for track in range(TRACK_COUNT):
        entry = tracks + track * TRACK_STRIDE
        if big32(rom, entry) == 0 or big32(rom, entry + 0x18) == 0:
            continue
        label = f'tracks/{slug(string_at(rom, big32(rom, entry)))}'
        if big32(rom, entry + 0x0C):
            name(big32(rom, entry + 0x0C), f'{label}/menu-model')
        geometry = big32(rom, entry + 0x14)
        name(geometry, f'{label}/geometry')
        name(geometry + big32(rom, geometry), f'{label}/textures', big32(rom, entry + 0x18))
        # A section of route is a header of 0x20 bytes before its zlib stream.
        for field, section in ((0x1C, 'route-a'), (0x24, 'route-b')):
            header = big32(rom, entry + field)
            name(header, f'{label}/{section}-header')
            name(header + 0x20, f'{label}/{section}')

    # The sound effects by the names of their enumeration, the engine first.
    header = (ROOT / 'src' / 'common' / 'soundeffect.h').read_text(encoding='utf-8')
    effects = [
        slug(match) for match in re.findall(r'^\s+SOUND_EFFECT_(\w+)(?: = \w+)?,', header, re.M)
        if match != 'NONE'
    ]
    sounds = ghidra_address('g_aSoundEffects', program) - VRAM + TEXT_ROM
    for index, effect in enumerate(effects[:SOUND_EFFECTS]):
        name(big32(rom, sounds + index * SOUND_EFFECT_STRIDE + 4), f'sounds/{effect}')

    music = ghidra_address('g_adwTrackMusicRom', program) - VRAM + TEXT_ROM
    for index in range(MUSIC_ENTRIES):
        name(big32(rom, music + index * 4), f'music/track-{index}')

    for offset, segment in linker_segments(arguments.release).items():
        name(offset, f'segments/{segment}')
    for offset, constant in source_constants(arguments.release).items():
        name(offset, f'constants/{constant}')

    zlib_assets = scan(rom, arguments.first)
    starts = {offset for offset, _ in zlib_assets}
    labels = data_labels(arguments.release)
    word_at = {address: value for _, _, words in labels for address, value in words}

    def item_text(address: int) -> str | None:
        """The text of the menu item whose romStart is the word at an address."""
        pointer = word_at.get(address - 0x0C, 0)
        if not VRAM <= pointer < VRAM + len(rom):
            return None
        at = pointer - VRAM + TEXT_ROM
        end = rom.find(b'\0', at, at + 64)
        text = rom[at:end] if end > at else b''
        return slug(text.decode('ascii')) if text.isascii() and slug(text.decode('ascii')) else None

    # The list that points to a menu item, such as g_apTiresOptions.
    lists = {value: label for _, label, words in labels for _, value in words
             if label.startswith('g_ap')}
    # The items of a list are adjacent. An item takes the list of the item before it.
    item_size = 0x14
    group = ''
    previous = 0
    for address in sorted(word_at):
        value = word_at[address]
        text = item_text(address) if value in starts else None
        if (text and word_at.get(address + 4, 0) > value and
                word_at.get(address - 8, 1 << 16) < 1 << 16):
            listed = lists.get(address - 0x0C)
            if listed:
                group = slug(re.sub(r'^g_ap|(Options|MenuItems)$', '', listed)) + '/'
            elif address - previous != item_size:
                group = ''
            previous = address
            name(value, f'menu-items/{group}{text}')

    for _, label, words in labels:
        values = [value for _, value in words]
        # A word after the start of an asset that addresses a later asset is the end of the first,
        # except in the table of music, a list of starts.
        hits = [
            value for index, value in enumerate(values)
            if value in starts and value not in names and
            (label == 'g_adwTrackMusicRom' or index == 0 or values[index - 1] not in starts or
             values[index - 1] >= value)
        ]
        hits = list(dict.fromkeys(hits))
        for index, value in enumerate(hits):
            name(value, f'data/{label}' + (f'-{index}' if len(hits) > 1 else ''))

    lines = []
    position = arguments.first

    def raw(until: int) -> None:
        nonlocal position
        while position < until:
            end = min([o for o in names if position < o < until] + [until, ends.get(position, until)])
            gap = rom[position:end]
            label = names.get(position) or (
                'padding' if not any(gap) else f'unreferenced/{position:06x}')
            lines.append(f'{position:06X} {end - position} raw {label}')
            position = end

    for offset, total in zlib_assets:
        raw(offset)
        label = names.get(offset, f'unreferenced/{offset:06x}')
        lines.append(f'{offset:06X} {total} zlib {label}')
        position = offset + ((total + 1) & ~1)
    raw(len(rom))
    print('\n'.join(lines))
    return 0


if __name__ == '__main__':
    sys.exit(main())
