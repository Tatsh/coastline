{
  build_command: 'cmake --build --preset sdl --clean-first --parallel 2',
  configure_command: 'cmake --preset sdl -DCMAKE_BUILD_TYPE=Debug -DCOASTLINE_VIDEO=ON',
  documentation_steps: [
    {
      name: 'Install dependencies',
      run: 'sudo apt-get update\nsudo apt-get install -y doxygen graphviz\n',
    },
    {
      name: 'Configure',
      run: 'cmake -B "$GITHUB_WORKSPACE/build" -DBUILD_DOCS_ONLY=ON',
    },
    {
      name: 'Build documentation',
      run: 'cmake --build "$GITHUB_WORKSPACE/build" --target docs',
    },
    {
      name: 'Page count',
      run: "COUNT=$(find \"$GITHUB_WORKSPACE/build/html\" -name '*.html' | wc -l)\necho \"Generated $COUNT page(s)\"\ntest \"$COUNT\" -gt 0\n",
    },
  ],
  env: {
    VCPKG_ROOT: '${{ github.workspace }}/.vcpkg',
  },
  setup_steps: [
    {
      name: 'Install SDL build dependencies',
      run: 'sudo apt-get update\nsudo apt-get install -y autoconf-archive cmake libasound2-dev libavcodec-dev \\\n  libavformat-dev libavutil-dev libegl1-mesa-dev libltdl-dev libswresample-dev \\\n  libswscale-dev libwayland-dev libx11-dev libxcursor-dev libxext-dev libxfixes-dev \\\n  libxi-dev libxkbcommon-dev libxrandr-dev libxss-dev libxtst-dev ninja-build \\\n  pkg-config wayland-protocols\ngit clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"\n',
    },
  ],
  workflow_file: 'pages.yml',
}
