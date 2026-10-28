#!/usr/bin/env python3
"""Prepare an isolated Boss Rally installation and start its Windows launcher."""

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
INSTALL_DIRECTORY = r'C:\BossRally'
INSTALL_KEY = r'HKLM\Software\SouthPeak Interactive\Boss Rally'


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, default=Path.home() / 'Downloads/Boss_Rally_ISO/data',
                        help='Extracted CD data directory.')
    parser.add_argument('--prefix', type=Path, default=ROOT / '.wiswa-ci/wineprefix',
                        help='Wine prefix, created when absent.')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/win-d3d',
                        help='CMake directory with the rebuilt DLL and launchers.')
    parser.add_argument('--ear-dll', type=Path, default=ROOT / '.wiswa-ci/earpds-unpacked.dll',
                        help='EAR sound DLL to copy beside the game as earpds.dll.')
    parser.add_argument('--stage-only', action='store_true',
                        help='Copy files without Wine; configure the prefix on a later run.')
    parser.add_argument('--no-launch', action='store_true',
                        help='Prepare and configure the Wine prefix without starting the game.')
    parser.add_argument('--desktop-size', default='800x600',
                        help='Size of the Wine desktop window, such as 800x600.')
    parser.add_argument('--wine', default=os.environ.get('WINE', 'wine'),
                        help='Wine executable; inherited Wine architecture settings are preserved.')
    parser.add_argument('--winecfg', default=os.environ.get('WINECFG', 'winecfg'),
                        help='winecfg executable for initialising a fresh prefix.')
    parser.add_argument('--wineserver', default=os.environ.get('WINESERVER', 'wineserver'),
                        help='wineserver executable used to wait for prefix setup to finish.')
    parser.add_argument('--configure-prefix', action='store_true',
                        help='Open winecfg even when the prefix already exists.')
    parser.add_argument('game_args', nargs=argparse.REMAINDER,
                        help='Arguments after -- go to the game.')
    return parser.parse_args()


def copy_file(source, destination):
    if not destination.exists() or source.stat().st_mtime_ns != destination.stat().st_mtime_ns:
        shutil.copy2(source, destination)


def replace_binary(source, destination):
    with tempfile.NamedTemporaryFile(dir=destination.parent, delete=False) as temporary:
        temporary_path = Path(temporary.name)
    try:
        shutil.copy2(source, temporary_path)
        temporary_path.replace(destination)
    finally:
        temporary_path.unlink(missing_ok=True)


def initialise_prefix(options, prefix, installation, environment):
    required = (prefix / 'system.reg', prefix / 'drive_c/windows/system32/kernel32.dll',
                prefix / 'drive_c/windows/explorer.exe')
    if all(path.is_file() for path in required) and not options.configure_prefix:
        return
    winecfg = shutil.which(options.winecfg)
    if winecfg is None:
        raise ValueError(f'winecfg executable {options.winecfg!r} was not found.')
    wineserver = shutil.which(options.wineserver)
    if wineserver is None:
        raise ValueError(f'wineserver executable {options.wineserver!r} was not found.')
    print('Opening winecfg. Close its window when prefix configuration is complete.', flush=True)
    subprocess.run([str(Path(winecfg).resolve())], env=environment,
                   cwd=installation, check=True)
    print('Waiting for Wine prefix setup and registry writes to finish.', flush=True)
    subprocess.run([wineserver, '-w'], env=environment, cwd=installation, check=True)
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise ValueError('Wine prefix initialisation is incomplete:\n' + '\n'.join(missing))


def stage(options):
    data = options.data.expanduser().resolve()
    prefix = options.prefix.expanduser().resolve()
    build = options.build_dir.expanduser().resolve()
    if not (data / 'BossRally.pod').is_file():
        raise ValueError(f'BossRally.pod is absent from {data}. Use --data with the extracted CD.')
    for name in ('BRString.dll',):
        if not (data / name).is_file():
            raise ValueError(f'{name} is absent from the extracted CD directory {data}.')
    rebuilt = {
        'BRD3D.dll': build / 'src/BRD3D.dll',
        'BRally.exe': build / 'src/platform/win/launcher/BRally.exe',
        'BossRally.exe': build / 'src/platform/win/launcher/BossRally.exe',
        'earpds.dll': options.ear_dll.expanduser().resolve(),
    }
    cache = build / 'CMakeCache.txt'
    compiler_match = re.search(r'^CMAKE_CXX_COMPILER:[^=]+=(.+)$',
                               cache.read_text(), re.MULTILINE)
    if compiler_match is None:
        raise ValueError(f'CMAKE_CXX_COMPILER is absent from {cache}.')
    compiler = Path(compiler_match.group(1))
    target = compiler.name.removesuffix('-g++')
    for name in ('libc++.dll', 'libunwind.dll'):
        runtime_path = compiler.parent.parent / target / 'bin' / name
        if not runtime_path.is_file():
            raise ValueError(f'The build compiler could not locate {name}.')
        rebuilt[name] = runtime_path
    missing = [str(path) for path in rebuilt.values() if not path.is_file()]
    if missing:
        raise ValueError('Required binaries are absent:\n' + '\n'.join(missing)
                         + '\nBuild BRD3D, coastline_brally, and coastline_bossrally first.'
                         + '\nSelect the EAR sound DLL with --ear-dll.')
    installation = prefix / 'drive_c/BossRally'
    if data == installation or data in installation.parents or installation in data.parents:
        raise ValueError('The installation and extracted CD directories must be separate.')
    installation.mkdir(parents=True, exist_ok=True)
    for source in data.rglob('*'):
        relative = source.relative_to(data)
        if any(part.lower() in ('msvcrt.dll', 'boot.exe', 'setvideo.exe', 'brally.exe',
                               'bossrally.exe', 'brd3d.dll', 'brglide.dll', 'earpds.dll')
               for part in relative.parts):
            continue
        destination = installation / relative
        if source.is_dir():
            destination.mkdir(parents=True, exist_ok=True)
        elif source.is_file():
            destination.parent.mkdir(parents=True, exist_ok=True)
            copy_file(source, destination)
    for name, source in rebuilt.items():
        replace_binary(source, installation / name)
        print(f'Copied {source} to {installation / name}', flush=True)
    ini = installation / 'BossRally.ini'
    if not ini.exists():
        ini.write_text('[Video]\nDriver=D3D\n\n[Game]\nPlayMusic=0\nPlaySFX=1\n',
                       encoding='ascii')
    (installation / '.windows-label').write_text('Boss Rally\n', encoding='ascii')
    devices = prefix / 'dosdevices'
    devices.mkdir(exist_ok=True)
    for letter, target in (('c:', prefix / 'drive_c'), ('z:', Path('/')),
                           ('d:', installation)):
        drive = devices / letter
        if drive.is_symlink():
            if drive.resolve() != target:
                raise ValueError(f'{drive} already maps another directory. Choose another --prefix.')
        elif drive.exists():
            raise ValueError(f'{drive} already exists. Choose another --prefix.')
        else:
            drive.symlink_to(target, target_is_directory=True)
    print(f'Prepared {installation}', flush=True)
    return prefix, installation


def main():
    options = arguments()
    if re.fullmatch(r'[1-9][0-9]{2,4}x[1-9][0-9]{2,4}', options.desktop_size) is None:
        raise ValueError('--desktop-size must use WIDTHxHEIGHT, such as 800x600.')
    prefix, installation = stage(options)
    if options.stage_only:
        print('Files staged. Wine prefix initialisation and registry configuration are pending.')
        return 0
    wine = shutil.which(options.wine)
    if wine is None:
        raise ValueError(f'Wine executable {options.wine!r} was not found.')
    wine = str(Path(wine).resolve())
    environment = dict(os.environ, WINEPREFIX=str(prefix))
    initialise_prefix(options, prefix, installation, environment)

    def run(*command):
        subprocess.run([wine, *command], env=environment, cwd=installation, check=True)

    print('Configuring the Wine installation registry entry.', flush=True)
    run('reg', 'add', INSTALL_KEY, '/v', 'Directory', '/t', 'REG_SZ',
        '/d', INSTALL_DIRECTORY, '/f', '/reg:32')
    run('reg', 'add', r'HKCU\Software\Wine\Drives', '/v', 'd:', '/t', 'REG_SZ',
        '/d', 'cdrom', '/f')
    debugger = prefix / 'drive_c/windows/system32/winedbg.exe'
    if not debugger.is_file():
        raise ValueError(f'Wine debugger is absent from {debugger}.')
    debugger_key = r'HKLM\Software\Microsoft\Windows NT\CurrentVersion\AeDebug'
    debugger_command = (r'C:\windows\system32\cmd.exe /c '
                        r'C:\windows\system32\winedbg.exe --auto %ld %ld '
                        r'> C:\BossRally\wine-crash.log 2>&1')
    for view in ('/reg:32', '/reg:64'):
        run('reg', 'add', debugger_key, '/v', 'Debugger', '/t', 'REG_SZ',
            '/d', debugger_command, '/f', view)
        run('reg', 'add', debugger_key, '/v', 'Auto', '/t', 'REG_SZ', '/d', '1', '/f', view)
    run('reg', 'add', r'HKCU\Software\Wine\WineDbg', '/v', 'ShowCrashDialog',
        '/t', 'REG_DWORD', '/d', '0', '/f')
    run('reg', 'add', r'HKCU\Software\Wine\Explorer', '/v', 'Desktop',
        '/t', 'REG_SZ', '/d', 'BossRally', '/f')
    run('reg', 'add', r'HKCU\Software\Wine\Explorer\Desktops', '/v', 'BossRally',
        '/t', 'REG_SZ', '/d', options.desktop_size, '/f')
    if options.no_launch:
        print('Wine prefix configured. Run this script again to start the game.')
        return 0
    game_args = options.game_args
    if game_args and game_args[0] == '--':
        game_args = game_args[1:]
    print(f'Starting BRally.exe in a {options.desktop_size} Wine desktop window.', flush=True)
    print(f'Application diagnostics append to {installation / "coastline.log"}', flush=True)
    print(f'Crash backtraces go to {installation / "wine-crash.log"}', flush=True)
    run(INSTALL_DIRECTORY + r'\BRally.exe', *game_args)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f'Error: {error}', file=sys.stderr)
        sys.exit(1)
