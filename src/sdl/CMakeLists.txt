# The stand-alone SDL game. It builds for the host with the host compiler, and SDL, argparse, and
# dr_libs come from vcpkg.
enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_C_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

# The SHA-1 and the asset chains of the host tools read the cartridge images. The music player of
# the Nintendo 64 release is shared C.
add_library(
  coastline_sdl STATIC
  config.cpp
  gpu.cpp
  importer.cpp
  input.cpp
  log.cpp
  music.cpp
  paths.cpp
  "${CMAKE_SOURCE_DIR}/src/audio.c"
  "${CMAKE_SOURCE_DIR}/tools/common/assetchain.cpp"
  "${CMAKE_SOURCE_DIR}/tools/common/sha1.cpp")
target_include_directories(coastline_sdl PRIVATE "${CMAKE_SOURCE_DIR}/src"
                                                 "${CMAKE_SOURCE_DIR}/tools/common")
target_link_libraries(coastline_sdl PRIVATE FLAC::FLAC ZLIB::ZLIB)
# A reconstructed routine retains the parameters of the original, used or not.
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/audio.c" PROPERTIES COMPILE_OPTIONS
                                                                         "-Wno-unused-parameter")
if(APPLE)
  enable_language(OBJCXX)
  target_sources(coastline_sdl PRIVATE paths-apple.mm)
  target_link_libraries(coastline_sdl PUBLIC "-framework Foundation")
endif()
target_include_directories(coastline_sdl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(coastline_sdl PRIVATE -Wall -Wextra)
target_link_libraries(coastline_sdl PUBLIC SDL3::SDL3)
if(WIN32)
  target_link_libraries(coastline_sdl PUBLIC shell32)
endif()

add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
                           COASTLINE_VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE coastline_sdl argparse::argparse)

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
endif()
