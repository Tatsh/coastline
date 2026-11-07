# The Glide renderer and what differs from the Direct3D library.

# The headers of the Glide 2.x SDK for the Voodoo 2, and the import library of glide2x.dll, built
# from the routines the game imports.
set(_glide "${CMAKE_SOURCE_DIR}/3rdparty/glide-2f226f0f9225ce8ee83e6a4a7042981e719d19ee")
set(_glide_def "${CMAKE_CURRENT_SOURCE_DIR}/glide2x.def")
set(_glide_import "${CMAKE_CURRENT_BINARY_DIR}/libglide2x.a")
add_custom_command(
  OUTPUT "${_glide_import}"
  COMMAND "${CMAKE_DLLTOOL}" -m i386 -d "${_glide_def}" -l "${_glide_import}"
  DEPENDS "${_glide_def}"
  COMMENT "Building the import library of glide2x.dll")
add_custom_target(coastline_glide_import DEPENDS "${_glide_import}")
add_library(coastline_glide INTERFACE)
target_include_directories(
  coastline_glide SYSTEM INTERFACE "${_glide}/glide2x/cvg/glide/src" "${_glide}/glide2x/cvg/incsrc"
                                   "${_glide}/swlibs/fxmisc")
target_link_libraries(coastline_glide INTERFACE "${_glide_import}")
add_dependencies(coastline_glide coastline_glide_import)

add_library(
  coastline_win32_glide STATIC
  displaylist.cpp
  displaymode.cpp
  fontatlas.c
  menusurface.cpp
  renderer.cpp
  rgb565image.c
  splash.cpp
  texture.c
  texturecache.cpp)
target_link_libraries(coastline_win32_glide PRIVATE coastline_options coastline_glide)

# RallyMain of the Glide library.
target_sources(coastline_main PRIVATE winmain.cpp)
target_link_libraries(coastline_main PRIVATE coastline_glide)
