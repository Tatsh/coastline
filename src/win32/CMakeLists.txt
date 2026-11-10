# The Windows port. A routine that differs substantially between renderers has one translation unit
# per renderer, suffixed with the renderer or under its directory (d3d, glide), and the build selects
# the unit. A routine whose difference is a few lines, DllMain() for one, stays in one unit.

# Each renderer's library exports RallyMain. Nothing refers to DllMain() or RallyMain by name, so the
# link would drop them from an archive, and their objects go into each library directly.
function(
  coastline_add_win_version target description company product internal_name filename copyright)
  set(COASTLINE_WIN_DESCRIPTION "${description}")
  set(COASTLINE_WIN_COMPANY "${company}")
  set(COASTLINE_WIN_PRODUCT "${product}")
  set(COASTLINE_WIN_INTERNAL_NAME "${internal_name}")
  set(COASTLINE_WIN_FILENAME "${filename}")
  set(COASTLINE_WIN_COPYRIGHT "${copyright}")
  set(COASTLINE_WIN_VERSION "${COASTLINE_METADATA_VERSION}")
  get_target_property(_target_type ${target} TYPE)
  if(_target_type STREQUAL "SHARED_LIBRARY")
    set(COASTLINE_WIN_FILE_TYPE VFT_DLL)
  else()
    set(COASTLINE_WIN_FILE_TYPE VFT_APP)
  endif()
  set(_resource "${CMAKE_CURRENT_BINARY_DIR}/${target}-version.rc")
  configure_file("${CMAKE_SOURCE_DIR}/src/win32/version.rc.in" "${_resource}" @ONLY)
  target_sources(${target} PRIVATE "${_resource}")
endfunction()

add_library(coastline_main OBJECT dllmain.c)
target_link_libraries(coastline_main PRIVATE coastline_options)

add_library(
  coastline_win32 STATIC
  app.cpp
  diagnostics.c
  directplay.cpp
  displaylist.cpp
  dxversion.cpp
  earsound.cpp
  frontend.cpp
  frontenddata.cpp
  frametimer.cpp
  gamesession.cpp
  gamestate.cpp
  hudnetwork.cpp
  input.cpp
  menufont.cpp
  modelrelocation.cpp
  netstream.cpp
  racercolor.cpp
  render.cpp
  renderdefaults.cpp
  runtimebuffers.cpp
  shutdown.cpp
  sound.cpp
  soundinput.cpp
  soundoutput.cpp
  startup.cpp
  texturescan.cpp
  timer.c
  tracktexture.cpp
  wavread.c)
target_link_libraries(coastline_win32 PRIVATE coastline_options)

if(TGR_RENDERER STREQUAL "d3d")
  # RallyMain of the Direct3D library.
  target_sources(coastline_main PRIVATE winmain-d3d.cpp)
  target_sources(coastline_win32 PRIVATE displaymodelist.cpp)
  add_subdirectory(d3d)
else()
  add_subdirectory(glide)
  target_link_libraries(coastline_win32 PRIVATE coastline_glide)
endif()

if(NOT TGR_SHARED_WIN_BUILD)
  add_subdirectory(ear)
endif()
# The programs that start the game: Boot.exe, BRally.exe, BossRally.exe, and SetVideo.exe.
if(NOT TGR_SHARED_WIN_BUILD)
  add_subdirectory(boot)
endif()
