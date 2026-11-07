# The game code every release shares.
set(_common_sources
  ambientsound.c
  assetdma.c
  assetdma-init.c
  billboardparticles.c
  bodyforces.c
  camerachange.c
  camerachase.c
  cameracollision.c
  collisionstep.c
  cubeclip.c
  debuginstance.c
  driving.c
  exhaustspawn.c
  fade.c
  fadeupdate.c
  fog.c
  frameclear.c
  stats.c
  frametime.c
  game.c
  headlight.c
  headlightglare.c
  heap.c
  hudmessage.c
  hudoverlay.c
  hudrace.c
  hudtime.c
  inputtranslate.c
  iomessage.c
  jolt.c
  laptime.c
  lastlapsnapshot.c
  lightning.c
  loadcar.c
  materialcolor.c
  mathutil.c
  modelanimation.c
  modelanimationflags.c
  modeldraw.c
  modelrelocation.c
  particleexhaustrender.c
  particleinit.c
  particlesmokerender.c
  particleupdate.c
  playercontrol.c
  playerinput.c
  precipitationrender.c
  precipitationupdate.c
  profileinit.c
  racecontrols.c
  racecredits.c
  racefinish.c
  raceinit.c
  raceload.c
  racephases.c
  raceprogress.c
  racerender.c
  raceresults.c
  racerinit.c
  racermotion.c
  racers.c
  racersharing.c
  racerstate.c
  racerun.c
  racespecials.c
  raininit.c
  renderinit.c
  replay.c
  rigidbody.c
  rledecompress.c
  routeposition.c
  routewalk.c
  savestate.c
  screen.c
  screen-stub.c
  fill.c
  screenfade.c
  screenrect.c
  seasontrack.c
  shadowreset.c
  sky.c
  skystate.c
  soundeffect.c
  soundpan.c
  soundsource.c
  startlights.c
  textrender.c
  textstyle.c
  texturebind.c
  timingframe.c
  tiresmokespawn.c
  tirespray.c
  tiresprayemit.c
  tiresprayrender.c
  tracetrack.c
  trackgrid.c
  trackrender.c
  trackvalid.c
  tricache.c
  util.c
  vblankquery.c
  vecmath.c
  vecmath-frame.c
  vecmath-light.c
  vehiclechoice.c
  vehicledeform.c
  vehicleeffects.c
  vehicleengine.c
  vehiclemodel.c
  vehiclemodelrender.c
  vehiclepaint.c
  vehiclephysics.c
  vehiclephysics-frame.c
  vehicleposition.c
  vehiclereset.c
  vehiclesound.c
  vehiclestatus.c
  vehicletrackpos.c
  vehiclevisibility.c
  visgrid.c
  visiblecompare.c
  visibleinstances.c
  volumecontrols.c
  volumecontrols-sfx.c
  wheelrender.c
  wind.c)
if(TGR_PLATFORM STREQUAL "sdl" OR (TGR_PLATFORM STREQUAL "n64" AND TGR_ENABLE_PATCHES))
  list(APPEND _common_sources paintshape.c)
endif()
if(TGR_PLATFORM STREQUAL "n64")
  list(REMOVE_ITEM _common_sources
    vehicledeform.c
    sky.c
    vehicletrackpos.c
       modelrelocation.c
       raceprogress.c
       racerun.c
       vehicleengine.c
       vehiclepaint.c
       visibleinstances.c
       vehiclesound.c)
endif()
if(TGR_IDO AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT TGR_ENABLE_PATCHES)
  list(REMOVE_ITEM _common_sources vehiclemodel.c loadcar.c modelanimation.c vehiclestatus.c)
  coastline_add_ido_trimmed_object(coastline_vehiclemodel vehiclemodel.c 0x2c0 0x2b8
                                  vehiclemodel)
  coastline_add_ido_trimmed_object(coastline_loadcar loadcar.c 0xe0 0xd4 loadcar)
  coastline_add_ido_trimmed_object(coastline_modelanimation modelanimation.c 0x3f0 0x3e8
                                  modelanimation)
  coastline_add_ido_trimmed_object(coastline_vehiclestatus vehiclestatus.c 0x2e0 0x2d8
                                  vehiclestatus)
  list(REMOVE_ITEM _common_sources vehiclephysics-frame.c)
  coastline_add_ido_trimmed_object(coastline_vehiclephysics_frame vehiclephysics-frame.c 0x470
                                  0x46c physics-update)
  list(REMOVE_ITEM _common_sources vehicleposition.c)
  coastline_add_ido_trimmed_object(coastline_vehicleposition vehicleposition.c 0x190 0x18c
                                  vehicleposition)
  list(REMOVE_ITEM _common_sources replay.c rledecompress.c)
  coastline_add_ido_trimmed_object(coastline_replay replay.c 0x2e0 0x2d4 replay)
  coastline_add_ido_trimmed_object(coastline_rledecompress rledecompress.c 0x1e0 0x1d8
                                  rledecompress)
  list(REMOVE_ITEM _common_sources assetdma-init.c)
  coastline_add_ido_trimmed_object(coastline_assetdma_init assetdma-init.c 0x10 0xc
                                  assetdma-init)
  list(REMOVE_ITEM _common_sources screen-stub.c)
  coastline_add_ido_trimmed_object(coastline_screen_stub screen-stub.c 0x10 0x8 screen-stub)
  if(NOT TGR_REGION STREQUAL "us")
    list(REMOVE_ITEM _common_sources screen.c)
    coastline_add_ido_trimmed_object(coastline_screen screen.c 0x70 0x68 screen)
  endif()
  list(REMOVE_ITEM _common_sources frametime.c)
  coastline_add_ido_trimmed_object(coastline_frametime frametime.c 0x240 0x23c frametime)
  list(REMOVE_ITEM _common_sources iomessage.c vblankquery.c stats.c)
  coastline_add_ido_trimmed_object(coastline_iomessage iomessage.c 0xf0 0xe8 iomessage)
  coastline_add_ido_trimmed_object(coastline_vblankquery vblankquery.c 0x10 0x10 vblankquery)
  coastline_add_ido_trimmed_object(coastline_stats stats.c 0x60 0x58 stats)
  list(REMOVE_ITEM _common_sources vecmath-light.c texturebind.c)
  coastline_add_ido_trimmed_object(coastline_vecmath_light vecmath-light.c 0x200 0x1f4
                                  vecmath-light)
  coastline_add_ido_trimmed_object(coastline_texturebind texturebind.c 0x680 0x678 texturebind)
  list(REMOVE_ITEM _common_sources fog.c)
  coastline_add_ido_trimmed_object(coastline_fog fog.c 0x11d0 0x11c8 fog)
  if(TGR_REGION STREQUAL "eu" OR TGR_REGION STREQUAL "jp")
    list(REMOVE_ITEM _common_sources renderinit.c)
    coastline_add_ido_trimmed_object(coastline_renderinit renderinit.c 0x320 0x314 renderinit)
  endif()
  list(REMOVE_ITEM _common_sources volumecontrols.c volumecontrols-sfx.c)
  coastline_add_ido_trimmed_object(coastline_volume_bgm volumecontrols.c 0x80 0x74 volume-bgm)
  coastline_add_ido_trimmed_object(coastline_volume_sfx volumecontrols-sfx.c 0x80 0x74 volume-sfx)
  list(REMOVE_ITEM _common_sources laptime.c raceresults.c)
  coastline_add_ido_trimmed_object(coastline_raceresults raceresults.c 0x330 0x328 raceresults)
endif()
add_library(coastline_common STATIC ${_common_sources})
target_link_libraries(coastline_common PRIVATE coastline_options)

if(TGR_PLATFORM STREQUAL "n64")
  # The mixer of the console and the player of its music modules.
  target_sources(coastline_common PRIVATE audio.c)
else()
  # The game code and data of Boss Rally that the Windows libraries and the SDL game share.
  target_sources(
    coastline_common
    PRIVATE camerasystem.cpp
            dashboard.c
            sidegrip.c
            trackinstance.c
            airplanestate.c
            assetfile.cpp
            assetnames.c
            buildinfo.c
            clockfields.c
            camerarender.c
            combine.cpp
            configfile.cpp
            controllerread.c
            convertedvertex.c
            creditdata.c
            damagetexture.cpp
            dashboardtextures.c
            debugmessage.cpp
            displaylistdata.c
            displaylistops.c
            effectdata.c
            emptycallback.c
            emulatedclock.cpp
            fileio.c
            fontdata.c
            fpshud.c
            framebuffer.c
            framebegin.c
            framefields.c
            frameflush-pc.c
            frametimer.cpp
            gameinit.cpp
            ghost.cpp
            hud.c
            hudassets.c
            hudcanvas.c
            hudlaplist.cpp
            hudquad.c
            inputdefaults.cpp
            numericdata.c
            packedvalue.cpp
            paintshop.c
            particlesnapshot.c
            physicsdata.c
            podarchive.cpp
            raceglobals.c
            racepacing.c
            racerslot.cpp
            racesound.cpp
            racescreen.cpp
            rallyconfig.cpp
            reflectiondata.c
            renderbackend.cpp
            runtimebuffers.c
            screendraw.c
            season.cpp
            settings.c
            shadowclip.cpp
            shadowproject.c
            snapshot.cpp
            snapshotfields.cpp
            snapshotinterpolation.cpp
            soundbank.c
            soundbuffer.cpp
            soundsample.c
            strings.c
            terrain.c
            texturesampling.c
            texturestream.c
            textureworkspace.c
            trackanimation.cpp
            trackload.cpp
            trackmenudata.c
            trackskidmarks.cpp
            vehicle.cpp
            vehiclemodelload.c
            vehiclesetup.cpp
            visibilitydata.c)
  # The PC port compiles these sources as C++, as the Windows libraries were built.
  set_source_files_properties(
    ambientsound.c
    billboardparticles.c
    camerachange.c
    camerachase.c
    cameracollision.c
    collisionstep.c
    controllerread.c
    displaylistops.c
    driving.c
    exhaustspawn.c
    fog.c
    game.c
    headlight.c
    hudmessage.c
    hudoverlay.c
    hudrace.c
    inputtranslate.c
    lastlapsnapshot.c
    loadcar.c
    particleexhaustrender.c
    particleinit.c
    particlesmokerender.c
    particleupdate.c
    playercontrol.c
    precipitationrender.c
    precipitationupdate.c
    racecontrols.c
    racefinish.c
    raceinit.c
    raceload.c
    racephases.c
    raceprogress.c
    racerender.c
    raceresults.c
    racerinit.c
    racermotion.c
    racers.c
    racersharing.c
    racerstate.c
    racerun.c
    racespecials.c
    routeposition.c
    seasontrack.c
    sky.c
    tiresmokespawn.c
    tirespray.c
    tiresprayemit.c
    tiresprayrender.c
    tracetrack.c
    trackrender.c
    vehiclechoice.c
    vehicledeform.c
    vehicleeffects.c
    vehicleengine.c
    vehiclemodelrender.c
    vehiclepaint.c
    vehiclephysics.c
    vehiclephysics-frame.c
    vehicleposition.c
    vehiclereset.c
    vehiclesound.c
    vehiclestatus.c
    vehicletrackpos.c
    vehiclevisibility.c
    visibleinstances.c
    volumecontrols.c
    wheelrender.c
    PROPERTIES LANGUAGE CXX)
  set_source_files_properties(dashboard.c
                              trackinstance.c
                              PROPERTIES LANGUAGE CXX)
  if(TGR_ENABLE_PATCHES)
    # The helpers of the quality of life changes, which only the patched libraries have.
    target_sources(coastline_common PRIVATE qualityoflife.c)
  endif()
  if(TGR_PLATFORM STREQUAL "sdl")
    # On a 64-bit processor, the structures read from files and the display lists store their
    # pointers as the 32-bit offsets of common/ptr32.h, and only C++ has them. The SDL game compiles
    # every source as C++, and every source sees the same layout.
    get_target_property(_sources coastline_common SOURCES)
    set_property(TARGET coastline_common PROPERTY SOURCES ${_sources})
    set_source_files_properties(${_sources} PROPERTIES LANGUAGE CXX)
  endif()
  # The game code includes the interpreter of the display lists, which the Glide headers define.
  if(TGR_RENDERER STREQUAL "glide")
    target_link_libraries(coastline_common PRIVATE coastline_glide)
  endif()
endif()

add_subdirectory(libultra)
set(COASTLINE_COMMON_LIBULTRA_LIBRARIES ${COASTLINE_COMMON_LIBULTRA_LIBRARIES} PARENT_SCOPE)
