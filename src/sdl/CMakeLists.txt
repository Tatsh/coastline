# The stand-alone SDL game. Its libraries come from the system, vcpkg, or pinned releases, as
# cmake/dependencies.cmake chooses.
enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
if(MSVC)
  # CMake has no flag of C23 for MSVC, whose newest C is /std:clatest.
  add_compile_options("$<$<COMPILE_LANGUAGE:C>:/std:clatest>")
else()
  set(CMAKE_C_STANDARD 23)
endif()
# The warnings of every library of the game, and the warning a reconstructed routine with unused
# parameters turns off.
if(MSVC)
  set(COASTLINE_WARNINGS /W4 /utf-8)
  set(COASTLINE_NO_UNUSED_PARAMETER /wd4100)
else()
  set(COASTLINE_WARNINGS -Wall -Wextra)
  set(COASTLINE_NO_UNUSED_PARAMETER -Wno-unused-parameter)
endif()

include("${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake")
# The pinned releases are the versions of the vcpkg baseline.
coastline_dependency(
  SDL3
  PACKAGE SDL3
  FIND_ARGS CONFIG
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
  HASH 9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
  OPTIONS SDL_SHARED=OFF SDL_STATIC=ON SDL_TEST_LIBRARY=OFF SDL_EXAMPLES=OFF SDL_TESTS=OFF
          SDL_INSTALL=OFF)
coastline_dependency(
  ARGPARSE
  PACKAGE argparse
  FIND_ARGS CONFIG
  URL https://github.com/p-ranav/argparse/archive/refs/tags/v3.2.tar.gz
  HASH 9dcb3d8ce0a41b2a48ac8baa54b51a9f1b6a2c52dd374e28cc713bab0568ec98
  OPTIONS ARGPARSE_INSTALL=OFF)
# {fmt} in place of std::format, which the C++ library of macOS has only from 13.3.
coastline_dependency(
  FMT
  PACKAGE fmt
  FIND_ARGS CONFIG
  URL https://github.com/fmtlib/fmt/releases/download/12.2.0/fmt-12.2.0.zip
  HASH a2f4a8d51178f954e4c339007f77edd76ba0cb2e36f87a48e5a5403d9be5878f
  OPTIONS FMT_INSTALL=OFF FMT_TEST=OFF FMT_DOC=OFF)
coastline_dependency(
  FLAC
  PACKAGE FLAC
  FIND_ARGS CONFIG
  URL https://github.com/xiph/flac/releases/download/1.5.0/flac-1.5.0.tar.xz
  HASH f2c1c76592a82ffff8413ba3c4a1299b6c7ab06c734dee03fd88630485c2b920
  OPTIONS BUILD_CXXLIBS=OFF BUILD_PROGRAMS=OFF BUILD_EXAMPLES=OFF BUILD_TESTING=OFF BUILD_DOCS=OFF
          INSTALL_MANPAGES=OFF WITH_OGG=OFF)
coastline_dependency(
  NLOHMANN_JSON
  PACKAGE nlohmann_json
  FIND_ARGS CONFIG
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  HASH 42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  OPTIONS JSON_BuildTests=OFF JSON_Install=OFF)
coastline_dependency(
  ZLIB
  PACKAGE ZLIB
  URL https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz
  HASH bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16
  OPTIONS ZLIB_BUILD_TESTING=OFF ZLIB_BUILD_SHARED=OFF ZLIB_INSTALL=OFF)
# The static zlib of the release has an alias of its own.
if(NOT TARGET ZLIB::ZLIB AND TARGET zlibstatic)
  add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

# EAR, the sound engine of the PC release, on SDL audio.
add_subdirectory("${CMAKE_SOURCE_DIR}/src/ear" "${CMAKE_BINARY_DIR}/src/ear")

# The race engine of the PC release, shared with the Windows libraries. Its libraries take the
# settings of src/CMakeLists.txt.
add_library(coastline_options INTERFACE)
target_compile_options(coastline_options INTERFACE ${COASTLINE_WARNINGS}
                                                   ${COASTLINE_NO_UNUSED_PARAMETER})
# MSVC compiles the C sources marked as C++ with the /TP CMake gives it.
if(NOT MSVC)
  target_compile_options(coastline_options INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:-x;c++>")
endif()
# Windows 7 is the oldest Windows SDL 3 runs on, with MSVC and MinGW alike.
if(WIN32)
  target_compile_definitions(coastline_options INTERFACE _WIN32_WINNT=0x0601 WINVER=0x0601)
endif()
target_include_directories(coastline_options INTERFACE "${CMAKE_SOURCE_DIR}/src"
                                                     "${CMAKE_SOURCE_DIR}/src/libultra/include")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/common" "${CMAKE_BINARY_DIR}/src/common")

# The shaders are GLSL, compiled to SPIR-V for Vulkan and translated to MSL for Metal by the host
# tools, and embedded in the executable.
set(_host_tools "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools")
coastline_tool(
  GLSLANG GLSLANG_VALIDATOR
  PROGRAM glslangValidator
  HINT "${_host_tools}/glslang"
  TARGET glslang-standalone
  URL https://github.com/KhronosGroup/glslang/archive/refs/tags/16.4.0.tar.gz
  HASH c634d6237eb0cc04d5ddf5dc9955daa175d82b0f8797acab45b49965e9f6df13
  OPTIONS BUILD_EXTERNAL=OFF ENABLE_OPT=OFF GLSLANG_TESTS=OFF GLSLANG_ENABLE_INSTALL=OFF)
coastline_tool(
  SPIRV_CROSS SPIRV_CROSS
  PROGRAM spirv-cross
  HINT "${_host_tools}/spirv-cross"
  TARGET spirv-cross
  URL https://github.com/KhronosGroup/SPIRV-Cross/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz
  HASH f708982e88b763ef5b0394ed468fc2c3628f68cdab89f2e7a946cd628c04e721
  OPTIONS SPIRV_CROSS_ENABLE_TESTS=OFF SPIRV_CROSS_SKIP_INSTALL=ON)
set(_shader_headers)
foreach(_shader model.frag model.vert tl.frag tl.vert)
  string(REPLACE "." "_" _stem "${_shader}")
  string(TOUPPER "${_stem}" _name)
  set(_source "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${_shader}")
  set(_spirv "${CMAKE_CURRENT_BINARY_DIR}/shaders/${_stem}.spv")
  set(_msl "${CMAKE_CURRENT_BINARY_DIR}/shaders/${_stem}.metal")
  set(_spirv_header "${CMAKE_CURRENT_BINARY_DIR}/shaders/${_stem}_spirv.h")
  set(_msl_header "${CMAKE_CURRENT_BINARY_DIR}/shaders/${_stem}_msl.h")
  add_custom_command(
    OUTPUT "${_spirv_header}" "${_msl_header}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMAND "${GLSLANG_VALIDATOR}" -V --quiet -o "${_spirv}" "${_source}"
    COMMAND "${SPIRV_CROSS}" --msl --msl-version 20000 --output "${_msl}" "${_spirv}"
    COMMAND "${CMAKE_COMMAND}" -DINPUT=${_spirv} -DOUTPUT=${_spirv_header} -DNAME=${_name}_SPIRV
            -P "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
    COMMAND "${CMAKE_COMMAND}" -DINPUT=${_msl} -DOUTPUT=${_msl_header} -DNAME=${_name}_MSL -P
            "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
    DEPENDS "${_source}" "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
    COMMENT "Compiling the shader ${_shader}"
    VERBATIM)
  list(APPEND _shader_headers "${_spirv_header}" "${_msl_header}")
endforeach()
add_custom_target(coastline_shaders DEPENDS ${_shader_headers})

# The manifests of the assets of each cartridge, embedded for the import.
set(_manifest_headers)
foreach(_release eu jp us)
  string(TOUPPER "${_release}" _name)
  set(_source "${CMAKE_CURRENT_SOURCE_DIR}/cartridge/${_release}.txt")
  set(_header "${CMAKE_CURRENT_BINARY_DIR}/cartridge/${_release}.h")
  add_custom_command(
    OUTPUT "${_header}"
    COMMAND "${CMAKE_COMMAND}" -DINPUT=${_source} -DOUTPUT=${_header} -DNAME=CARTRIDGE_${_name} -P
            "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
    DEPENDS "${_source}" "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
    COMMENT "Embedding the manifest of the ${_release} cartridge"
    VERBATIM)
  list(APPEND _manifest_headers "${_header}")
endforeach()
add_custom_target(coastline_manifests DEPENDS ${_manifest_headers})

# The SHA-1 and the asset chains of the host tools read the cartridge images. The music player of
# the Nintendo 64 release is shared C.
add_library(
  coastline_sdl STATIC
  config.cpp
  earsound.cpp
  enginecheck.cpp
  frontend.cpp
  ghostfile.cpp
  gpu.cpp
  importer.cpp
  indexedimage.cpp
  input.cpp
  intro.cpp
  log.cpp
  mainmenu.cpp
  menutext.cpp
  mipscpu.cpp
  modelview.cpp
  music.cpp
  n64attract.cpp
  n64model.cpp
  paths.cpp
  pending.cpp
  ptr32.cpp
  racedemo.cpp
  raceinput.cpp
  racemusic.cpp
  raceview.cpp
  runtimebuffers.cpp
  scenerenderer.cpp
  session.cpp
  stringresources.cpp
  tlrenderer.cpp
  trackload.cpp
  "${CMAKE_SOURCE_DIR}/src/common/audio.c"
  "${CMAKE_SOURCE_DIR}/tools/common/assetchain.cpp"
  "${CMAKE_SOURCE_DIR}/tools/common/sha1.cpp")
target_include_directories(coastline_sdl PRIVATE "${CMAKE_SOURCE_DIR}/src"
                                                 "${CMAKE_SOURCE_DIR}/src/libultra/include"
                                                 "${CMAKE_SOURCE_DIR}/tools/common"
                                                 "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(coastline_sdl coastline_shaders coastline_manifests)
target_link_libraries(coastline_sdl PRIVATE FLAC::FLAC fmt::fmt nlohmann_json::nlohmann_json
                                            ZLIB::ZLIB)
# A reconstructed routine retains the parameters of the original, used or not.
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/common/audio.c" pending.cpp
                            PROPERTIES COMPILE_OPTIONS "${COASTLINE_NO_UNUSED_PARAMETER}")
# The interpreter rounds each operation of the floating-point unit of the console on its own. MSVC
# does not contract operations without /fp:contract.
if(NOT MSVC)
  set_source_files_properties(mipscpu.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
endif()
if(APPLE)
  enable_language(OBJCXX)
  target_sources(coastline_sdl PRIVATE paths-apple.mm)
  target_link_libraries(coastline_sdl PUBLIC "-framework Foundation")
endif()
target_include_directories(coastline_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(coastline_sdl PRIVATE ${COASTLINE_WARNINGS})
target_link_libraries(coastline_sdl PUBLIC SDL3::SDL3 coastline_ear)
if(WIN32)
  target_link_libraries(coastline_sdl PUBLIC shell32)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
                           COASTLINE_VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${COASTLINE_WARNINGS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE coastline_sdl argparse::argparse)
# The race engine calls the routines of the platform in coastline_sdl, and of SDL where the Windows
# libraries call Windows. The game links every routine of the engine, called or not, and a routine
# the engine calls that the game lacks fails the link.
target_link_libraries(coastline_common PRIVATE ${COASTLINE_COMMON_LIBULTRA_LIBRARIES}
                                               $<LINK_ONLY:coastline_sdl> SDL3::SDL3)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,coastline_common>")

install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
# The shared SDL of vcpkg is installed beside the game on Windows, and in the library directory
# elsewhere. The SDL of the system is not installed, and the pinned release is static.
get_target_property(_sdl_imported SDL3::SDL3 IMPORTED)
if(TARGET SDL3::SDL3-shared AND _sdl_imported AND NOT USE_SYSTEM_SDL3)
  install(IMPORTED_RUNTIME_ARTIFACTS SDL3::SDL3-shared RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
          LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()
if(NOT WIN32 AND NOT APPLE)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES INSTALL_RPATH
                                                         "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  install(FILES io.github.tatsh.coastline.desktop
          DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt" "${CMAKE_SOURCE_DIR}/README.md"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}")
