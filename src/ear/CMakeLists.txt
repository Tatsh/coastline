add_library(
  coastline_ear STATIC
  bank.c
  channel.cpp
  configuration.cpp
  direction.c
  directionalmatrix.cpp
  distance.c
  environment.cpp
  error.c
  event.cpp
  imaadpcmstream.cpp
  initialization.cpp
  memorybudget.c
  playback.cpp
  playbacklifetime.cpp
  playbacksmoothing.c
  runtime.c
  scheduledevent.cpp
  shutdown.cpp
  version.c
  volume.c
  wavechunks.c
  wavesample.cpp)
target_include_directories(coastline_ear PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(coastline_ear PRIVATE -Wall -Wextra -Wno-unused-parameter)
set_target_properties(coastline_ear PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
