add_library(coastline_ear STATIC backend.c distance.c earplayback.cpp environment.cpp error.c
                                exports.def memorybudget.c playbacklifetime.cpp runtime.c
                                scheduledevent.cpp timer.c version.c volume.c window.c)
target_include_directories(coastline_ear PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(coastline_ear PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_ear PRIVATE kernel32 user32 version winmm)
set_target_properties(coastline_ear PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
