# Builds the Boss Rally renderer libraries in separate trees, and reuses renderer-independent
# archives from the Direct3D build when linking the Glide library.

include(ExternalProject)

if(NOT TGR_VARIANTS STREQUAL "boss-rally")
  message(FATAL_ERROR "TGR_VARIANTS only accepts boss-rally. Use a regional preset for N64.")
endif()

foreach(_variant win-d3d win-glide)
  string(REGEX MATCH "^([a-z0-9]+)-([a-z0-9]+)$" _ "${_variant}")
  set(_platform "${CMAKE_MATCH_1}")
  set(_choice "${CMAKE_MATCH_2}")
  set(_choice_arg "-DTGR_RENDERER=${_choice}")
  if(_choice STREQUAL "glide")
    list(APPEND _choice_arg "-DTGR_SHARED_WIN_BUILD=${CMAKE_BINARY_DIR}/win-d3d")
  endif()
  list(APPEND _choice_arg -DTGR_INTERNAL_WIN_VARIANT=ON)
  set(_toolchain_arg "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/${_platform}-toolchain.cmake")
  if(MSVC AND _platform STREQUAL "win")
    set(_toolchain_arg "")
    list(APPEND _choice_arg "-DDXSDK_DIR=${DXSDK_DIR}")
  endif()
  ExternalProject_Add(
    ${_variant}
    SOURCE_DIR "${CMAKE_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/${_variant}"
    CMAKE_ARGS -DTGR_PLATFORM=${_platform} ${_choice_arg}
               -DTGR_ENABLE_PATCHES=${TGR_ENABLE_PATCHES}
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
               ${_toolchain_arg}
    BUILD_ALWAYS ON
    INSTALL_COMMAND ""
    TEST_COMMAND "")
endforeach()
add_dependencies(win-glide win-d3d)

find_program(TGR_MAKENSIS makensis HINTS "C:/Program Files (x86)/NSIS" REQUIRED)
set(_boss_rally_installer "${CMAKE_BINARY_DIR}/BossRally-Setup.exe")
add_custom_command(
  OUTPUT "${_boss_rally_installer}"
  COMMAND
    "${TGR_MAKENSIS}"
    "-DOUTPUT=${_boss_rally_installer}"
    "-DBUILD=${CMAKE_BINARY_DIR}/win-d3d"
    "-DGLIDE_BUILD=${CMAKE_BINARY_DIR}/win-glide"
    "${CMAKE_SOURCE_DIR}/cmake/bossrally.nsi"
  DEPENDS win-glide "${CMAKE_SOURCE_DIR}/cmake/bossrally.nsi"
  VERBATIM)
add_custom_target(boss-rally-installer ALL DEPENDS "${_boss_rally_installer}")
