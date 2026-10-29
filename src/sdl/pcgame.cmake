# The code of the PC release that the SDL game runs: the shared sources the Windows libraries build,
# and the game logic of src/win32. Each source compiles in the language the Windows build gives it.
# The DirectX, DirectPlay, and Win32 backends and the DirectDraw menus are left out; the SDL game
# provides what they did.
set(_src "${CMAKE_SOURCE_DIR}/src")
set(COASTLINE_PC_C_SOURCES
    common/assetdma.c common/bodyforces.c common/camerarender.c common/completionthreads.c common/cubeclip.c common/debuginstance.c
    common/emptycallback.c common/fade.c common/fadeupdate.c common/framebegin.c common/frameclear.c common/frameflush.c common/frametime.c common/game.c
    common/headlightglare.c common/heap.c common/hudtime.c common/iomessage.c common/laptime.c common/libultra/gu/lookat.c
    common/libultra/gu/lookathil.c common/libultra/gu/lookatref.c common/libultra/gu/mtxcatf.c common/libultra/gu/mtxutil.c
    common/libultra/gu/perspective.c common/libultra/gu/rotate.c common/libultra/gu/scale.c common/libultra/gu/translate.c
    common/libultra/io/contreaddata.c common/libultra/io/controller.c libultra/os/getcount.c
    common/libultra/os/recvmesg.c common/libultra/os/sendmesg.c common/libultra/os/syncprintf.c common/lightning.c
    common/materialcolor.c common/mathutil.c common/modelanimation.c common/modelanimationflags.c common/modeldraw.c
    common/modelrelocation.c common/playerinput.c common/profileinit.c common/racecredits.c common/raininit.c common/renderinit.c common/replay.c
    common/rigidbody.c common/rledecompress.c common/routewalk.c common/savestate.c common/screen.c common/screendraw.c common/screenfade.c
    common/screenrect.c common/shadowreset.c common/sidegrip.c common/skystate.c common/soundeffect.c common/soundpan.c common/soundsource.c
    common/startlights.c common/textrender.c common/textstyle.c common/texturebind.c common/texturestream.c common/timingframe.c
    common/trackgrid.c common/trackvalid.c common/tricache.c common/util.c common/vblankquery.c common/vecmath.c common/vehiclemodel.c common/visgrid.c
    common/visiblecompare.c common/airplanestate.c common/assetnames.c win32/strings.c
    win32/buildinfo.c common/clockfields.c common/convertedvertex.c common/creditdata.c
    common/dashboardtextures.c common/displaylistdata.c common/effectdata.c
    common/fontdata.c win32/fpshud.c common/framebuffer.c common/framefields.c common/hudassets.c
    common/hud.c common/hudcanvas.c common/hudquad.c win32/numericdata.c win32/paintshop.c
    common/particlesnapshot.c common/physicsdata.c common/raceglobals.c win32/racepacing.c
    win32/reflectiondata.c win32/runtimebuffers.c common/settings.c common/shadowproject.c
    common/soundbank.c common/soundsample.c common/terrain.c common/texturesampling.c
    common/textureworkspace.c win32/timer.c common/trackmenudata.c win32/vehiclemodelload.c
    common/visibilitydata.c common/wind.c)
set(COASTLINE_PC_CXX_SOURCES
    common/ambientsound.c common/billboardparticles.c common/camerachange.c common/camerachase.c common/cameracollision.c
    common/collisionstep.c common/controllerread.c common/dashboard.c common/displaylistops.c common/driving.c common/exhaustspawn.c common/fog.c
    common/headlight.c common/hudmessage.c common/hudoverlay.c common/hudrace.c common/inputtranslate.c common/lastlapsnapshot.c common/loadcar.c
    common/particleexhaustrender.c common/particleinit.c common/particlesmokerender.c common/particleupdate.c common/playercontrol.c
    common/precipitationrender.c common/precipitationupdate.c common/racecontrols.c common/racefinish.c common/raceinit.c common/raceload.c
    common/racephases.c common/raceprogress.c common/racerender.c common/raceresults.c common/racerinit.c common/racermotion.c common/racers.c
    common/racersharing.c common/racerstate.c common/racerun.c common/racespecials.c common/routeposition.c common/seasontrack.c common/sky.c
    common/tiresmokespawn.c common/tirespray.c common/tiresprayemit.c common/tiresprayrender.c common/tracetrack.c common/trackinstance.c
    common/trackrender.c common/vehiclechoice.c common/vehicledeform.c common/vehicleeffects.c common/vehicleengine.c
    common/vehiclemodelrender.c common/vehiclepaint.c common/vehiclephysics.c common/vehiclereset.c common/vehiclesound.c
    common/vehiclestatus.c common/vehicletrackpos.c common/vehiclevisibility.c common/visibleinstances.c common/volumecontrols.c
    common/wheelrender.c win32/assetfile.cpp win32/gamestate.cpp win32/rallyconfig.cpp
    win32/configfile.cpp win32/displaylist.cpp win32/frametimer.cpp win32/gameinit.cpp
    win32/gamesession.cpp win32/ghost.cpp win32/hudlaplist.cpp win32/hudnetwork.cpp
    win32/inputdefaults.cpp win32/modelrelocation.cpp win32/podarchive.cpp win32/racercolor.cpp
    common/racerslot.cpp win32/racescreen.cpp win32/racesound.cpp win32/render.cpp
    win32/renderdefaults.cpp win32/runtimebuffers.cpp win32/season.cpp win32/shadowclip.cpp
    common/snapshot.cpp win32/snapshotfields.cpp win32/snapshotinterpolation.cpp
    win32/texturescan.cpp win32/trackanimation.cpp win32/trackload.cpp win32/trackskidmarks.cpp
    win32/tracktexture.cpp common/vehicle.cpp win32/vehiclesetup.cpp)
list(TRANSFORM COASTLINE_PC_C_SOURCES PREPEND "${_src}/")
list(TRANSFORM COASTLINE_PC_CXX_SOURCES PREPEND "${_src}/")
# The SDL game compiles every source as C++, for the 32-bit offsets of Ptr32 in the structures
# read from files.
set_source_files_properties(${COASTLINE_PC_C_SOURCES} ${COASTLINE_PC_CXX_SOURCES}
                            PROPERTIES LANGUAGE CXX)
# These sources read no structure with a Ptr32 and convert from void * as C does.
set_source_files_properties("${_src}/common/savestate.c" "${_src}/win32/paintshop.c" PROPERTIES LANGUAGE C)

# The SDL game provides these in place of their Windows units.
set(COASTLINE_PC_SDL_SOURCES bitmap.cpp d3dcompat.cpp dinputcompat.cpp fileio.cpp frontendstate.cpp
                             ghostrecording.cpp
                             dsoundcompat.cpp mmsystemcompat.cpp n64displaylist.cpp network.cpp
                             pcgamerun.cpp
                             renderhost.cpp stringtable.cpp)
# The routines of the textures of BRD3D.dll that do not use DirectDraw, and the conversions of the
# states of vehicles the network streams.
list(APPEND COASTLINE_PC_CXX_SOURCES "${_src}/win32/d3d/texture.cpp" "${_src}/win32/netstream.cpp"
     "${_src}/win32/app.cpp" "${_src}/win32/startup.cpp" "${_src}/win32/input.cpp"
     "${_src}/win32/earsound.cpp" "${_src}/win32/sound.cpp" "${_src}/win32/wavread.c")

add_library(coastline_pcgame STATIC ${COASTLINE_PC_C_SOURCES} ${COASTLINE_PC_CXX_SOURCES}
                                    ${COASTLINE_PC_SDL_SOURCES})
target_include_directories(coastline_pcgame PUBLIC "${_src}" "${_src}/libultra/include")
target_link_libraries(coastline_pcgame PUBLIC SDL3::SDL3 PRIVATE FLAC::FLAC
                                                                 nlohmann_json::nlohmann_json)
# A cast between a pointer and a 32-bit integer loses half the pointer on a 64-bit processor, and
# is an error so that every one is converted to Ptr32.
# The data of a source is placed in the order of its definitions, as in the Windows libraries. Some
# loads of the original write past an array into the one defined after it, such as the TMEM shadow
# into the shadow of its palettes.
target_compile_options(
  coastline_pcgame PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-x;c++>" -Wno-unused-parameter
                           -fno-toplevel-reorder -Werror=int-to-pointer-cast
                           "$<$<COMPILE_LANGUAGE:C>:-Werror=pointer-to-int-cast>")
