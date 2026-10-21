#!/usr/bin/env python3
"""
Generate PROGRESS.md and progress/*.md from the function tables of the Ghidra project.

Each program is exported through the HTTP bridge with tools/ExportFunctionTable.java. The three
ROMs form one group keyed on the USA release, and the two PC libraries form another keyed on
BRD3D.dll. A routine is done when a source file under src tags its address.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.request
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BLOCK = 0x10000
GROUPS = {
  'n64': {
    'title': 'Nintendo 64',
    'programs': [('USA', 'usa.z64'), ('JPN', 'jpn.z64'), ('EUR', 'eur.z64')],
  },
  'pc': {
    'title': 'PC port',
    'programs': [('BRD3D', 'BRD3D.dll'), ('BRGlide', 'BRGlide.dll')],
  },
}
LABELS = [label for group in GROUPS.values() for label, _ in group['programs']]
STATUS_GLYPH = {'done': ':white_check_mark:', 'stub': ':construction:', 'declared': ':memo:',
                'remaining': ''}
STATUS_ORDER = {'remaining': 0, 'declared': 1, 'stub': 2, 'done': 3}

N64_SDK = re.compile(r'^(__)?os[A-Z]|^gu[A-Z]|^__(ll|ull|d_to|f_to|ll_to|ull_to)_'
                     r'|^_(Printf|Ldtob|Litob|Genld|Putfld)$'
                     r'|^(sprintf|proutSprintf|bcopy|bzero|bcmp|memcpy|strchr|strlen|sinf|cosf'
                     r'|sqrtf|sins|ldiv|lldiv|rmonPrintf|send_mesg|viMgrMain'
                     r'|_VirtualToPhysicalTask|_MakeMotorData|corrupted|corrupted_init)$')
ZLIB = re.compile(r'^(inflate|huft_build|adler32|zcalloc|zcfree)')
PC_COMPILER = re.compile(r'^(DynamicInit|AtExitDestroy|EHHandler)_')
PC_RUNTIME_NAMES = {
  'BuildCatchObject', 'CallCatchBlock', 'CallCatchBlock_finally', 'CatchGuardHandler', 'CatchIt',
  'DestructExceptionObject', 'FindHandler', 'FindHandlerForForeignException',
  'TranslatorGuardHandler',
}


@dataclass
class Function:
  name: str
  address: int
  length: int
  xrefs: int
  thunk: bool
  prototype: str


@dataclass
class Row:
  name: str
  component: str
  excluded: str | None
  addresses: dict[str, int] = field(default_factory=dict)
  length: int = 0
  xrefs: int = 0
  prototype: str = ''
  status: str = 'remaining'

  @property
  def block(self) -> int:
    return next(iter(self.addresses.values())) // BLOCK * BLOCK


def export(ghidra: str, program: str) -> list[Function]:
  """Run the export script on one program and parse its rows."""
  code = (ROOT / 'tools' / 'ExportFunctionTable.java').read_text(encoding='utf-8')
  request = urllib.request.Request(f'{ghidra}/run_script_inline?program={program}',
                                   data=json.dumps({'code': code}).encode(),
                                   headers={'Content-Type': 'application/json'})
  with urllib.request.urlopen(request, timeout=600) as response:
    text = response.read().decode('utf-8')
  if 'SCRIPT COMPLETED SUCCESSFULLY' not in text:
    raise SystemExit(f'{program}: the export script failed:\n{text}')
  body = text.split('--- SCRIPT OUTPUT ---\n', 1)[1].split('\n=== SCRIPT COMPLETED', 1)[0]
  return parse_table(body)


def parse_table(text: str) -> list[Function]:
  functions = []
  for line in text.splitlines():
    fields = line.split('\t')
    if len(fields) != 7:
      continue
    name, address, length, xrefs, thunk, _external, prototype = fields
    functions.append(Function(name, int(address, 16), int(length), int(xrefs), thunk == 'true',
                              prototype))
  return functions


def classify(group: str, function: Function) -> tuple[str, str | None]:
  """Return the component of a function and the reason it is excluded, if it is."""
  name = function.name
  if group == 'n64':
    if function.address < 0x80000000:
      return 'PIF ROM', 'console boot ROM, not part of the cartridge'
    if function.address < 0x80000400:
      return 'Exception vector', 'copied from the SDK exception preamble at run time'
    if function.address >= 0xa4000000:
      return 'IPL3 boot code', None
    if ZLIB.match(name):
      return 'zlib', None
    if N64_SDK.match(name):
      return 'SDK (libultra)', None
    return 'Game code', None
  if function.thunk:
    return 'Thunk', 'import or jump thunk the linker generates'
  if name == 'adler32':
    return 'zlib', None
  if PC_COMPILER.match(name) or name in PC_RUNTIME_NAMES:
    return 'Compiler-generated', 'static initialisers, atexit registrations, and exception funclets'
  if re.match(r'^[_$?]', name) or (re.match(r'^[a-z]', name) and not name.startswith('gu')):
    return 'C runtime', 'MSVC runtime library'
  return 'Game code', None


def scan_sources() -> dict[tuple[str, int], str]:
  """Map every tagged address under src to done, stub, or declared."""
  tag = re.compile(r'\b(%s): 0x([0-9a-fA-F]+)' % '|'.join(LABELS))
  statuses: dict[tuple[str, int], str] = {}
  for path in sorted((ROOT / 'src').rglob('*')):
    if path.suffix not in ('.c', '.cpp', '.h'):
      continue
    text = path.read_text(encoding='utf-8')
    for match in tag.finditer(text):
      opened = text.rfind('/*', 0, match.start())
      if opened > text.rfind('*/', 0, match.start()):
        closed = text.find('*/', match.end())
      else:
        opened = text.rfind('\n', 0, match.start()) + 1
        closed = text.find('\n', match.end())
      block = text[opened:closed if closed >= 0 else len(text)]
      if '@stub' in block or '(stub)' in block:
        status = 'stub'
      elif path.suffix != '.h':
        status = 'done'
      else:
        status = 'declared'
      key = (match.group(1), int(match.group(2), 16))
      if STATUS_ORDER[status] > STATUS_ORDER.get(statuses.get(key, 'remaining'), 0):
        statuses[key] = status
  return statuses


def build_rows(group: str, tables: dict[str, list[Function]],
               statuses: dict[tuple[str, int], str]) -> list[Row]:
  """Join the programs of a group on function name, the first program being the master."""
  by_name: dict[str, dict[str, list[Function]]] = defaultdict(lambda: defaultdict(list))
  for label, _ in GROUPS[group]['programs']:
    for function in sorted(tables[label], key=lambda f: f.address):
      by_name[function.name][label].append(function)
  rows = []
  for name, per_label in by_name.items():
    for index in range(max(len(functions) for functions in per_label.values())):
      row = None
      for label, _ in GROUPS[group]['programs']:
        if index >= len(per_label[label]):
          continue
        function = per_label[label][index]
        if row is None:
          component, excluded = classify(group, function)
          row = Row(name, component, excluded, length=function.length, xrefs=function.xrefs,
                    prototype=function.prototype)
        row.addresses[label] = function.address
        status = statuses.get((label, function.address), 'remaining')
        if STATUS_ORDER[status] > STATUS_ORDER[row.status]:
          row.status = status
      rows.append(row)
  return rows


def slug(text: str) -> str:
  return re.sub(r'[^a-z0-9]+', '-', text.lower()).strip('-')


def cell(text: str) -> str:
  return text.replace('|', '\\|')


def table(headers: list[str], aligns: list[str], rows: list[list[str]]) -> list[str]:
  """Render a Markdown table padded the way Prettier pads one."""
  widths = [max(len(h), *(len(r[i]) for r in rows), 3) for i, h in enumerate(headers)]
  lines = []
  for cells in [headers, *rows]:
    out = []
    for text, width, align in zip(cells, widths, aligns):
      pad = width - len(text)
      if align == 'right':
        out.append(' ' * pad + text)
      elif align == 'center':
        out.append(' ' * (pad // 2) + text + ' ' * (pad - pad // 2))
      else:
        out.append(text + ' ' * pad)
    lines.append('| ' + ' | '.join(out) + ' |')
  rule = []
  for width, align in zip(widths, aligns):
    if align == 'right':
      rule.append('-' * (width - 1) + ':')
    elif align == 'center':
      rule.append(':' + '-' * (width - 2) + ':')
    else:
      rule.append('-' * width)
  lines.insert(1, '| ' + ' | '.join(rule) + ' |')
  return lines


def count(rows: list[Row]) -> tuple[int, int, int, str]:
  total = len(rows)
  done = sum(row.status == 'done' for row in rows)
  stubbed = sum(row.status == 'stub' for row in rows)
  share = f'{100 * done / total:.1f}%' if total else '0.0%'
  return total, done, stubbed, share


def write_part(group: str, component: str, block: int, rows: list[Row]) -> tuple[Path, str]:
  labels = [label for label, _ in GROUPS[group]['programs']]
  path = ROOT / 'progress' / f'{group}-{slug(component)}-{block:08x}.md'
  total, done, _stubbed, _share = count(rows)
  rows = sorted(rows, key=lambda r: (-r.length, -r.xrefs, STATUS_ORDER[r.status], r.name))
  body = []
  for row in rows:
    body.append([f'`{cell(row.name)}`', STATUS_GLYPH[row.status], str(row.xrefs), str(row.length)]
                + [f'`0x{row.addresses[l]:08x}`' if l in row.addresses else '' for l in labels]
                + [f'`{cell(row.prototype)}`'])
  lines = [
    f'# {component}, {GROUPS[group]["title"]}, `0x{block:08x}` to `0x{block + BLOCK:08x}`',
    '',
    f'{done} of {total} routines done.',
    '',
    'Sorted by length, then reference count, then status (remaining first), then name.',
    'Signatures are the current Ghidra prototypes and are preliminary.',
    '',
    *table(['Name', 'Status', '# xref', 'Length', *labels, 'Signature'],
           ['left', 'center', 'right', 'right', *(['left'] * len(labels)), 'left'], body),
    '',
  ]
  path.write_text('\n'.join(lines), encoding='utf-8')
  return path, f'`0x{block:08x}`-`0x{block + BLOCK:08x}`'


def write_report(groups: dict[str, list[Row]]) -> None:
  lines = [
    '# Progress',
    '',
    'Reconstruction status of Top Gear Rally. The three Nintendo 64 releases form one group,',
    'keyed on the USA release, and the two PC port libraries form another, keyed on the Direct3D',
    'library. A routine is done when a source file under `src` tags its address, for example',
    '`@ghidraAddress USA: 0x80234abc` in a Doxygen block or `// USA: 0x80234abc` on a definition.',
    'A routine marked as a stub (an `@stub` tag in its Doxygen block, or `(stub)` after its',
    'address) is shown with :construction: and is not counted as done. A routine tagged in a',
    'header with no body in the tree yet is shown with :memo: and is not counted as done.',
    '',
    'Routines the compiler or the linker supplies are counted and excluded. The N64 SDK',
    '(libultra) and the IPL3 boot code are reconstructed and counted. zlib is imported under',
    '`3rdparty` at the matching release, edited in place where the game differs, and counted. The',
    'PIF ROM is the console boot ROM, is not part of the cartridge, and is excluded.',
    '',
    'Regenerate after every merged change, with the Ghidra project open and the HTTP bridge',
    'running:',
    '',
    '```shell',
    'python3 tools/progress.py',
    '```',
    '',
    '## Work outline',
    '',
    '1. Choose the N64 toolchain and add the CMake toolchain file.',
    '2. Reconstruct the USA ROM, one subsystem directory under `src` at a time, in the order of',
    '   the parts below (largest routines first).',
    '3. Port each routine to the JPN and EUR releases, and reconstruct the routines only those',
    '   releases have.',
    '4. Reconstruct the PC port from the two libraries, reusing the N64 sources where the port',
    '   retained them.',
    '',
  ]
  parts = []
  for group, rows in groups.items():
    labels = GROUPS[group]['programs']
    included = [row for row in rows if row.excluded is None]
    excluded = [row for row in rows if row.excluded is not None]
    total, done, stubbed, share = count(included)
    declared = sum(row.status == 'declared' for row in included)
    summary = [['Functions in the programs', f'{len(rows):,}']]
    for reason in sorted({row.component for row in excluded}):
      reason_rows = [row for row in excluded if row.component == reason]
      summary.append([f'Excluded as {reason} ({reason_rows[0].excluded})',
                      f'{len(reason_rows):,}'])
    summary += [
      ['To reconstruct', f'{total:,}'],
      ['Done', f'{done:,}'],
      ['Stubbed (not counted as done)', f'{stubbed:,}'],
      ['Declared only (not counted as done)', f'{declared:,}'],
      ['Share done', share],
    ]
    only = [[f'Only in {label}', f'{sum(set(r.addresses) == {label} for r in included):,}']
            for label, _ in labels[1:]]
    lines += [f'## {GROUPS[group]["title"]}', '',
              f'The programs are {", ".join(f"`{program}`" for _, program in labels)}.', '',
              *table(['Measure', 'Count'], ['left', 'right'], summary + only), '',
              '### By component', '']
    components = []
    for component in sorted({row.component for row in included}):
      component_rows = [row for row in included if row.component == component]
      total, done, stubbed, share = count(component_rows)
      components.append([component, f'{total:,}', f'{done:,}', f'{stubbed:,}', share])
    lines += table(['Component', 'Routines', 'Done', 'Stubbed', 'Share'],
                   ['left', 'right', 'right', 'right', 'right'], components) + ['']
    by_part: dict[tuple[str, int], list[Row]] = defaultdict(list)
    for row in included:
      by_part[(row.component, row.block)].append(row)
    for (component, block), part_rows in sorted(by_part.items(), key=lambda item: item[0][::-1]):
      path, span = write_part(group, component, block, part_rows)
      total, done, _stubbed, share = count(part_rows)
      parts.append([f'[{path.name}](progress/{path.name})', GROUPS[group]['title'], component,
                    span, f'{total:,}', f'{done:,}', share])
  lines += ['## Parts', '', 'Each part lists the routines of one component in a 64 KiB address',
            'block of the master program.', '',
            *table(['Part', 'Group', 'Component', 'Range', 'Routines', 'Done', 'Share'],
                   ['left', 'left', 'left', 'left', 'right', 'right', 'right'], parts), '']
  (ROOT / 'PROGRESS.md').write_text('\n'.join(lines), encoding='utf-8')


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('--ghidra', default='http://127.0.0.1:8090', help='HTTP bridge URL')
  parser.add_argument('--cache', type=Path, default=ROOT / '.wiswa-ci' / 'progress',
                      help='directory for the exported tables')
  parser.add_argument('--offline', action='store_true',
                      help='read the tables from the cache instead of Ghidra')
  args = parser.parse_args()
  args.cache.mkdir(parents=True, exist_ok=True)
  statuses = scan_sources()
  groups = {}
  for group, settings in GROUPS.items():
    tables = {}
    for label, program in settings['programs']:
      cached = args.cache / f'{program}.tsv'
      if args.offline:
        tables[label] = parse_table(cached.read_text(encoding='utf-8'))
      else:
        tables[label] = export(args.ghidra, program)
        cached.write_text('\n'.join('\t'.join([f.name, f'{f.address:08x}', str(f.length),
                                               str(f.xrefs), str(f.thunk).lower(), 'false',
                                               f.prototype]) for f in tables[label]) + '\n',
                          encoding='utf-8')
      print(f'{program}: {len(tables[label])} functions', file=sys.stderr)
    groups[group] = build_rows(group, tables, statuses)
  progress = ROOT / 'progress'
  progress.mkdir(exist_ok=True)
  for stale in progress.glob('*.md'):
    stale.unlink()
  write_report(groups)
  return 0


if __name__ == '__main__':
  sys.exit(main())
