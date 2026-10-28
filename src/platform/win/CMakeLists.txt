# The entry points of the Windows port. A routine that differs substantially between renderers has
# one translation unit per renderer, suffixed with the renderer, and the build selects the unit. A
# routine whose difference is a few lines, DllMain() for one, stays in one unit. Each renderer's
# library exports RallyMain. Nothing refers to DllMain() or RallyMain by name, so the link would drop
# them from an archive, and the objects go into each library directly.
add_library(
  coastline_main OBJECT
  assetfile.cpp
  boot/app.cpp
  boot/displaymodelist.cpp
  boot/dxversion.cpp
  boot/gamestate.cpp
  boot/rallyconfig.cpp
  boot/startup.cpp
  boot/strings.c
  configfile.cpp
  dashboardtextures.c
  directplay.cpp
  displaylist.cpp
  dllmain.c
  earsound.cpp
  fileio.c
  frametimer.cpp
  framebuffer.c
  fpshud.c
  frontend.cpp
  gameinit.cpp
  ghost.cpp
  hudlaplist.cpp
  hudnetwork.cpp
  hudquad.c
  hud.c
  input.cpp
  menufont.cpp
  modelrelocation.cpp
  netstream.cpp
  paintshop.c
  particlesnapshot.c
  podarchive.cpp
  racercolor.cpp
  racepacing.c
  racerslot.cpp
  racesound.cpp
  racescreen.cpp
  render.cpp
  season.cpp
  snapshot.cpp
  snapshotinterpolation.cpp
  sound.cpp
  terrain.c
  texturesampling.c
  texturescan.cpp
  trackanimation.cpp
  trackload.cpp
  trackskidmarks.cpp
  tracktexture.cpp
  timer.c
  vehicle.cpp
  vehiclemodelload.c
  vehiclesetup.cpp
  wavread.c
  winmain-${TGR_RENDERER}.cpp)
# The DirectDraw and Direct3D renderer belongs to the Direct3D library alone.
if(TGR_RENDERER STREQUAL "d3d")
  target_sources(
    coastline_main
    PRIVATE assetnames.c
            airplanestate.c
            clockfields.c
            framefields.c
            snapshotfields.cpp
            soundsample.c
            soundbank.c
            trackmenudata.c
            creditdata.c
            displaylistdata.c
            effectdata.c
            boot/shutdown.cpp
            frontenddata.cpp
            fontdata.c
            hudassets.c
            inputdefaults.cpp
            numericdata.c
            physicsdata.c
            reflectiondata.c
            renderdefaults.cpp
            runtimebuffers.c
            runtimebuffers.cpp
            settings.c
            d3d/ddutil.cpp
            d3d/devicecaps.cpp
            d3d/driver.cpp
            d3d/renderer.cpp
            d3d/texture.cpp
            gamesession-d3d.cpp)
endif()
target_link_libraries(coastline_main PRIVATE coastline_common)

if(TGR_RENDERER STREQUAL "glide")
  target_sources(coastline_main PRIVATE rgb565image.c)
endif()

add_subdirectory(launcher)
