#!/usr/bin/env python3
"""
Write the Doxygen tag file of the Win32 API that the Windows sources use.

The pages of the API reference on learn.microsoft.com are generated from the MicrosoftDocs/sdk-api
repository, one Markdown file a page, named after the kind, the header, and the symbol in lowercase
(`nf-synchapi-waitforsingleobject.md`). The repository is cloned without the contents of its files,
and only the pages whose names match an identifier of the sources are checked out. The `api_name`
list of a page gives the exact spellings it documents, and a spelling is linked only when the
sources use it exactly and the game does not declare it.

A type without a page of its own (`DWORD`, `HANDLE`, `HWND`) links to its row of the page of the
Windows data types. A handle that neither has links to the page of the structure it refers to,
from HANDLE_PAGES.

Doxygen adds `.html` to a link without an extension, which learn.microsoft.com does not serve. Each
link therefore ends in `?.`, a query the site ignores.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import urllib.request
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

REPOSITORY = 'https://github.com/MicrosoftDocs/sdk-api.git'
CONTENT = 'sdk-api-src/content/'
# The links are relative to the reference of the API, https://learn.microsoft.com/en-us/windows/
# win32/api/.
DATA_TYPES_URL = 'https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types'
DATA_TYPES_PAGE = '../winprog/windows-data-types'
# The handles that have no page of their own and the page of the structure each refers to.
HANDLE_PAGES = {
    'HMMIO': '../../../previous-versions/dd757322(v=vs.85)',
}
SOURCE_SUFFIXES = ('.c', '.cpp', '.h')
# The query that keeps Doxygen from adding an extension.
LINK_SUFFIX = '?.'
IDENTIFIER = re.compile(r'(->|\.|::)?\s*\b([A-Za-z_][A-Za-z0-9_]*)\b')
# The comments and the literals, whose words are not identifiers.
NOT_CODE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.DOTALL)
# The types the game defines.
TYPE_DEFINITION = re.compile(r'\b(?:struct|class|union|enum)\s+([A-Za-z_]\w*)\s*[{:;]|'
                             r'\btypedef\b[^;{]*?\b([A-Za-z_]\w*)\s*;|#define\s+([A-Za-z_]\w*)')
# A routine a header of the game declares: a name after a type at the start of a line, then a
# parenthesis.
FUNCTION_DECLARATION = re.compile(r'^[ \t]*([A-Za-z_][\w \t\*&:<>,]*?)[ \t\*&]+([A-Za-z_]\w*)\s*\(',
                                  re.MULTILINE)
# The words that start a statement rather than a declaration.
STATEMENT_WORDS = {'case', 'delete', 'do', 'else', 'goto', 'if', 'new', 'return', 'sizeof', 'while'}
# The anchors of the rows of the page of the data types.
DATA_TYPE_ANCHOR = re.compile(r'<span id="([A-Z][A-Z0-9_]*)">')
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


def read_code(path: Path) -> str:
  return NOT_CODE.sub(' ', path.read_text(encoding='utf-8', errors='replace'))


def source_files(roots: list[Path]) -> list[Path]:
  return sorted(path for root in roots for path in root.rglob('*') if path.suffix in SOURCE_SUFFIXES)


def declared_names(roots: list[Path]) -> set[str]:
  """The types the game defines and the routines its headers declare."""
  names: set[str] = set()
  for path in source_files(roots):
    code = read_code(path)
    for match in TYPE_DEFINITION.finditer(code):
      names.update(name for name in match.groups() if name)
    if path.suffix == '.h':
      for prefix, name in FUNCTION_DECLARATION.findall(code):
        if prefix.split()[0] not in STATEMENT_WORDS:
          names.add(name)
  return names


def source_identifiers(roots: list[Path]) -> tuple[set[str], set[str]]:
  """The free identifiers of the sources, and every identifier including those after `->`."""
  free: set[str] = set()
  members: set[str] = set()
  for path in source_files(roots):
    for access, identifier in IDENTIFIER.findall(read_code(path)):
      (members if access else free).add(identifier)
  return free, members | free


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


def data_types() -> set[str]:
  """The types the page of the Windows data types has a row for."""
  with urllib.request.urlopen(DATA_TYPES_URL, timeout=60) as response:
    return set(DATA_TYPE_ANCHOR.findall(response.read().decode('utf-8')))


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
  parser.add_argument('--declarations',
                      type=Path,
                      nargs='+',
                      default=[Path('src')],
                      help='the directories whose own declarations are never linked')
  args = parser.parse_args()
  declared = declared_names(args.declarations)
  free, identifiers = source_identifiers(args.sources)
  # A method links only in the scope of its interface, so a method of the game of the same name
  # does not stop it.
  free -= declared
  lowered = {identifier.lower() for identifier in identifiers}

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
    kind = page.partition('/')[2].partition('-')[0]
    text = (args.sdk_api / f'{CONTENT}{page}.md').read_text(encoding='utf-8')
    for name in api_names(text):
      interface, _, method = name.rpartition('.')
      if kind == 'nf' and interface:
        if interface in free and method in identifiers:
          methods[interface].setdefault(method, page)
      elif name in free:
        links.setdefault(name, (kind, page))

  tagfile = ET.Element('tagfile')
  file_members: defaultdict[str, list[tuple[str, str, str]]] = defaultdict(list)
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
      file_members[page.partition('/')[0]].append((member_kind, name, page))
  for header, entries in sorted(file_members.items()):
    compound = ET.SubElement(tagfile, 'compound', kind='file')
    ET.SubElement(compound, 'name').text = f'{header}.h'
    ET.SubElement(compound, 'filename').text = f'{header}/{LINK_SUFFIX}'
    for member_kind, name, page in entries:
      add_member(compound, member_kind, name, page)

  types = sorted((free & data_types()) - links.keys())
  handles = sorted(name for name in HANDLE_PAGES if name in free and name not in links)
  if types or handles:
    compound = ET.SubElement(tagfile, 'compound', kind='file')
    ET.SubElement(compound, 'name').text = 'windows-data-types'
    ET.SubElement(compound, 'filename').text = DATA_TYPES_PAGE + LINK_SUFFIX
    for name in types:
      add_member(compound, 'typedef', name, DATA_TYPES_PAGE, name)
    for name in handles:
      add_member(compound, 'typedef', name, HANDLE_PAGES[name])
  ET.indent(tagfile, space='  ')
  args.output.write_text('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' +
                         ET.tostring(tagfile, encoding='unicode') + '\n',
                         encoding='utf-8')
  return 0


def add_member(compound: ET.Element, kind: str, name: str, page: str, anchor: str = '') -> None:
  member = ET.SubElement(compound, 'member', kind=kind)
  ET.SubElement(member, 'type')
  ET.SubElement(member, 'name').text = name
  ET.SubElement(member, 'anchorfile').text = page + LINK_SUFFIX
  ET.SubElement(member, 'anchor').text = anchor
  ET.SubElement(member, 'arglist').text = '()' if kind == 'function' else ''


if __name__ == '__main__':
  raise SystemExit(main())
