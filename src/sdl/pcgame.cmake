# The code of the PC release that the SDL game runs: the shared sources the Windows libraries build,
# and the game logic of src/win32. Each source compiles in the language the Windows build gives it.
# The DirectX, DirectPlay, and Win32 backends and the DirectDraw menus are left out; the SDL game
# provides what they did.
set(_src "${CMAKE_SOURCE_DIR}/src")
set(COASTLINE_PC_C_SOURCES
    assetdma.c bodyforces.c camerarender.c completionthreads.c cubeclip.c debuginstance.c
    emptycallback.c fade.c fadeupdate.c framebegin.c frameclear.c frameflush.c frametime.c game.c
    headlightglare.c heap.c hudtime.c iomessage.c laptime.c libultra/gu/lookat.c
    libultra/gu/lookathil.c libultra/gu/lookatref.c libultra/gu/mtxcatf.c libultra/gu/mtxutil.c
    libultra/gu/perspective.c libultra/gu/rotate.c libultra/gu/scale.c libultra/gu/translate.c
    libultra/io/contreaddata.c libultra/io/controller.c libultra/os/getcount.c
    libultra/os/recvmesg.c libultra/os/sendmesg.c libultra/os/syncprintf.c lightning.c
    materialcolor.c mathutil.c modelanimation.c modelanimationflags.c modeldraw.c
    modelrelocation.c playerinput.c profileinit.c racecredits.c raininit.c renderinit.c replay.c
    rigidbody.c rledecompress.c routewalk.c savestate.c screen.c screendraw.c screenfade.c
    screenrect.c shadowreset.c sidegrip.c skystate.c soundeffect.c soundpan.c soundsource.c
    startlights.c textrender.c textstyle.c texturebind.c texturestream.c timingframe.c
    trackgrid.c trackvalid.c tricache.c util.c vblankquery.c vecmath.c vehiclemodel.c visgrid.c
    visiblecompare.c win32/airplanestate.c win32/assetnames.c win32/boot/strings.c
    win32/buildinfo.c win32/clockfields.c win32/convertedvertex.c win32/creditdata.c
    win32/dashboardtextures.c win32/displaylistdata.c win32/effectdata.c
    win32/fontdata.c win32/fpshud.c win32/framebuffer.c win32/framefields.c win32/hudassets.c
    win32/hud.c win32/hudcanvas.c win32/hudquad.c win32/numericdata.c win32/paintshop.c
    win32/particlesnapshot.c win32/physicsdata.c win32/raceglobals.c win32/racepacing.c
    win32/reflectiondata.c win32/runtimebuffers.c win32/settings.c win32/shadowproject.c
    win32/soundbank.c win32/soundsample.c win32/terrain.c win32/texturesampling.c
    win32/textureworkspace.c win32/timer.c win32/trackmenudata.c win32/vehiclemodelload.c
    win32/visibilitydata.c wind.c)
set(COASTLINE_PC_CXX_SOURCES
    ambientsound.c billboardparticles.c camerachange.c camerachase.c cameracollision.c
    collisionstep.c controllerread.c dashboard.c displaylistops.c driving.c exhaustspawn.c fog.c
    headlight.c hudmessage.c hudoverlay.c hudrace.c inputtranslate.c lastlapsnapshot.c loadcar.c
    particleexhaustrender.c particleinit.c particlesmokerender.c particleupdate.c playercontrol.c
    precipitationrender.c precipitationupdate.c racecontrols.c racefinish.c raceinit.c raceload.c
    racephases.c raceprogress.c racerender.c raceresults.c racerinit.c racermotion.c racers.c
    racersharing.c racerstate.c racerun.c racespecials.c routeposition.c seasontrack.c sky.c
    tiresmokespawn.c tirespray.c tiresprayemit.c tiresprayrender.c tracetrack.c trackinstance.c
    trackrender.c vehiclechoice.c vehicledeform.c vehicleeffects.c vehicleengine.c
    vehiclemodelrender.c vehiclepaint.c vehiclephysics.c vehiclereset.c vehiclesound.c
    vehiclestatus.c vehicletrackpos.c vehiclevisibility.c visibleinstances.c volumecontrols.c
    wheelrender.c win32/assetfile.cpp win32/boot/gamestate.cpp win32/boot/rallyconfig.cpp
    win32/configfile.cpp win32/displaylist.cpp win32/frametimer.cpp win32/gameinit.cpp
    win32/gamesession.cpp win32/ghost.cpp win32/hudlaplist.cpp win32/hudnetwork.cpp
    win32/inputdefaults.cpp win32/modelrelocation.cpp win32/podarchive.cpp win32/racercolor.cpp
    win32/racerslot.cpp win32/racescreen.cpp win32/racesound.cpp win32/render.cpp
    win32/renderdefaults.cpp win32/runtimebuffers.cpp win32/season.cpp win32/shadowclip.cpp
    win32/snapshot.cpp win32/snapshotfields.cpp win32/snapshotinterpolation.cpp
    win32/texturescan.cpp win32/trackanimation.cpp win32/trackload.cpp win32/trackskidmarks.cpp
    win32/tracktexture.cpp win32/vehicle.cpp win32/vehiclesetup.cpp)
list(TRANSFORM COASTLINE_PC_C_SOURCES PREPEND "${_src}/")
list(TRANSFORM COASTLINE_PC_CXX_SOURCES PREPEND "${_src}/")
# The SDL game compiles every source as C++, for the 32-bit offsets of Ptr32 in the structures
# read from files.
set_source_files_properties(${COASTLINE_PC_C_SOURCES} ${COASTLINE_PC_CXX_SOURCES}
                            PROPERTIES LANGUAGE CXX)
# These sources read no structure with a Ptr32 and convert from void * as C does.
set_source_files_properties("${_src}/savestate.c" "${_src}/win32/paintshop.c" PROPERTIES LANGUAGE C)

# The SDL game provides these in place of their Windows units.
set(COASTLINE_PC_SDL_SOURCES bitmap.cpp d3dcompat.cpp dinputcompat.cpp fileio.cpp frontendstate.cpp
                             dsoundcompat.cpp mmsystemcompat.cpp n64displaylist.cpp network.cpp
                             pcgamerun.cpp
                             renderhost.cpp stringtable.cpp)
# The routines of the textures of BRD3D.dll that do not use DirectDraw, and the conversions of the
# states of vehicles the network streams.
list(APPEND COASTLINE_PC_CXX_SOURCES "${_src}/win32/d3d/texture.cpp" "${_src}/win32/netstream.cpp"
     "${_src}/win32/boot/app.cpp" "${_src}/win32/boot/startup.cpp" "${_src}/win32/input.cpp"
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
