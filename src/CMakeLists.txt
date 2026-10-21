# Each subsystem directory builds its static library and lists its sources explicitly. A
# recursive glob would hide what is built. A source added to the tree would join the build with no
# record of the decision, and a source removed would disappear the same way.
#
# Static libraries are deliberate. Archiving produces the object files and the archives without
# linking a ROM. The reconstruction is partial, and no link can succeed until every routine a call
# arrives at has a body.

# Everything the subsystem libraries share. An interface library provides the settings without
# compiling anything itself.
add_library(coastline_common INTERFACE)
target_include_directories(coastline_common INTERFACE "${CMAKE_SOURCE_DIR}/src")

# Subsystems join this list as their directories are reconstructed.
set(COASTLINE_SUBSYSTEMS)

foreach(SUBSYSTEM ${COASTLINE_SUBSYSTEMS})
  add_subdirectory(${SUBSYSTEM})
endforeach()

# The entry points sit at the root of the tree rather than in a subsystem. A routine that differs
# substantially between targets has one translation unit per target, suffixed with the target, and
# the build selects the unit. A routine whose difference is a few lines, DllMain() for one, stays
# in one unit. The Windows port is C++ and exports RallyMain from each renderer's library. The
# Nintendo 64 entry is C.
if(TGR_PLATFORM STREQUAL "win")
  enable_language(CXX)
  add_library(coastline_main OBJECT dllmain.c winmain-${TGR_RENDERER}.cpp)
else()
  add_library(coastline_main OBJECT main.c)
endif()
target_link_libraries(coastline_main PRIVATE coastline_common)

# Every archive the build produces, for a target that builds all of them at once.
set(COASTLINE_LIBRARIES coastline_main)
foreach(SUBSYSTEM ${COASTLINE_SUBSYSTEMS})
  list(APPEND COASTLINE_LIBRARIES coastline_${SUBSYSTEM})
endforeach()

add_custom_target(archives DEPENDS ${COASTLINE_LIBRARIES})
