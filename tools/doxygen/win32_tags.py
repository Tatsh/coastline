#!/usr/bin/env python3
"""
Write the Doxygen tag file of the Win32 API that the Windows sources use.

The pages of the API reference on learn.microsoft.com are generated from the MicrosoftDocs/sdk-api
repository, one Markdown file a page, named after the kind, the header, and the symbol in lowercase
(`nf-synchapi-waitforsingleobject.md`). The repository is cloned without the contents of its files,
and only the pages whose names match an identifier of the sources are checked out. The `api_name`
list of a page gives the exact spellings it documents, and a spelling is linked only when the
sources use it exactly.

Doxygen adds `.html` to a link without an extension, which learn.microsoft.com does not serve. Each
link therefore ends in `?.`, a query the site ignores.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

REPOSITORY = 'https://github.com/MicrosoftDocs/sdk-api.git'
CONTENT = 'sdk-api-src/content/'
SOURCE_SUFFIXES = ('.c', '.cpp', '.h')
# The query that keeps Doxygen from adding an extension.
LINK_SUFFIX = '?.'
IDENTIFIER = re.compile(r'(->|\.|::)?\s*\b([A-Za-z_][A-Za-z0-9_]*)\b')
# The comments and the literals, whose words are not identifiers.
NOT_CODE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.DOTALL)
# The names the sources define themselves, which are not linked.
DEFINITION = re.compile(r'\b(?:struct|class|union|enum)\s+([A-Za-z_]\w*)\s*[{:;]|'
                        r'\btypedef\b[^;{]*?\b([A-Za-z_]\w*)\s*;|#define\s+([A-Za-z_]\w*)')
# The kinds of page written: functions and methods, structures, enumerations, callbacks, and
# interfaces.
KINDS = ('nf', 'ns', 'ne', 'nc', 'nn')


def git(sdk_api: Path, *arguments: str) -> str:
  return subprocess.run(['git', '-C', str(sdk_api), *arguments],
                        check=True,
                        capture_output=True,
                        text=True).stdout


def list_pages(sdk_api: Path) -> list[str]:
  """List the pages of the reference, cloning the repository without file contents if needed."""
  if not (sdk_api / '.git').exists():
    subprocess.run([
        'git', 'clone', '--quiet', '--filter=blob:none', '--no-checkout', '--depth', '1',
        REPOSITORY,
        str(sdk_api)
    ],
                   check=True)
  return [
      name[len(CONTENT):-len('.md')]
      for name in git(sdk_api, 'ls-tree', '-r', '--name-only', 'HEAD').splitlines()
      if name.startswith(CONTENT) and name.endswith('.md')
  ]


def source_identifiers(sources: list[Path]) -> tuple[set[str], set[str]]:
  """
  The free identifiers of the sources, and those that only follow `->`, `.`, or `::`, without the
  names the sources define.
  """
  free: set[str] = set()
  members: set[str] = set()
  defined: set[str] = set()
  for root in sources:
    for path in root.rglob('*'):
      if path.suffix in SOURCE_SUFFIXES:
        code = NOT_CODE.sub(' ', path.read_text(encoding='utf-8', errors='replace'))
        for match in DEFINITION.finditer(code):
          defined.update(name for name in match.groups() if name)
        for access, identifier in IDENTIFIER.findall(code):
          (members if access else free).add(identifier)
  return free - defined, (members | free) - defined


def api_names(text: str) -> list[str]:
  """The spellings in the `api_name` list of the front matter of a page."""
  names: list[str] = []
  in_list = False
  for line in text.splitlines()[1:]:
    if line.startswith('---'):
      break
    if line.startswith('api_name:'):
      in_list = True
    elif in_list and line.startswith(' - '):
      names.append(line[3:].strip())
    else:
      in_list = False
  return names


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
  parser.add_argument('output', type=Path, help='the tag file to write')
  parser.add_argument('--sdk-api',
                      type=Path,
                      required=True,
                      help='a clone of MicrosoftDocs/sdk-api, made there when absent')
  parser.add_argument('--sources',
                      type=Path,
                      nargs='+',
                      default=[Path('src/platform/win')],
                      help='the directories whose identifiers are looked up')
  args = parser.parse_args()
  identifiers, members = source_identifiers(args.sources)
  lowered = {identifier.lower() for identifier in members}

  candidates: list[str] = []
  for page in sorted(list_pages(args.sdk_api)):
    _, _, stem = page.partition('/')
    kind, _, rest = stem.partition('-')
    _, _, symbol = rest.partition('-')
    if kind not in KINDS or not symbol:
      continue
    parts = symbol.split('-')
    # The page of an ANSI or a wide form also documents the name of the macro the sources use.
    if all(part in lowered or part[:-1] in lowered for part in parts):
      candidates.append(page)
  paths = [f'{CONTENT}{page}.md' for page in candidates]
  subprocess.run(['git', '-C', str(args.sdk_api), 'sparse-checkout', 'set', '--no-cone', *paths],
                 check=True,
                 capture_output=True)
  git(args.sdk_api, 'checkout', '--quiet', 'HEAD')

  # The first page of a spelling in the order of the headers is linked.
  links: dict[str, tuple[str, str]] = {}
  methods: dict[str, dict[str, str]] = defaultdict(dict)
  for page in candidates:
    header, _, stem = page.partition('/')
    kind = stem.partition('-')[0]
    text = (args.sdk_api / f'{CONTENT}{page}.md').read_text(encoding='utf-8')
    for name in api_names(text):
      interface, _, method = name.rpartition('.')
      if kind == 'nf' and interface:
        if interface in identifiers and method in members:
          methods[interface].setdefault(method, page)
      elif name in identifiers:
        links.setdefault(name, (kind, page))

  tagfile = ET.Element('tagfile')
  members: defaultdict[str, list[tuple[str, str, str]]] = defaultdict(list)
  for name, (kind, page) in sorted(links.items()):
    if kind == 'ns':
      compound = ET.SubElement(tagfile, 'compound', kind='struct')
      ET.SubElement(compound, 'name').text = name
      ET.SubElement(compound, 'filename').text = page + LINK_SUFFIX
    elif kind == 'nn':
      compound = ET.SubElement(tagfile, 'compound', kind='class')
      ET.SubElement(compound, 'name').text = name
      ET.SubElement(compound, 'filename').text = page + LINK_SUFFIX
      for method, method_page in sorted(methods.get(name, {}).items()):
        add_member(compound, 'function', method, method_page)
    else:
      member_kind = {'nf': 'function', 'ne': 'enumeration', 'nc': 'typedef'}[kind]
      members[page.partition('/')[0]].append((member_kind, name, page))
  for header, entries in sorted(members.items()):
    compound = ET.SubElement(tagfile, 'compound', kind='file')
    ET.SubElement(compound, 'name').text = f'{header}.h'
    ET.SubElement(compound, 'filename').text = f'{header}/{LINK_SUFFIX}'
    for member_kind, name, page in entries:
      add_member(compound, member_kind, name, page)
  ET.indent(tagfile, space='  ')
  args.output.write_text('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' +
                         ET.tostring(tagfile, encoding='unicode') + '\n',
                         encoding='utf-8')
  return 0


def add_member(compound: ET.Element, kind: str, name: str, page: str) -> None:
  member = ET.SubElement(compound, 'member', kind=kind)
  ET.SubElement(member, 'type')
  ET.SubElement(member, 'name').text = name
  ET.SubElement(member, 'anchorfile').text = page + LINK_SUFFIX
  ET.SubElement(member, 'anchor')
  ET.SubElement(member, 'arglist').text = '()' if kind == 'function' else ''


if __name__ == '__main__':
  raise SystemExit(main())
