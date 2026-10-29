# The Windows port. A routine that differs substantially between renderers has one translation unit
# per renderer, suffixed with the renderer or under its directory (d3d, glide), and the build selects
# the unit. A routine whose difference is a few lines, DllMain() for one, stays in one unit.

# Each renderer's library exports RallyMain. Nothing refers to DllMain() or RallyMain by name, so the
# link would drop them from an archive, and their objects go into each library directly.
add_library(coastline_main OBJECT dllmain.c)
target_link_libraries(coastline_main PRIVATE coastline_options)

add_library(
  coastline_win32 STATIC
  airplanestate.c
  app.cpp
  assetfile.cpp
  assetnames.c
  buildinfo.c
  clockfields.c
  configfile.cpp
  convertedvertex.c
  creditdata.c
  dashboardtextures.c
  diagnostics.c
  directplay.cpp
  displaylist.cpp
  displaylistdata.c
  displaymodelist.cpp
  dxversion.cpp
  earsound.cpp
  effectdata.c
  fileio.c
  fontdata.c
  fpshud.c
  framebuffer.c
  framefields.c
  frametimer.cpp
  frontend.cpp
  frontenddata.cpp
  gameinit.cpp
  gamesession.cpp
  gamestate.cpp
  ghost.cpp
  hud.c
  hudassets.c
  hudcanvas.c
  hudlaplist.cpp
  hudnetwork.cpp
  hudquad.c
  input.cpp
  inputdefaults.cpp
  menufont.cpp
  modelrelocation.cpp
  netstream.cpp
  numericdata.c
  paintshop.c
  particlesnapshot.c
  physicsdata.c
  podarchive.cpp
  raceglobals.c
  racepacing.c
  racercolor.cpp
  racerslot.cpp
  racescreen.cpp
  racesound.cpp
  rallyconfig.cpp
  reflectiondata.c
  render.cpp
  renderdefaults.cpp
  runtimebuffers.c
  runtimebuffers.cpp
  season.cpp
  settings.c
  shadowclip.cpp
  shadowproject.c
  shutdown.cpp
  snapshot.cpp
  snapshotfields.cpp
  snapshotinterpolation.cpp
  sound.cpp
  soundbank.c
  soundsample.c
  startup.cpp
  strings.c
  terrain.c
  texturesampling.c
  texturescan.cpp
  textureworkspace.c
  timer.c
  trackanimation.cpp
  trackload.cpp
  trackmenudata.c
  trackskidmarks.cpp
  tracktexture.cpp
  vehicle.cpp
  vehiclemodelload.c
  vehiclesetup.cpp
  visibilitydata.c
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
