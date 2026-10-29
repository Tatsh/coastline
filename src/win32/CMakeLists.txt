# The Windows port. A routine that differs substantially between renderers has one translation unit
# per renderer, suffixed with the renderer or under its directory (d3d, glide), and the build selects
# the unit. A routine whose difference is a few lines, DllMain() for one, stays in one unit.

# Each renderer's library exports RallyMain. Nothing refers to DllMain() or RallyMain by name, so the
# link would drop them from an archive, and their objects go into each library directly.
add_library(coastline_main OBJECT dllmain.c)
target_link_libraries(coastline_main PRIVATE coastline_options)

add_library(
  coastline_win32 STATIC
  app.cpp
  assetfile.cpp
  buildinfo.c
  configfile.cpp
  diagnostics.c
  directplay.cpp
  displaylist.cpp
  displaymodelist.cpp
  dxversion.cpp
  earsound.cpp
  fileio.c
  fpshud.c
  frametimer.cpp
  frontend.cpp
  frontenddata.cpp
  gameinit.cpp
  gamesession.cpp
  gamestate.cpp
  ghost.cpp
  hudnetwork.cpp
  input.cpp
  inputdefaults.cpp
  menufont.cpp
  modelrelocation.cpp
  netstream.cpp
  numericdata.c
  paintshop.c
  podarchive.cpp
  racepacing.c
  racercolor.cpp
  racesound.cpp
  rallyconfig.cpp
  render.cpp
  renderdefaults.cpp
  runtimebuffers.c
  runtimebuffers.cpp
  season.cpp
  shutdown.cpp
  snapshotfields.cpp
  snapshotinterpolation.cpp
  sound.cpp
  startup.cpp
  strings.c
  texturescan.cpp
  timer.c
  trackload.cpp
  tracktexture.cpp
  vehiclemodelload.c
  vehiclesetup.cpp
  wavread.c)
target_link_libraries(coastline_win32 PRIVATE coastline_options)

if(TGR_RENDERER STREQUAL "d3d")
  # RallyMain of the Direct3D library.
  target_sources(coastline_main PRIVATE winmain-d3d.cpp)
  add_subdirectory(d3d)
else()
  add_subdirectory(glide)
  target_link_libraries(coastline_win32 PRIVATE coastline_glide)
endif()

add_subdirectory(ear)
# The programs that start the game: Boot.exe, BRally.exe, BossRally.exe, and SetVideo.exe.
add_subdirectory(boot)
