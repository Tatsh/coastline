#!/usr/bin/env python3
"""Verify IDO output against ROM-matched instruction digests."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IDO = Path(os.environ.get('IDO_ROOT', ROOT / '.wiswa-ci/ido/5.3'))
BIN = Path(os.environ.get('N64_INST', ROOT / '.wiswa-ci/toolchains/n64/opt/libdragon')) / 'bin'
ROMS = {'us': 'usa.z64', 'jp': 'jpn.z64', 'eu': 'eur.z64'}
ROM_SHA1 = {'us': 'bfc51f086ebdb78904dc24ad0c3a2d946e26d022',
            'jp': 'e0415b87c5a5b69ac581e79137406bdaa79b354a',
            'eu': '9fae5d98fe3464f00d59034690df90118646eb95'}
REGION_VALUES = {'us': 1, 'jp': 2, 'eu': 3}
REGION = 'us'
ENCODER: Path | None = None
DEFINES = ['-DTGR_PLATFORM_N64=1', '-DTGR_PLATFORM_WIN=2', '-DTGR_PLATFORM=1', '-DTGR_REGION_US=1',
           '-DTGR_REGION_JP=2', '-DTGR_REGION_EU=3', '-DTGR_REGION=1', '-DTGR_RENDERER_D3D=1',
           '-DTGR_RENDERER_GLIDE=2', '-DTGR_RENDERER=0', '-D_LANGUAGE_C', '-DF3DEX_GBI',
           '-D_MIPS_SZLONG=32', '-D__sgi', '-DNDEBUG']
MASKS = {'R_MIPS_26': 0xFC000000, 'R_MIPS_HI16': 0xFFFF0000, 'R_MIPS_LO16': 0xFFFF0000}


def run(cmd: list[str]) -> str:
    return subprocess.run(cmd, capture_output=True, text=True, check=True).stdout


def binutil(name: str) -> str:
    for prefix in ('mips64-elf-', 'mips64-libdragon-elf-'):
        path = BIN / f'{prefix}{name}'
        if path.is_file():
            return str(path)
    raise FileNotFoundError(f'Cannot find {name} under {BIN}.')


def originals() -> dict[str, tuple[int, int]]:
    path = ROOT / 'tools' / 'splat' / f'symbol_addrs.{REGION}.txt'
    text = path.read_text(encoding='utf-8')
    return {m.group(1): (int(m.group(2), 16), int(m.group(3), 16))
            for m in re.finditer(r'^(\S+) = 0x([0-9A-Fa-f]+); // type:func size:0x([0-9A-Fa-f]+)',
                                 text, re.M)}


def functions(obj: Path) -> list[tuple[str, list[int], list[int]]]:
    """Return (name, words, masks) for each function defined in the .text of an object."""
    with tempfile.TemporaryDirectory(dir=ROOT / '.wiswa-ci') as tmp:
        text = Path(tmp) / 'text.bin'
        run([binutil('objcopy'), '-O', 'binary', '--only-section=.text', str(obj),
             str(text)])
        code = text.read_bytes()
    relocs = {}
    section = None
    for line in run([binutil('readelf'), '-rW', str(obj)]).splitlines():
        if line.startswith('Relocation section'):
            section = line.split("'")[1]
            continue
        parts = line.split()
        if section in ('.rel.text', '.rela.text') and len(parts) >= 3 and parts[2] in MASKS:
            relocs[int(parts[0], 16)] = MASKS[parts[2]]
    result = []
    for line in run([binutil('readelf'), '-sW', str(obj)]).splitlines():
        parts = line.split()
        if len(parts) == 8 and parts[3] == 'FUNC' and parts[6] != 'UND':
            offset, size = int(parts[1], 16), int(parts[2])
            words = [struct.unpack('>I', code[offset + i:offset + i + 4])[0]
                     for i in range(0, size, 4)]
            masks = [relocs.get(offset + i, 0xFFFFFFFF) for i in range(0, size, 4)]
            result.append((parts[7], words, masks, offset))
    # IDO gives a static function no symbol. The code between the end of one global function and
    # the start of the next is the statics of the source between them, in source order.
    result.sort(key=lambda item: item[3])
    gaps = []
    if result and result[0][3] > 0:
        # Statics before the first global function: the original's range ends where it starts.
        first = result[0]
        gap = [struct.unpack('>I', code[i:i + 4])[0] for i in range(0, first[3], 4)]
        masks = [relocs.get(i, 0xFFFFFFFF) for i in range(0, first[3], 4)]
        gaps.append((f'^..{first[0]}', gap, masks, 0))
    if result:
        last = result[-1]
        end = last[3] + 4 * len(last[1])
        tail = [struct.unpack('>I', code[i:i + 4])[0] for i in range(end, len(code) - 3, 4)]
        while tail and tail[-1] == 0:
            tail.pop()
        if tail:
            masks = [relocs.get(end + 4 * i, 0xFFFFFFFF) for i in range(len(tail))]
            gaps.append((f'{last[0]}..$', tail, masks, end))
    for (name, words, _, offset), (next_name, *_rest) in zip(result, result[1:]):
        end = offset + 4 * len(words)
        start = _rest[2]
        if start > end:
            gap = [struct.unpack('>I', code[i:i + 4])[0] for i in range(end, start, 4)]
            masks = [relocs.get(i, 0xFFFFFFFF) for i in range(end, start, 4)]
            gaps.append((f'{name}..{next_name}', gap, masks, end))
    return result + gaps


def register_fields(word: int) -> int:
    """The bits of the register numbers of an instruction, by its format."""
    opcode = word >> 26
    if opcode == 0:  # SPECIAL: rs, rt, rd
        return 0x03FFF800
    if opcode == 1:  # REGIMM: rs; rt selects the operation
        return 0x03E00000
    if opcode in (2, 3):  # J, JAL
        return 0
    if opcode == 0x11:  # COP1: ft, fs, fd; fmt selects the operation
        return 0x001FFFC0
    return 0x03FF0000  # I-type: rs, rt


def matches(words: list[int], masks: list[int], address: int, size: int,
            rom: bytes) -> tuple[bool | str, int]:
    """Return the match status and exact instruction bytes in the reference range."""
    start = address - 0x80200000 + 0x1000
    theirs = [struct.unpack('>I', rom[start + i:start + i + 4])[0] for i in range(0, size, 4)]
    words = list(words)
    while words and words[-1] == 0 and len(words) > len(theirs):
        words.pop()
    matched = 4 * sum((m & k) == (t & k) for m, t, k in zip(words, theirs, masks))
    if len(words) != len(theirs):
        return False, matched
    if all((m & k) == (t & k) for m, t, k in zip(words, theirs, masks)):
        return True, matched
    if all((m & k & ~register_fields(m)) == (t & k & ~register_fields(t))
           for m, t, k in zip(words, theirs, masks)):
        return 'registers', matched
    return False, matched


def digest(words: list[int], masks: list[int]) -> str:
    value = hashlib.sha256(struct.pack('>I', len(words)))
    for word, mask in zip(words, masks):
        value.update(struct.pack('>II', mask, word & mask))
    return value.hexdigest()


def flag_sets(source: Path) -> list[list[str]]:
    """The alternative IDO flags of a source, from tools/ido_flags.txt."""
    import fnmatch
    relative = source.relative_to(ROOT).as_posix()
    for line in (ROOT / 'tools' / 'ido_flags.txt').read_text(encoding='utf-8').splitlines():
        if not line.strip() or line.startswith('#'):
            continue
        pattern, flags = line.split(None, 1)
        if fnmatch.fnmatch(relative, pattern):
            return [alternative.split() for alternative in flags.split('|')]
    return [['-O2']]


def compile_ido(source: Path, flags: list[str], obj: Path) -> bool:
    if not any(flag.startswith('-mips') for flag in flags):
        flags = ['-mips2', *flags]
    compiled = source
    if REGION == 'jp' and not source.read_bytes().isascii():
        compiled = obj.with_name(source.name)
        if ENCODER is None:
            raise RuntimeError('Japanese text requires --encoder.')
        subprocess.run([str(ENCODER), str(source), str(compiled)], check=True)
    result = subprocess.run(
        [str(IDO / 'cc'), '-c', '-nostdinc', '-non_shared', '-G', '0',
         '-Xcpluscomm', *flags, '-Wab,-r4300_mul', f'-I{ROOT / "tools" / "ido-include"}',
         f'-I{ROOT / "src"}', f'-I{ROOT / "src" / "n64"}',
         f'-I{ROOT / "src" / "common" / "libultra" / "include"}',
         f'-I{ROOT / "src" / "n64" / "libultra" / "include"}',
         f'-I{ROOT / "3rdparty" / "zlib-1.0.4"}', f'-I{source.parent}', *DEFINES, '-o', str(obj),
         str(compiled)], capture_output=True, text=True, check=False)
    return result.returncode == 0


def assemble(source: Path, obj: Path) -> bool:
    result = subprocess.run(
        [binutil('gcc'), '-c', '-march=vr4300', '-mabi=32', '-mno-abicalls', '-fno-pic',
         '-G0', f'-I{ROOT / "src" / "common" / "libultra" / "include"}', f'-I{ROOT / "src"}',
         *DEFINES, '-x', 'assembler-with-cpp', '-o', str(obj), str(source)],
        capture_output=True, text=True,
        check=False)
    return result.returncode == 0


def candidates(obj: Path, table: dict[str, tuple[int, int]],
               by_address: list[tuple[int, str]]):
    for name, words, masks, _offset in functions(obj):
        if '..' not in name:
            if name in table:
                address, size = table[name]
                yield [f'0x{address:08x}'], words, masks, address, size
            continue
        before, after = name.split('..')
        if before == '^' and after in table:
            end = table[after][0]
            start = end - 4 * len(words)
        elif after == '$' and before in table:
            start = table[before][0] + table[before][1]
            end = start + 4 * len(words)
        elif before in table and after in table:
            start = table[before][0] + table[before][1]
            end = table[after][0]
        else:
            continue
        keys = [f'0x{address:08x}' for address, _ in by_address if start <= address < end]
        if keys:
            yield keys, words, masks, start, end - start


def main() -> int:
    global REGION, IDO, BIN, ENCODER
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--region', choices=sorted(ROMS), required=True)
    parser.add_argument('--baseline', type=Path)
    parser.add_argument('--rom', type=Path, help='Generate a baseline from a verified ROM image.')
    parser.add_argument('--encoder', type=Path, help='The encode-text executable for Japanese text.')
    parser.add_argument('--ido-root', type=Path, default=IDO)
    parser.add_argument('--binutils-root', type=Path, default=BIN.parent)
    args = parser.parse_args()
    REGION, IDO, BIN, ENCODER = args.region, args.ido_root, args.binutils_root / 'bin', args.encoder
    DEFINES[DEFINES.index('-DTGR_REGION=1')] = f'-DTGR_REGION={REGION_VALUES[REGION]}'
    if not (IDO / 'cc').is_file():
        parser.error(f'IDO 5.3 compiler is missing at {IDO / "cc"}.')
    binutil('objcopy')
    binutil('readelf')
    if args.rom:
        rom = args.rom.read_bytes()
        if hashlib.sha1(rom).hexdigest() != ROM_SHA1[REGION]:
            parser.error(f'{args.rom} is not the verified {ROMS[REGION]} image.')
        expected = None
    else:
        baseline = args.baseline or ROOT / 'tools' / 'matching' / f'{REGION}.json'
        data = json.loads(baseline.read_text(encoding='utf-8'))
        if data['region'] != REGION or data['rom_sha1'] != ROM_SHA1[REGION]:
            parser.error(f'{baseline} does not describe the {REGION} release.')
        expected = data['functions']
        rom = None
    table = originals()
    by_address = sorted((address, name) for name, (address, _size) in table.items())
    entries: dict[str, dict] = {}
    seen: set[str] = set()
    if expected is None:
        sources = sorted(p for p in (ROOT / 'src').rglob('*') if p.suffix in ('.c', '.s')
                         and 'platform' not in p.parts and 'asm' not in p.parts
                         and p.name != 'divdi3.c')
        zlib = ROOT / '3rdparty' / 'zlib-1.0.4'
        sources += [zlib / name for name in ('adler32.c', 'infblock.c', 'infcodes.c', 'inffast.c',
                                             'inflate.c', 'inftrees.c', 'infutil.c', 'uncompr.c',
                                             'zutil.c')]
    else:
        sources = [ROOT / path for path in sorted({source for entry in expected.values()
                                                   for source in entry['sources']})]
    (ROOT / '.wiswa-ci').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=ROOT / '.wiswa-ci') as tmp:
        obj = Path(tmp) / 'out.o'
        for source in sources:
            relative = source.relative_to(ROOT).as_posix()
            compiled = False
            for flags in ([['as']] if source.suffix == '.s' else flag_sets(source)):
                if not (assemble(source, obj) if flags == ['as'] else
                        compile_ido(source, flags, obj)):
                    continue
                compiled = True
                for keys, words, masks, address, size in candidates(obj, table, by_address):
                    seen.update(keys)
                    fingerprint = digest(words, masks)
                    if expected is not None:
                        for key in keys:
                            entry = expected.get(key)
                            if entry and fingerprint in entry['sources'].get(relative, []):
                                entries.setdefault(key, {'sources': {}})['sources'][relative] = True
                    elif matches(words, masks, address, size, rom)[0] is True:
                        for key in keys:
                            digests = entries.setdefault(key, {'sources': {}})['sources'].setdefault(
                                relative, [])
                            if fingerprint not in digests:
                                digests.append(fingerprint)
            if not compiled and expected is not None:
                raise RuntimeError(f'{relative} does not compile with IDO 5.3.')
    if expected is None:
        missing = seen - entries.keys()
        if missing:
            print(f'{len(missing)} reconstructed routines do not match the ROM.', file=sys.stderr)
            return 1
        for entry in entries.values():
            entry['sources'] = {source: sorted(digests)
                                for source, digests in sorted(entry['sources'].items())}
        print(json.dumps({'region': REGION, 'rom_sha1': ROM_SHA1[REGION],
                          'functions': entries}, indent=2, sort_keys=True))
        return 0
    missing = [(key, source) for key, entry in expected.items() for source in entry['sources']
               if source not in entries.get(key, {'sources': {}})['sources']]
    added = seen - expected.keys()
    for key, source in sorted(missing):
        print(f'{key} in {source}: compiled code differs from the ROM-matched baseline.',
              file=sys.stderr)
    for key in sorted(added):
        print(f'{key}: reconstructed routine is absent from the baseline.', file=sys.stderr)
    print(f'{len(entries)}/{len(expected)} ROM-matched routines verified for {REGION}.')
    return 1 if missing or added else 0


if __name__ == '__main__':
    sys.exit(main())
