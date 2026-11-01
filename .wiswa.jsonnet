{
  uses_user_defaults: true,
  project_name: 'coastline',
  project_type: 'c',
  version: '0.0.0',
  description: 'Reconstructed source of the Nintendo 64 game Top Gear Rally.',
  keywords: ['decompilation', 'game', 'nintendo 64', 'reverse engineering', 'top gear rally'],
  want_codeql: false,
  want_tests: false,
  want_winget: false,
  want_snap: true,
  want_flatpak: true,
  publishing+: {
    flathub: 'sh.tat.coastline',
  },
  flatpak_image: 'ghcr.io/flathub-infra/flatpak-github-actions:freedesktop-25.08',
  flatpak+: {
    'finish-args': [
      '--device=dri',
      '--device=input',
      '--filesystem=/media:ro',
      '--filesystem=/run/media:ro',
      '--filesystem=home:ro',
      '--share=ipc',
      '--socket=fallback-x11',
      '--socket=pulseaudio',
      '--socket=wayland',
    ],
    modules: [
      {
        buildsystem: 'meson',
        cleanup: ['/include', '/lib/pkgconfig', '/lib/libgamemodeauto*'],
        'config-opts': [
          '-Dwith-examples=false',
          '-Dwith-pam-renicing=false',
          '-Dwith-sd-bus-provider=no-daemon',
          '-Dwith-systemd-group=false',
          '-Dwith-systemd-user-unit=false',
          '-Dwith-util=false',
        ],
        name: 'gamemode',
        'post-install': [
          'install -Dm644 LICENSE.txt "${FLATPAK_DEST}/share/licenses/gamemode/LICENSE.txt"',
        ],
        sources: [
          {
            commit: 'c54d6d4243b0dd0afcb49f2c9836d432da171a2b',
            tag: '1.8.2',
            type: 'git',
            url: 'https://github.com/FeralInteractive/gamemode.git',
          },
        ],
      },
      {
        buildsystem: 'cmake-ninja',
        'build-options': {
          'build-args': ['--share=network'],
        },
        builddir: true,
        'config-opts': [
          '-DCMAKE_BUILD_TYPE=Release',
          '-DTGR_ENABLE_PATCHES=ON',
          '-DTGR_PLATFORM=sdl',
        ],
        name: 'coastline',
        sources: [
          {
            path: '.',
            skip: ['.claude', '.wiswa-ci', 'build', 'node_modules'],
            type: 'dir',
          },
        ],
      },
    ],
    'runtime-version': '25.08',
  },
  snapcraft+: {
    'adopt-info': 'coastline',
    apps+: {
      coastline+: {
        command: 'usr/bin/coastline',
        'command-chain': ['bin/gpu-2404-wrapper'],
        'common-id': 'sh.tat.coastline',
        desktop: 'usr/share/applications/sh.tat.coastline.desktop',
        environment: {
          XDG_CACHE_HOME: '$SNAP_USER_COMMON/.cache',
          XDG_DATA_HOME: '$SNAP_USER_COMMON/.local/share',
          XDG_STATE_HOME: '$SNAP_USER_COMMON/.local/state',
        },
        plugs: [
          'audio-playback',
          'desktop',
          'home',
          'joystick',
          'opengl',
          'removable-media',
          'wayland',
          'x11',
        ],
      },
    },
    contact:: null,
    description: |||
      Coastline is a stand-alone game of Top Gear Rally and Boss Rally. It needs a Boss Rally disc
      image and a Top Gear Rally cartridge image, which `coastline import --cd` and
      `coastline import --rom` import.
    |||,
    donation:: null,
    grade: 'devel',
    icon: 'src/sdl/sh.tat.coastline.svg',
    issues:: null,
    layout: {
      '/usr/share/drirc.d': { symlink: '$SNAP/gpu-2404/drirc.d' },
      '/usr/share/libdrm': { bind: '$SNAP/gpu-2404/libdrm' },
      '/usr/share/X11/XErrorDB': { symlink: '$SNAP/gpu-2404/X11/XErrorDB' },
    },
    parts+: {
      coastline: {
        'build-packages': [
          'autoconf-archive',
          'git',
          'libasound2-dev',
          'libdecor-0-dev',
          'libegl1-mesa-dev',
          'libpulse-dev',
          'libwayland-dev',
          'libx11-dev',
          'libxcursor-dev',
          'libxext-dev',
          'libxfixes-dev',
          'libxi-dev',
          'libxkbcommon-dev',
          'libxrandr-dev',
          'libxss-dev',
          'libxtst-dev',
          'ninja-build',
          'pkg-config',
          'wayland-protocols',
        ],
        'cmake-generator': 'Ninja',
        'cmake-parameters': [
          '-DCMAKE_BUILD_TYPE=Release',
          '-DCMAKE_INSTALL_PREFIX=/usr',
          '-DTGR_ENABLE_PATCHES=ON',
          '-DTGR_PLATFORM=sdl',
        ],
        'override-pull': |||
          craftctl default
          craftctl set version="$(git describe --always --tags)"
        |||,
        plugin: 'cmake',
        source: '.',
        'stage-packages': [
          'libasound2t64',
          'libdecor-0-0',
          'libgamemode0',
          'libpulse0',
          'libwayland-client0',
          'libwayland-cursor0',
          'libwayland-egl1',
          'libx11-6',
          'libxcursor1',
          'libxext6',
          'libxfixes3',
          'libxi6',
          'libxkbcommon0',
          'libxrandr2',
          'libxss1',
        ],
      },
      'gpu-2404': {
        after: ['coastline'],
        'override-prime': |||
          craftctl default
          ${CRAFT_PART_SRC}/bin/gpu-2404-cleanup mesa-2404
        |||,
        plugin: 'dump',
        prime: ['bin/gpu-2404-wrapper'],
        source: 'https://github.com/canonical/gpu-snap.git',
      },
    },
    platforms+: { amd64: null, arm64: null },
    plugs: {
      'gpu-2404': {
        'default-provider': 'mesa-2404',
        interface: 'content',
        target: '$SNAP/gpu-2404',
      },
    },
    'source-code':: null,
    summary: 'Top Gear Rally and Boss Rally in one stand-alone game',
    title:: null,
    version:: null,
    website:: null,
  },
  clang_format_args: "$(git ls-files 'src/*.c' 'src/*.cpp' 'src/*.h' 'src/*.mm' 'tests/*.c' 'tests/*.cpp' 'tools/*.cpp' 'tools/*.h')",
  clang_format+: {
    IncludeBlocks: 'Regroup',
    // The C and POSIX headers come before the SDK headers (ultra64.h and the others).
    IncludeCategories: [
      {
        CaseSensitive: true,
        Priority: 1,
        Regex: '^<(%s)\\.h>$' % std.join('|', [
          'assert',
          'ctype',
          'errno',
          'fcntl',
          'float',
          'inttypes',
          'limits',
          'locale',
          'math',
          'setjmp',
          'signal',
          'stdarg',
          'stdbool',
          'stddef',
          'stdint',
          'stdio',
          'stdlib',
          'string',
          'strings',
          'sys/[a-z_]+',
          'time',
          'unistd',
        ]),
      },
      {
        CaseSensitive: true,
        Priority: 1,
        Regex: '^<[a-z_]+>$',
      },
      {
        CaseSensitive: true,
        Priority: 2,
        Regex: '^<',
      },
      {
        CaseSensitive: true,
        Priority: 3,
        Regex: '^"',
      },
    ],
    ReflowComments: true,
  },
  // One configure and build preset per variant, the all preset that builds every variant as a
  // superbuild, and a workflow for each that configures and builds in one command. The vcpkg
  // preset is retained for inheritance and hidden.
  local variants = ['n64-' + region for region in ['us', 'jp', 'eu']] +
                   ['win-' + renderer for renderer in ['d3d', 'glide']],
  cmake_presets+: {
    version: 6,
    configurePresets: [super.configurePresets[0] { hidden: true }] + [
      {
        binaryDir: '${sourceDir}/build/${presetName}',
        cacheVariables: { CMAKE_EXPORT_COMPILE_COMMANDS: 'ON' },
        generator: 'Ninja',
        hidden: true,
        name: 'base',
      },
      {
        cacheVariables: {
          CMAKE_TOOLCHAIN_FILE: '${sourceDir}/cmake/n64-toolchain.cmake',
          TGR_PLATFORM: 'n64',
        },
        hidden: true,
        inherits: 'base',
        name: 'n64',
      },
      {
        cacheVariables: {
          CMAKE_TOOLCHAIN_FILE: '${sourceDir}/cmake/win-toolchain.cmake',
          TGR_PLATFORM: 'win',
        },
        hidden: true,
        inherits: 'base',
        name: 'win',
      },
    ] + [
      { cacheVariables: { TGR_REGION: region }, inherits: 'n64', name: 'n64-' + region }
      for region in ['us', 'jp', 'eu']
    ] + [
      { cacheVariables: { TGR_RENDERER: renderer }, inherits: 'win', name: 'win-' + renderer }
      for renderer in ['d3d', 'glide']
    ] + [
      { cacheVariables: { TGR_VARIANTS: 'all' }, inherits: 'base', name: 'all' },
      // The tests build for the host with the host compiler.
      { cacheVariables: { TGR_TESTS: 'ON' }, inherits: 'base', name: 'tests' },
      // The stand-alone SDL game builds for the host, with its libraries from vcpkg.
      {
        cacheVariables: {
          CMAKE_BUILD_TYPE: 'RelWithDebInfo',
          CMAKE_TOOLCHAIN_FILE: '$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake',
          TGR_PLATFORM: 'sdl',
          VCPKG_MANIFEST_FEATURES: 'sdl',
        },
        inherits: 'base',
        name: 'sdl',
      },
    ],
    buildPresets: [
      { configurePreset: preset, name: preset }
      for preset in variants + ['all', 'tests', 'sdl']
    ],
    testPresets: [
      { configurePreset: 'tests', name: 'tests', output: { outputOnFailure: true } },
    ],
    workflowPresets: [
      {
        name: preset,
        steps: [{ name: preset, type: 'configure' }, { name: preset, type: 'build' }],
      }
      for preset in variants + ['all', 'sdl']
    ] + [
      {
        name: 'tests',
        steps: [
          { name: 'tests', type: 'configure' },
          { name: 'tests', type: 'build' },
          { name: 'tests', type: 'test' },
        ],
      },
    ],
  },
  // ROM images used for comparison.
  shared_ignore+: ['*.n64', '*.v64', '*.z64', '__pycache__/'],
  // The generated progress tables list every routine name and prototype, the assembly splat
  // generated is not prose, and the vendored sources retain their upstream style. The worktrees of
  // agents are copies of the tree with their own build output, and .wiswa-ci holds scratch builds
  // too large for markdownlint to walk.
  package_json+: {
    cspell+: {
      ignorePaths+: [
        '.claude/worktrees/**',
        '3rdparty/**',
        'asm/**',
        'docs/*.tags',
        'progress/**',
        'src/sdl/cartridge/*.txt',
        'tools/splat/symbol_addrs.*.txt',
      ],
    },
    'markdownlint-cli2'+: {
      // Only the root .gitignore, not one in every directory of the scratch builds.
      gitignore: '.gitignore',
      ignores: ['.claude/worktrees/**', '.wiswa-ci/**', '3rdparty/**'],
    },
  },
  pre_commit_config+: {
    exclude: '^(3rdparty|asm)/',
    // The tag file of cppreference is an upstream file larger than the default limit, and so are
    // the font atlases of the Glide library as the bytes of the original.
    repos: [
      repo + (if std.objectHas(repo, 'hooks') then {
                hooks: [
                  if hook.id == 'check-added-large-files' then
                    hook {
                      exclude: '^docs/cppreference\\.tags$|' +
                               '^src/win32/glide/fontatlas\\.c$|' + hook.exclude,
                    }
                  else hook
                  for hook in repo.hooks
                ],
              } else {})
      for repo in super.repos
    ],
  },
  gitattributes+: [
    '/3rdparty/** -text linguist-vendored',
    '/asm/** linguist-generated',
  ],
  // clang-format formats the Objective-C++ of the SDL game.
  prettierignore+: ['*.mm', '/.claude/worktrees/', '/3rdparty/', '/asm/'],
  // The host tools under tools/ need these packages to build. The N64 build does not use vcpkg.
  vcpkg+: {
    dependencies: [
      { host: true, name: 'argparse' },
      { host: true, name: 'spdlog' },
      { host: true, name: 'zlib' },
    ],
  },
  vscode+: {
    // IntelliSense targets the USA cartridge build with the packaged libdragon compiler on PATH.
    // CMake Tools supplies the flags of each file once a preset is configured.
    c_cpp: {
      configurations: [
        {
          cStandard: 'gnu89',
          compilerArgs: ['-G0', '-ffreestanding', '-fno-pic', '-mabi=32', '-march=vr4300', '-mno-abicalls'],
          compilerPath: 'mips64-libdragon-elf-gcc',
          compileCommands: '${workspaceFolder}/build/n64-us/compile_commands.json',
          configurationProvider: 'ms-vscode.cmake-tools',
          defines: [
            'TGR_PLATFORM=1',
            'TGR_PLATFORM_N64=1',
            'TGR_PLATFORM_WIN=2',
            'TGR_REGION=1',
            'TGR_REGION_EU=3',
            'TGR_REGION_JP=2',
            'TGR_REGION_US=1',
            'TGR_RENDERER=1',
            'TGR_RENDERER_D3D=1',
            'TGR_RENDERER_GLIDE=2',
          ],
          includePath: [
            '${workspaceFolder}/3rdparty/zlib-1.0.4',
            '${workspaceFolder}/src',
            '${workspaceFolder}/src/libultra/include',
          ],
          intelliSenseMode: 'linux-gcc-x86',
          name: 'N64 (USA)',
        },
      ] + [
        {
          cStandard: 'c23',
          compilerArgs: ['-m32'],
          compilerPath: '/usr/lib/mingw64-toolchain/bin/x86_64-w64-mingw32-g++',
          compileCommands: '${workspaceFolder}/build/win-' + renderer + '/compile_commands.json',
          configurationProvider: 'ms-vscode.cmake-tools',
          cppStandard: 'c++23',
          defines: [
            'TGR_PLATFORM=2',
            'TGR_PLATFORM_N64=1',
            'TGR_PLATFORM_WIN=2',
            'TGR_REGION=1',
            'TGR_REGION_EU=3',
            'TGR_REGION_JP=2',
            'TGR_REGION_US=1',
            'TGR_RENDERER=' + (if renderer == 'd3d' then '1' else '2'),
            'TGR_RENDERER_D3D=1',
            'TGR_RENDERER_GLIDE=2',
          ],
          includePath: [
            '${workspaceFolder}/src',
            '${workspaceFolder}/src/libultra/include',
            '/usr/lib/mingw64-toolchain/x86_64-w64-mingw32/include',
            '/usr/lib/mingw64-toolchain/lib/gcc/x86_64-w64-mingw32/16/include/',
          ],
          intelliSenseMode: 'windows-gcc-x86',
          name: 'Windows (' + (if renderer == 'd3d' then 'Direct3D' else 'Glide') + ')',
        }
        for renderer in ['d3d', 'glide']
      ],
      env: {},
      version: 4,
    },
    settings+: {
      // The scratch builds and data of .wiswa-ci are not part of the project.
      'C_Cpp.files.exclude': { '**/.wiswa-ci': true },
      'cmake.useCMakePresets': 'always',
      'files.watcherExclude': { '**/.wiswa-ci/**': true },
      'search.exclude': { '**/.wiswa-ci': true },
    },
  },
}
