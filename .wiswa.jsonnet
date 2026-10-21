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
  clang_format_args: "$(git ls-files 'src/*.c' 'src/*.cpp' 'src/*.h' 'tools/*.cpp' 'tools/*.h')",
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
  // superbuild, and a workflow for each that configures and builds in one command. No cross
  // toolchain is chosen yet. The vcpkg preset is kept for inheritance and hidden. The n64 and win
  // presets gain their toolchain files when the builds are set up.
  local variants = ['n64-' + region for region in ['us', 'jp', 'eu']] +
                   ['win-' + renderer for renderer in ['d3d', 'glide']],
  cmake_presets+: {
    version: 6,
    configurePresets: [super.configurePresets[0] { hidden: true }] + [
      {
        binaryDir: '${sourceDir}/build/${presetName}',
        generator: 'Ninja',
        hidden: true,
        name: 'base',
      },
      { cacheVariables: { TGR_PLATFORM: 'n64' }, hidden: true, inherits: 'base', name: 'n64' },
      { cacheVariables: { TGR_PLATFORM: 'win' }, hidden: true, inherits: 'base', name: 'win' },
    ] + [
      { cacheVariables: { TGR_REGION: region }, inherits: 'n64', name: 'n64-' + region }
      for region in ['us', 'jp', 'eu']
    ] + [
      { cacheVariables: { TGR_RENDERER: renderer }, inherits: 'win', name: 'win-' + renderer }
      for renderer in ['d3d', 'glide']
    ] + [
      { cacheVariables: { TGR_VARIANTS: 'all' }, inherits: 'base', name: 'all' },
    ],
    buildPresets: [
      { configurePreset: variant, name: variant, targets: ['archives'] }
      for variant in variants
    ] + [{ configurePreset: 'all', name: 'all' }],
    workflowPresets: [
      {
        name: preset,
        steps: [{ name: preset, type: 'configure' }, { name: preset, type: 'build' }],
      }
      for preset in variants + ['all']
    ],
  },
  // ROM images used for comparison.
  shared_ignore+: ['*.n64', '*.v64', '*.z64', '__pycache__/'],
  // The generated progress tables list every routine name and prototype.
  package_json+: {
    cspell+: {
      ignorePaths+: ['progress/**'],
    },
  },
  // The host tools under tools/ need these packages to build. The N64 build does not use vcpkg.
  vcpkg+: {
    dependencies: [
      { host: true, name: 'argparse' },
      { host: true, name: 'spdlog' },
      { host: true, name: 'zlib' },
    ],
  },
  vscode+: {
    settings+: { 'cmake.useCMakePresets': 'always' },
  },
}
