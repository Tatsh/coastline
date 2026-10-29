# The MCI music devices of EAR and the file of MFC they write with. The portable core in src/ear
# calls these routines and they call back into the core.
add_library(coastline_win32_ear STATIC exports.def file.cpp musicdevice.cpp musicplayback.cpp)
target_include_directories(coastline_win32_ear PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(coastline_win32_ear PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_win32_ear PRIVATE dxguid kernel32 ole32 user32 version winmm)
set_target_properties(coastline_win32_ear PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
target_link_libraries(coastline_ear PUBLIC coastline_win32_ear)
target_link_libraries(coastline_win32_ear PUBLIC coastline_ear)

# earpds.dll, the audio engine the game loads beside its renderer library, with the exports the
# game imports by their decorated names.
add_library(earpds SHARED exports.def)
set_target_properties(earpds PROPERTIES PREFIX "" LINKER_LANGUAGE CXX)
target_link_libraries(
  earpds PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,coastline_ear>"
                 "$<LINK_LIBRARY:WHOLE_ARCHIVE,coastline_win32_ear>" dxguid kernel32 ole32 user32
                 version winmm)
