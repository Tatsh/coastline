# The entry points of the Windows port. A routine that differs substantially between renderers has
# one translation unit per renderer, suffixed with the renderer, and the build selects the unit. A
# routine whose difference is a few lines, DllMain() for one, stays in one unit. Each renderer's
# library exports RallyMain. Nothing refers to DllMain() or RallyMain by name, so the link would drop
# them from an archive, and the objects go into each library directly.
add_library(
  coastline_main OBJECT
  boot/rallyconfig.cpp
  boot/startup.cpp
  boot/strings.cpp
  directplay.cpp
  dllmain.c
  fileio.cpp
  winmain-${TGR_RENDERER}.cpp)
# The DirectDraw and Direct3D renderer belongs to the Direct3D library alone.
if(TGR_RENDERER STREQUAL "d3d")
  target_sources(coastline_main PRIVATE d3d/driver.cpp d3d/renderer.cpp d3d/texture.cpp)
endif()
target_link_libraries(coastline_main PRIVATE coastline_common)
