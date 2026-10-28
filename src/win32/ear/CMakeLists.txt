# The DirectSound output, the MCI music devices, the timer, and the window of EAR. The portable
# core in src/ear calls these routines and they call back into the core.
add_library(
  coastline_win32_ear STATIC
  backend.c
  directsoundoutput.cpp
  earplayback.cpp
  exports.def
  musicdevice.cpp
  musicplayback.cpp
  pcmstream.cpp
  timer.c
  window.c)
target_include_directories(coastline_win32_ear PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(coastline_win32_ear PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_win32_ear PRIVATE dxguid kernel32 ole32 user32 version winmm)
set_target_properties(coastline_win32_ear PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
target_link_libraries(coastline_ear PUBLIC coastline_win32_ear)
target_link_libraries(coastline_win32_ear PUBLIC coastline_ear)
