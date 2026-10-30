# The stand-alone SDL game. It builds for the host with the host compiler, and SDL, argparse, and
# dr_libs come from vcpkg.
enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

# EAR, the sound engine of the PC release, on SDL audio.
add_subdirectory("${CMAKE_SOURCE_DIR}/src/ear" "${CMAKE_BINARY_DIR}/src/ear")

# The race engine of the PC release, shared with the Windows libraries. Its libraries take the
# settings of src/CMakeLists.txt.
add_library(coastline_options INTERFACE)
target_compile_options(coastline_options INTERFACE -Wall -Wextra -Wno-unused-parameter
                                                   "$<$<COMPILE_LANGUAGE:CXX>:-x;c++>")
target_include_directories(coastline_options INTERFACE "${CMAKE_SOURCE_DIR}/src"
                                                     "${CMAKE_SOURCE_DIR}/src/libultra/include")
add_subdirectory("${CMAKE_SOURCE_DIR}/src/common" "${CMAKE_BINARY_DIR}/src/common")

# The shaders are GLSL, compiled to SPIR-V for Vulkan and translated to MSL for Metal by the host
# tools of vcpkg, and embedded in the executable.
set(_host_tools "${VCPKG_INSTALLED_DIR}/${VCPKG_HOST_TRIPLET}/tools")
find_program(GLSLANG_VALIDATOR glslangValidator HINTS "${_host_tools}/glslang" REQUIRED)
find_program(SPIRV_CROSS spirv-cross HINTS "${_host_tools}/spirv-cross" REQUIRED)
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
  modelview.cpp
  music.cpp
  n64model.cpp
  paths.cpp
  pending.cpp
  ptr32.cpp
  racemusic.cpp
  runtimebuffers.cpp
  scenerenderer.cpp
  stringresources.cpp
  tlrenderer.cpp
  "${CMAKE_SOURCE_DIR}/src/common/audio.c"
  "${CMAKE_SOURCE_DIR}/tools/common/assetchain.cpp"
  "${CMAKE_SOURCE_DIR}/tools/common/sha1.cpp")
target_include_directories(coastline_sdl PRIVATE "${CMAKE_SOURCE_DIR}/src"
                                                 "${CMAKE_SOURCE_DIR}/src/libultra/include"
                                                 "${CMAKE_SOURCE_DIR}/tools/common"
                                                 "${CMAKE_CURRENT_BINARY_DIR}")
add_dependencies(coastline_sdl coastline_shaders coastline_manifests)
target_link_libraries(coastline_sdl PRIVATE FLAC::FLAC nlohmann_json::nlohmann_json ZLIB::ZLIB)
# A reconstructed routine retains the parameters of the original, used or not.
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/common/audio.c" pending.cpp
                            PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter")
if(APPLE)
  enable_language(OBJCXX)
  target_sources(coastline_sdl PRIVATE paths-apple.mm)
  target_link_libraries(coastline_sdl PUBLIC "-framework Foundation")
endif()
target_include_directories(coastline_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(coastline_sdl PRIVATE -Wall -Wextra)
target_link_libraries(coastline_sdl PUBLIC SDL3::SDL3 coastline_ear)
if(WIN32)
  target_link_libraries(coastline_sdl PUBLIC shell32)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
                           COASTLINE_VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE coastline_sdl argparse::argparse)
# The race engine calls the routines of the platform in coastline_sdl, and of SDL where the Windows
# libraries call Windows. The game links every routine of the engine, called or not, and a routine
# the engine calls that the game lacks fails the link.
target_link_libraries(coastline_common PRIVATE ${COASTLINE_COMMON_LIBULTRA_LIBRARIES}
                                               $<LINK_ONLY:coastline_sdl> SDL3::SDL3)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,coastline_common>")

install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
# A shared SDL is installed beside the game on Windows, and in the library directory elsewhere. A
# static SDL is part of the executable.
if(TARGET SDL3::SDL3-shared)
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
