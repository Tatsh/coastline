# The entry points of the Windows port. A routine that differs substantially between renderers has
# one translation unit per renderer, suffixed with the renderer or under its directory (d3d, glide),
# and the build selects the unit. A routine whose difference is a few lines, DllMain() for one,
# stays in one unit. Each renderer's library exports RallyMain. Nothing refers to DllMain() or
# RallyMain by name, so the link would drop them from an archive, and the objects go into each
# library directly.
add_library(
  coastline_main OBJECT
  airplanestate.c
  assetfile.cpp
  assetnames.c
  boot/app.cpp
  boot/displaymodelist.cpp
  boot/dxversion.cpp
  boot/gamestate.cpp
  boot/rallyconfig.cpp
  boot/shutdown.cpp
  boot/startup.cpp
  boot/strings.c
  buildinfo.c
  clockfields.c
  configfile.cpp
  creditdata.c
  dashboardtextures.c
  directplay.cpp
  displaylist.cpp
  displaylistdata.c
  dllmain.c
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
  reflectiondata.c
  render.cpp
  renderdefaults.cpp
  runtimebuffers.c
  runtimebuffers.cpp
  season.cpp
  settings.c
  snapshot.cpp
  snapshotfields.cpp
  snapshotinterpolation.cpp
  sound.cpp
  soundbank.c
  soundsample.c
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
# The DirectDraw and Direct3D renderer belongs to the Direct3D library alone.
if(TGR_RENDERER STREQUAL "d3d")
  target_sources(
    coastline_main
    PRIVATE d3d/ddutil.cpp
            d3d/devicecaps.cpp
            d3d/driver.cpp
            d3d/renderer.cpp
            d3d/texture.cpp
            gamesession-d3d.cpp
            winmain-d3d.cpp)
endif()
target_link_libraries(coastline_main PRIVATE coastline_common)

# The Glide renderer and what differs from the Direct3D library belong to the Glide library alone.
if(TGR_RENDERER STREQUAL "glide")
  target_sources(coastline_main PRIVATE glide/rgb565image.c glide/texture.c glide/winmain.cpp)

  # The headers of the Glide 2.x SDK for the Voodoo 2, and the import library of glide2x.dll,
  # built from the routines the game imports.
  set(_glide "${CMAKE_SOURCE_DIR}/3rdparty/glide-2f226f0f9225ce8ee83e6a4a7042981e719d19ee")
  set(_glide_def "${CMAKE_CURRENT_SOURCE_DIR}/glide/glide2x.def")
  set(_glide_import "${CMAKE_CURRENT_BINARY_DIR}/libglide2x.a")
  add_custom_command(
    OUTPUT "${_glide_import}"
    COMMAND "${CMAKE_DLLTOOL}" -m i386 -d "${_glide_def}" -l "${_glide_import}"
    DEPENDS "${_glide_def}"
    COMMENT "Building the import library of glide2x.dll")
  add_custom_target(coastline_glide_import DEPENDS "${_glide_import}")
  add_library(coastline_glide INTERFACE)
  target_include_directories(
    coastline_glide SYSTEM INTERFACE "${_glide}/glide2x/cvg/glide/src" "${_glide}/glide2x/cvg/incsrc"
                                     "${_glide}/swlibs/fxmisc")
  target_link_libraries(coastline_glide INTERFACE "${_glide_import}")
  add_dependencies(coastline_glide coastline_glide_import)
  target_link_libraries(coastline_main PRIVATE coastline_glide)
endif()

add_subdirectory(launcher)
