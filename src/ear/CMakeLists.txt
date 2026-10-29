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
  imaadpcmstream.cpp
  initialization.cpp
  memorybudget.c
  mfc.cpp
  pcmstream.cpp
  playback.cpp
  playbacklifetime.cpp
  playbacksmoothing.c
  registry.cpp
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
set_target_properties(coastline_ear PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES)
if(TGR_PLATFORM STREQUAL "sdl")
  # The SDL game reads the wave files and mixes the buffers with SDL in place of the multimedia
  # I/O and DirectSound of Windows. The types shared with the PC release come from common/types.h,
  # and it includes the types of libultra.
  target_sources(coastline_ear PRIVATE riff.cpp sdlmixer.cpp)
  target_include_directories(coastline_ear PUBLIC "${CMAKE_SOURCE_DIR}/src/libultra/include")
  target_link_libraries(coastline_ear PUBLIC SDL3::SDL3)
  # The calling convention of the exports exists only on 32-bit x86 Windows, as in common/types.h.
  if(NOT (WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4))
    target_compile_definitions(coastline_ear PUBLIC __stdcall=)
  endif()
endif()
