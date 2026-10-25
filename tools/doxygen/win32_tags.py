#!/usr/bin/env python3
"""
Write the Doxygen tag file of the Win32 API that the Windows sources use.

The pages of the API reference on learn.microsoft.com are generated from the MicrosoftDocs/sdk-api
repository, one Markdown file a page, named after the kind, the header, and the symbol in lowercase
(`nf-synchapi-waitforsingleobject.md`). The repository is cloned without the contents of its files,
and only the pages whose names match an identifier of the sources are checked out. The `api_name`
list of a page gives the exact spellings it documents, and a spelling is linked only when the
sources use it exactly and the game does not declare it.

The pages of the multimedia API outside the reference (the MCI structures) are generated from the
MicrosoftDocs/win32 repository, whose directories of WIN32_DOCS_DIRECTORIES are checked out whole
and linked by the `api_name` lists of their pages.

A type without a page of its own (`DWORD`, `HANDLE`, `HWND`) links to its row of the page of the
Windows data types. A name that neither has links to the page PAGES_OF_NAMES records for it.

Doxygen adds `.html` to a link without an extension, which learn.microsoft.com does not serve. Each
link therefore ends in `?.`, a query the site ignores.

The older DirectX APIs are no longer on learn.microsoft.com. With `--archive-output`, a second tag
file links the names the first does not to the references of DirectX 7 and 8 that the MSDN Library
had in 2000 and 2001, as the Wayback Machine of archive.org captured them. The names of the pages
of DirectX 7 are codes, so the title of each page is fetched once and kept in the cache file. The
pages of DirectX 8 are named after the names they document.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import time
import urllib.error
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
WIN32_DOCS_REPOSITORY = 'https://github.com/MicrosoftDocs/win32.git'
WIN32_DOCS_CONTENT = 'desktop-src/'
WIN32_DOCS_DIRECTORIES = ('Multimedia',)
# The names without a page in either repository, and the page that documents each: the structure
# a handle or a reference refers to or that returns it, the page of the values of a result, or the
# page of a routine in the previous versions of the documentation.
PAGES_OF_NAMES = {
    'HMMIO': '../../../previous-versions/dd757322(v=vs.85)',
    'MCIDEVICEID': '../multimedia/mci-open-parms',
    'MCIERROR': '../multimedia/handling-mci-errors',
    'mciSendCommandA': '../../../previous-versions/dd757160(v=vs.85)',
    # A reference to a GUID.
    'REFGUID': 'guiddef/ns-guiddef-guid',
}
# The kind of an entry of the multimedia pages by the last word of the title of its page.
WIN32_DOCS_KINDS = {'structure': 'ns', 'function': 'nf', 'macro': 'nf'}
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
# The reference of DirectX 7 of the MSDN Library and the prefixes of its pages of DirectDraw,
# Direct3D Immediate Mode, DirectSound, DirectPlay, and DirectInput. The links are relative to
# https://web.archive.org/web/.
ARCHIVE_PREFIX = 'msdn.microsoft.com/library/psdk/directx/'
ARCHIVE_REFERENCES = ('ddref_', 'imref_', 'dsref_', 'dpref_', 'diref_')
ARCHIVE_INDEX = ('https://web.archive.org/cdx/search/cdx?url=' + ARCHIVE_PREFIX +
                 '&matchType=prefix&collapse=urlkey&fl=timestamp,original'
                 '&filter=statuscode:200&filter=mimetype:text/html')
# A page of the reference of DirectX 8, named after the name it documents in lowercase, then its
# component. The pages of the glossary, one a letter, are not of a name.
ARCHIVE_DX8_PAGE = re.compile(r'/hh/directx8_c/_dx_([a-z0-9_]+)_(?!glos\.)[a-z0-9]+\.htm$',
                              re.IGNORECASE)
# The title of an archived page that documents a name, or a method of an interface.
ARCHIVE_TITLE = re.compile(r'<title>\s*([A-Za-z_]\w*(?:::[A-Za-z_]\w*)?)\s*</title>', re.IGNORECASE)
# The pause between two requests to archive.org, in seconds, and the tries of a request.
ARCHIVE_PAUSE = 1.0
ARCHIVE_TRIES = 5


def git(sdk_api: Path, *arguments: str) -> str:
  return subprocess.run(['git', '-C', str(sdk_api), *arguments],
                        check=True,
                        capture_output=True,
                        text=True).stdout


def clone(repository: str, path: Path) -> None:
  """Clone a repository without the contents of its files, unless it is there."""
  if not (path / '.git').exists():
    subprocess.run([
        'git', 'clone', '--quiet', '--filter=blob:none', '--no-checkout', '--depth', '1',
        repository,
        str(path)
    ],
                   check=True)


def win32_docs_pages(win32_docs: Path) -> dict[str, tuple[str, str]]:
  """Map each name of the multimedia pages to its kind and to its page, relative to the API."""
  clone(WIN32_DOCS_REPOSITORY, win32_docs)
  directories = [f'{WIN32_DOCS_CONTENT}{directory}/' for directory in WIN32_DOCS_DIRECTORIES]
  subprocess.run(
      ['git', '-C', str(win32_docs), 'sparse-checkout', 'set', '--no-cone', *directories],
      check=True,
      capture_output=True)
  git(win32_docs, 'checkout', '--quiet', 'HEAD')
  pages: dict[str, tuple[str, str]] = {}
  for directory in directories:
    for path in sorted((win32_docs / directory).glob('*.md')):
      text = path.read_text(encoding='utf-8')
      title = re.search(r'^title:\s*(.*)$', text, re.MULTILINE)
      words = re.sub(r'\(.*?\)', '', title.group(1)).split() if title else []
      kind = WIN32_DOCS_KINDS.get(words[-1].lower()) if words else None
      if kind is None:
        continue
      page = '../' + str(path.relative_to(win32_docs / WIN32_DOCS_CONTENT).with_suffix('')).lower()
      for name in api_names(text):
        pages.setdefault(name, (kind, page))
  return pages


def list_pages(sdk_api: Path) -> list[str]:
  """List the pages of the reference, cloning the repository without file contents if needed."""
  clone(REPOSITORY, sdk_api)
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
    elif in_list and line.lstrip().startswith('- '):
      names.append(line.lstrip()[2:].strip())
    else:
      in_list = False
  return names


def fetch(url: str) -> str:
  """Fetch a page of archive.org, waiting longer after each refusal."""
  for attempt in range(ARCHIVE_TRIES):
    time.sleep(ARCHIVE_PAUSE * 2**attempt)
    try:
      with urllib.request.urlopen(url, timeout=120) as response:
        return response.read().decode('utf-8', errors='replace')
    except (urllib.error.URLError, TimeoutError):
      continue
  raise RuntimeError(f'archive.org did not serve {url}')


def archive_pages(cache: Path) -> tuple[dict[str, str], dict[str, str]]:
  """
  Map the name each archived page of the reference of DirectX 7 documents to its capture, and the
  lowercase name of each page of the reference of DirectX 8 to its capture.
  """
  titles: dict[str, str] = json.loads(cache.read_text(encoding='utf-8')) if cache.exists() else {}
  dx8: dict[str, str] = {}
  for line in sorted(fetch(ARCHIVE_INDEX).splitlines()):
    timestamp, _, original = line.partition(' ')
    capture = f'{timestamp}/{original}'
    if (match := ARCHIVE_DX8_PAGE.search(original)) is not None:
      dx8.setdefault(match.group(1).lower(), capture)
    elif original.rpartition('/')[2].startswith(ARCHIVE_REFERENCES) and capture not in titles:
      match = ARCHIVE_TITLE.search(fetch(f'https://web.archive.org/web/{timestamp}id_/{original}'))
      titles[capture] = match.group(1) if match else ''
      cache.write_text(json.dumps(titles, indent=2, sort_keys=True) + '\n', encoding='utf-8')
  pages: dict[str, str] = {}
  for capture, title in sorted(titles.items()):
    if title:
      pages.setdefault(title, capture)
  return pages, dx8


def write_tagfile(tagfile: ET.Element, output: Path) -> None:
  ET.indent(tagfile, space='  ')
  output.write_text('<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' +
                    ET.tostring(tagfile, encoding='unicode') + '\n',
                    encoding='utf-8')


def archive_tagfile(pages: dict[str, str], dx8: dict[str, str], free: set[str],
                    identifiers: set[str], linked: set[str]) -> ET.Element:
  """The tag file of the archived pages of the names the sources use and the first does not link."""
  tagfile = ET.Element('tagfile')
  # The pages of DirectX 8 are matched by lowercase names, so only uppercase spellings (structures
  # and constants), with or without the A or W of an ANSI or a wide form, are looked up there. A
  # method of DirectX 8 belongs to a later interface than the one the sources use.
  for name in sorted(free - linked - pages.keys()):
    if name.upper() != name:
      continue
    lower = name.lower()
    capture = dx8.get(lower) or (dx8.get(lower[:-1]) if lower[-1:] in 'aw' else None)
    if capture is not None:
      compound = ET.SubElement(tagfile, 'compound', kind='struct')
      ET.SubElement(compound, 'name').text = name
      ET.SubElement(compound, 'filename').text = capture
  def spelling(name: str) -> str | None:
    # The page of a name also documents the ANSI and the wide form the sources may use.
    for candidate in (name, f'{name}A', f'{name}W'):
      if candidate in free and candidate not in linked:
        return candidate
    return None

  methods: defaultdict[str, list[tuple[str, str]]] = defaultdict(list)
  interface_pages: dict[str, str] = {}
  routines: list[tuple[str, str]] = []
  for title, capture in pages.items():
    interface, _, method = title.rpartition('::')
    if interface:
      if (name := spelling(interface)) is not None and method in identifiers:
        methods[name].append((method, capture))
    elif (name := spelling(title)) is not None:
      if re.fullmatch(r'I[A-Z]\w*', title):
        methods.setdefault(name, [])
        interface_pages[name] = capture
      elif title.upper() == title:
        compound = ET.SubElement(tagfile, 'compound', kind='struct')
        ET.SubElement(compound, 'name').text = name
        ET.SubElement(compound, 'filename').text = capture
      else:
        routines.append((name, capture))
  for interface, entries in sorted(methods.items()):
    compound = ET.SubElement(tagfile, 'compound', kind='class')
    ET.SubElement(compound, 'name').text = interface
    if interface in interface_pages:
      ET.SubElement(compound, 'filename').text = interface_pages[interface]
    for method, capture in sorted(entries):
      add_member(compound, 'function', method, capture, suffix='')
  if routines:
    compound = ET.SubElement(tagfile, 'compound', kind='file')
    ET.SubElement(compound, 'name').text = 'directx'
    for name, capture in sorted(routines):
      add_member(compound, 'function', name, capture, suffix='')
  return tagfile


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
  parser.add_argument('--win32-docs',
                      type=Path,
                      required=True,
                      help='a clone of MicrosoftDocs/win32, made there when absent')
  parser.add_argument('--archive-output',
                      type=Path,
                      help='the tag file of the archived reference of DirectX 7 to write')
  parser.add_argument('--archive-cache',
                      type=Path,
                      help='the file that keeps the titles of the archived pages between runs')
  args = parser.parse_args()
  if args.archive_output and not args.archive_cache:
    parser.error('--archive-output needs --archive-cache')
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
    # A page of a later revision of a name has a suffix such as `~r1`.
    parts = symbol.partition('~')[0].split('-')
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
    # The site writes the `~` of a revision as `-`.
    page = page.replace('~', '-')
    for name in api_names(text):
      interface, _, method = name.rpartition('.')
      if kind == 'nf' and interface:
        if interface in free and method in identifiers:
          methods[interface].setdefault(method, page)
      elif name in free:
        links.setdefault(name, (kind, page))
  for name, entry in win32_docs_pages(args.win32_docs).items():
    if name in free:
      links.setdefault(name, entry)

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
      # A page outside the reference is grouped under its directory, beside the reference.
      group = '../' + page.split('/')[1] if page.startswith('../') else page.partition('/')[0]
      file_members[group].append((member_kind, name, page))
  for group, entries in sorted(file_members.items()):
    compound = ET.SubElement(tagfile, 'compound', kind='file')
    ET.SubElement(compound, 'name').text = group.removeprefix('../') + (
        '' if group.startswith('../') else '.h')
    ET.SubElement(compound, 'filename').text = f'{group}/{LINK_SUFFIX}'
    for member_kind, name, page in entries:
      add_member(compound, member_kind, name, page)

  types = sorted((free & data_types()) - links.keys())
  handles = sorted(name for name in PAGES_OF_NAMES if name in free and name not in links)
  if types or handles:
    compound = ET.SubElement(tagfile, 'compound', kind='file')
    ET.SubElement(compound, 'name').text = 'windows-data-types'
    ET.SubElement(compound, 'filename').text = DATA_TYPES_PAGE + LINK_SUFFIX
    for name in types:
      add_member(compound, 'typedef', name, DATA_TYPES_PAGE, name)
    for name in handles:
      # A routine starts with a lowercase letter, and a type does not.
      add_member(compound, 'function' if name[0].islower() else 'typedef', name,
                 PAGES_OF_NAMES[name])
  write_tagfile(tagfile, args.output)
  if args.archive_output:
    linked = set(links) | set(types) | set(handles)
    pages, dx8 = archive_pages(args.archive_cache)
    write_tagfile(archive_tagfile(pages, dx8, free, identifiers, linked), args.archive_output)
  return 0


def add_member(compound: ET.Element,
               kind: str,
               name: str,
               page: str,
               anchor: str = '',
               suffix: str = LINK_SUFFIX) -> None:
  member = ET.SubElement(compound, 'member', kind=kind)
  ET.SubElement(member, 'type')
  ET.SubElement(member, 'name').text = name
  ET.SubElement(member, 'anchorfile').text = page + suffix
  ET.SubElement(member, 'anchor').text = anchor
  ET.SubElement(member, 'arglist').text = '()' if kind == 'function' else ''


if __name__ == '__main__':
  raise SystemExit(main())
