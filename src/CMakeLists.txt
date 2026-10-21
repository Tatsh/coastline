# Each subsystem directory builds its own static library and lists its sources explicitly. A
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

# Every archive the build produces, for a target that builds all of them at once.
set(COASTLINE_LIBRARIES)
foreach(SUBSYSTEM ${COASTLINE_SUBSYSTEMS})
  list(APPEND COASTLINE_LIBRARIES coastline_${SUBSYSTEM})
endforeach()

add_custom_target(archives DEPENDS ${COASTLINE_LIBRARIES})
