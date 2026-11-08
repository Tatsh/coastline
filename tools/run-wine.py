#!/usr/bin/env python3
"""Prepare an isolated Boss Rally installation and start its Windows launcher."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import os
import re
import shutil
import subprocess as sp
import tempfile

import click

__all__ = ('main',)

ROOT = Path(__file__).resolve().parents[1]
INSTALL_DIRECTORY = r'C:\BossRally'
INSTALL_KEY = r'HKLM\Software\SouthPeak Interactive\Boss Rally'


@dataclass(frozen=True)
class _Options:
    """Paths and switches for a Wine staging run."""

    build_dir: Path
    configure_prefix: bool
    data: Path
    debugger: bool
    desktop_size: str
    ear_dll: Path
    game_args: tuple[str, ...]
    no_build: bool
    no_launch: bool
    prefix: Path
    stage_only: bool
    wine: str
    winecfg: str
    wineserver: str


def _copy_file(source: Path, destination: Path) -> None:
    """
    Copy a source file if its timestamp differs from the installed copy.

    Parameters
    ----------
    source : Path
        File to copy.
    destination : Path
        Installed file.
    """
    if not destination.exists() or source.stat().st_mtime_ns != destination.stat().st_mtime_ns:
        shutil.copy2(source, destination)


def _replace_binary(source: Path, destination: Path) -> None:
    """
    Replace a binary after copying it into the installation directory.

    Parameters
    ----------
    source : Path
        Rebuilt binary.
    destination : Path
        Installed binary.
    """
    with tempfile.NamedTemporaryFile(dir=destination.parent, delete=False) as temporary:
        temporary_path = Path(temporary.name)
    try:
        shutil.copy2(source, temporary_path)
        temporary_path.replace(destination)
    finally:
        temporary_path.unlink(missing_ok=True)


def _initialize_prefix(
    options: _Options, prefix: Path, installation: Path, environment: dict[str, str]
) -> None:
    """
    Configure a missing Wine prefix and wait for its registry files.

    Parameters
    ----------
    options : _Options
        Wine executable choices.
    prefix : Path
        Wine prefix directory.
    installation : Path
        Staged game directory.
    environment : dict[str, str]
        Environment with WINEPREFIX set.

    Raises
    ------
    ValueError
        A Wine executable is missing or prefix setup is incomplete.
    """
    required = (prefix / 'system.reg', prefix / 'drive_c/windows/system32/kernel32.dll',
                prefix / 'drive_c/windows/explorer.exe')
    if all(path.is_file() for path in required) and not options.configure_prefix:
        return
    winecfg = shutil.which(options.winecfg)
    if winecfg is None:
        message = f'winecfg executable {options.winecfg!r} was not found.'
        raise ValueError(message)
    wineserver = shutil.which(options.wineserver)
    if wineserver is None:
        message = f'wineserver executable {options.wineserver!r} was not found.'
        raise ValueError(message)
    click.echo('Opening winecfg. Close its window when prefix configuration is complete.')
    sp.run([str(Path(winecfg).resolve())], env=environment, cwd=installation, check=True)
    click.echo('Waiting for Wine prefix setup and registry writes to finish.')
    sp.run([str(Path(wineserver).resolve()), '-w'], env=environment, cwd=installation, check=True)
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        message = 'Wine prefix initialisation is incomplete:\n' + '\n'.join(missing)
        raise ValueError(message)


# ruff: ignore[complex-structure,too-many-branches]
def _stage(options: _Options) -> tuple[Path, Path]:
    """
    Stage CD data and rebuilt binaries into a Wine prefix.

    Parameters
    ----------
    options : _Options
        Source paths and build choices.

    Returns
    -------
    tuple[Path, Path]
        Prefix and installation directory.

    Raises
    ------
    ValueError
        Source data, rebuilt binaries, or the Wine prefix is invalid.
    """
    data = options.data.expanduser().resolve()
    prefix = options.prefix.expanduser().resolve()
    build = options.build_dir.expanduser().resolve()
    if not (data / 'BossRally.pod').is_file():
        message = f'BossRally.pod is absent from {data}. Use --data with the extracted CD.'
        raise ValueError(message)
    for name in ('BRString.dll',):
        if not (data / name).is_file():
            message = f'{name} is absent from the extracted CD directory {data}.'
            raise ValueError(message)
    rebuilt = {
        'BRD3D.dll': build / 'src/BRD3D.dll',
        'BRally.exe': build / 'src/win32/boot/BRally.exe',
        'BossRally.exe': build / 'src/win32/boot/BossRally.exe',
        'earpds.dll': options.ear_dll.expanduser().resolve(),
    }
    cache = build / 'CMakeCache.txt'
    if not cache.is_file():
        message = f'{build} is not configured. Configure it with a win-* preset first.'
        raise ValueError(message)
    if re.search(r'^TGR_ENABLE_PATCHES:BOOL=ON$', cache.read_text(), re.MULTILINE) is None:
        click.echo(f'Warning: {build} is built without TGR_ENABLE_PATCHES, so the game writes no '
                   'coastline.log and no crash dump.', err=True)
    if not options.no_build:
        click.echo(f'Building {build}.')
        cmake = shutil.which('cmake')
        if cmake is None:
            message = 'cmake executable was not found.'
            raise ValueError(message)
        sp.run([str(Path(cmake).resolve()), '--build', str(build)], check=True)
    compiler_match = re.search(r'^CMAKE_CXX_COMPILER:[^=]+=(.+)$',
                               cache.read_text(), re.MULTILINE)
    if compiler_match is None:
        message = f'CMAKE_CXX_COMPILER is absent from {cache}.'
        raise ValueError(message)
    compiler = Path(compiler_match.group(1))
    target = compiler.name.removesuffix('-g++')
    for name in ('libc++.dll', 'libunwind.dll'):
        runtime_path = compiler.parent.parent / target / 'bin' / name
        if not runtime_path.is_file():
            message = f'The build compiler could not locate {name}.'
            raise ValueError(message)
        rebuilt[name] = runtime_path
    missing = [str(path) for path in rebuilt.values() if not path.is_file()]
    if missing:
        message = ('Required binaries are absent:\n' + '\n'.join(missing)
                   + '\nBuild BRD3D, coastline_brally, and coastline_bossrally first.'
                   + '\nSelect the EAR sound DLL with --ear-dll.')
        raise ValueError(message)
    installation = prefix / 'drive_c/BossRally'
    if data == installation or data in installation.parents or installation in data.parents:
        message = 'The installation and extracted CD directories must be separate.'
        raise ValueError(message)
    installation.mkdir(parents=True, exist_ok=True)
    for source in data.rglob('*'):
        relative = source.relative_to(data)
        if any(part.lower() in {'msvcrt.dll', 'boot.exe', 'setvideo.exe', 'brally.exe',
                                'bossrally.exe', 'brd3d.dll', 'brglide.dll', 'earpds.dll'}
               for part in relative.parts):
            continue
        destination = installation / relative
        if source.is_dir():
            destination.mkdir(parents=True, exist_ok=True)
        elif source.is_file():
            destination.parent.mkdir(parents=True, exist_ok=True)
            _copy_file(source, destination)
    for name, source in rebuilt.items():
        _replace_binary(source, installation / name)
        click.echo(f'Copied {source} to {installation / name}')
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
                message = f'{drive} already maps another directory. Choose another --prefix.'
                raise ValueError(message)
        elif drive.exists():
            message = f'{drive} already exists. Choose another --prefix.'
            raise ValueError(message)
        else:
            drive.symlink_to(target, target_is_directory=True)
    click.echo(f'Prepared {installation}')
    return prefix, installation


@click.command(help='Stage Boss Rally and launch it under Wine.')
@click.option('--data', type=click.Path(path_type=Path),
              default=Path.home() / 'Downloads/Boss_Rally_ISO/data',
              help='Extracted CD data directory.')
@click.option('--prefix', type=click.Path(path_type=Path), default=ROOT / '.wiswa-ci/wineprefix',
              help='Wine prefix, created when absent.')
@click.option('--build-dir', type=click.Path(path_type=Path),
              default=ROOT / 'build/boss-rally/win-d3d',
              help='CMake directory with the rebuilt DLL and launchers.')
@click.option('--ear-dll', type=click.Path(path_type=Path),
              default=ROOT / '.wiswa-ci/earpds-unpacked.dll',
              help='EAR sound DLL to copy beside the game as earpds.dll.')
@click.option('--no-build', is_flag=True,
              help='Copy the binaries in --build-dir without building them first.')
@click.option('--stage-only', is_flag=True,
              help='Copy files without Wine; configure the prefix on a later run.')
@click.option('--no-launch', is_flag=True,
              help='Prepare and configure the Wine prefix without starting the game.')
@click.option('--desktop-size', default='800x600',
              help='Size of the Wine desktop window, such as 800x600.')
@click.option('--wine', default=lambda: os.environ.get('WINE', 'wine'),
              help='Wine executable; inherited Wine architecture settings are preserved.')
@click.option('--winecfg', default=lambda: os.environ.get('WINECFG', 'winecfg'),
              help='winecfg executable for initialising a fresh prefix.')
@click.option('--wineserver', default=lambda: os.environ.get('WINESERVER', 'wineserver'),
              help='wineserver executable used to wait for prefix setup to finish.')
@click.option('--configure-prefix', is_flag=True,
              help='Open winecfg even when the prefix already exists.')
@click.option('--debugger', is_flag=True,
              help='Start BRally.exe under winedbg. Type c to run it; bt prints a backtrace.')
@click.argument('game_args', nargs=-1, type=click.UNPROCESSED)
def main(**values: object) -> None:
    """
    Stage Boss Rally and launch it under Wine.

    Parameters
    ----------
    values : object
        Options parsed by Click.

    Raises
    ------
    click.BadParameter
        The desktop size is invalid.
    click.Abort
        Staging or launching failed.
    """
    options = _Options(**values)
    if re.fullmatch(r'[1-9][0-9]{2,4}x[1-9][0-9]{2,4}', options.desktop_size) is None:
        message = 'Use WIDTHxHEIGHT, such as 800x600.'
        raise click.BadParameter(message, param_hint='--desktop-size')
    try:
        _run(options)
    except (OSError, ValueError, sp.CalledProcessError) as error:
        click.echo(f'Error: {error}', err=True)
        raise click.Abort from error


def _run(options: _Options) -> None:
    """
    Configure Wine and run the staged game.

    Parameters
    ----------
    options : _Options
        Staging and launch options.

    Raises
    ------
    ValueError
        The selected Wine executable is missing.
    """
    prefix, installation = _stage(options)
    if options.stage_only:
        click.echo('Files staged. Wine prefix initialisation and registry '
                   'configuration are pending.')
        return
    wine = shutil.which(options.wine)
    if wine is None:
        message = f'Wine executable {options.wine!r} was not found.'
        raise ValueError(message)
    wine = str(Path(wine).resolve())
    environment = dict(os.environ, WINEPREFIX=str(prefix))
    _initialize_prefix(options, prefix, installation, environment)

    def run(*command: str) -> None:
        sp.run([wine, *command], env=environment, cwd=installation, check=True)

    click.echo('Configuring the Wine installation registry entry.')
    run('reg', 'add', INSTALL_KEY, '/v', 'Directory', '/t', 'REG_SZ',
        '/d', INSTALL_DIRECTORY, '/f', '/reg:32')
    run('reg', 'add', r'HKCU\Software\Wine\Drives', '/v', 'd:', '/t', 'REG_SZ',
        '/d', 'cdrom', '/f')
    if options.no_launch:
        click.echo('Wine prefix configured. Run this script again to start the game.')
        return
    game_args = options.game_args
    if game_args and game_args[0] == '--':
        game_args = game_args[1:]
    if options.debugger:
        # winedbg takes no desktop switch, so the prefix's default desktop is set instead.
        run('reg', 'add', r'HKCU\Software\Wine\Explorer', '/v', 'Desktop', '/t', 'REG_SZ',
            '/d', 'BossRally', '/f')
        run('reg', 'add', r'HKCU\Software\Wine\Explorer\Desktops', '/v', 'BossRally',
            '/t', 'REG_SZ', '/d', options.desktop_size, '/f')
        click.echo(f'Starting BRally.exe under winedbg in a {options.desktop_size} Wine desktop '
                   'window.')
        run('winedbg', INSTALL_DIRECTORY + r'\BRally.exe', *game_args)
        return
    click.echo(f'Starting BRally.exe in a {options.desktop_size} Wine desktop window.')
    run('explorer', f'/desktop=BossRally,{options.desktop_size}',
        INSTALL_DIRECTORY + r'\BRally.exe', *game_args)


if __name__ == '__main__':
    main()
