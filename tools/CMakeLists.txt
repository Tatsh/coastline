# The main build runs the host tools in this project to extract the assets of the original ROM.
# The main build configures this project with the host compiler. The tools are never installed.
#
# Each dependency comes from vcpkg when the vcpkg toolchain is in use, otherwise from the system,
# and otherwise from the pinned release below. zlib comes only from vcpkg or the system. The vcpkg
# manifest is vcpkg.json at the repository root, selected through VCPKG_MANIFEST_DIR.
cmake_minimum_required(VERSION 3.25)
project(coastline_tools CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# The options below apply to the pinned releases only when a release is downloaded.
set(ARGPARSE_INSTALL OFF)
set(SPDLOG_INSTALL OFF)
FetchContent_Declare(
  argparse
  URL https://github.com/p-ranav/argparse/archive/refs/tags/v3.2.tar.gz
  URL_HASH SHA256=9dcb3d8ce0a41b2a48ac8baa54b51a9f1b6a2c52dd374e28cc713bab0568ec98
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_Declare(
  spdlog
  URL https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz
  URL_HASH SHA256=d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744
  FIND_PACKAGE_ARGS CONFIG)
FetchContent_MakeAvailable(argparse spdlog)
find_package(ZLIB REQUIRED)

# Reports whether a dependency came from vcpkg, the system, or a download. location is a path
# from the package search, empty when the package was downloaded.
function(coastline_tools_report_source dependency location)
  if(NOT location)
    set(_source "the pinned release (FetchContent)")
  elseif(VCPKG_INSTALLED_DIR)
    string(FIND "${location}" "${VCPKG_INSTALLED_DIR}" _vcpkg_at)
    if(_vcpkg_at EQUAL 0)
      set(_source "vcpkg (${location})")
    else()
      set(_source "the system (${location})")
    endif()
  else()
    set(_source "the system (${location})")
  endif()
  message(STATUS "${dependency} comes from ${_source}.")
endfunction()

foreach(_package argparse spdlog)
  set(_location "")
  if(${_package}_FOUND)
    set(_location "${${_package}_DIR}")
  endif()
  coastline_tools_report_source(${_package} "${_location}")
endforeach()
list(GET ZLIB_INCLUDE_DIRS 0 _zlib_location)
coastline_tools_report_source(zlib "${_zlib_location}")

add_subdirectory(common)
add_subdirectory(build-rom)
add_subdirectory(extract-rom)
