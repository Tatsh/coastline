add_library(
  coastline_ear STATIC
  backend.c
  bank.c
  channel.cpp
  configuration.cpp
  direction.c
  directionalmatrix.cpp
  directsoundoutput.cpp
  distance.c
  earplayback.cpp
  environment.cpp
  error.c
  event.cpp
  exports.def
  initialization.cpp
  memorybudget.c
  musicdevice.cpp
  musicplayback.cpp
  pcmstream.cpp
  playbacklifetime.cpp
  playbacksmoothing.c
  runtime.c
  scheduledevent.cpp
  shutdown.cpp
  timer.c
  version.c
  volume.c
  wavechunks.c
  wavesample.cpp
  window.c)
target_include_directories(coastline_ear PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(coastline_ear PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_ear PRIVATE dxguid kernel32 ole32 user32 version winmm)
set_target_properties(coastline_ear PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
