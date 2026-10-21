# The entry points of the Windows port. A routine that differs substantially between renderers has
# one translation unit per renderer, suffixed with the renderer, and the build selects the unit. A
# routine whose difference is a few lines, DllMain() for one, stays in one unit. Each renderer's
# library exports RallyMain. Nothing refers to DllMain() or RallyMain by name, so the link would drop
# them from an archive, and the objects go into each library directly.
add_library(coastline_main OBJECT dllmain.c winmain-${TGR_RENDERER}.cpp)
target_link_libraries(coastline_main PRIVATE coastline_common)
