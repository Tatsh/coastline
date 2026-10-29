# The tests build the reconstructed C for the host and check it against the branches and words of
# the disassembly. Each test includes or links the sources it covers, with stubs for the rest.
#
# The sources assume 32-bit addresses in places: the display lists hold the addresses of globals as
# words, so the tests are linked as fixed executables that sit below 4 GB. The sources are checked
# for warnings by the build of the ROM; here they are compiled quietly.

set(_src "${CMAKE_SOURCE_DIR}/src")
set(_gu
    "${_src}/libultra/gu/lookatref.c"
    "${_src}/libultra/gu/lookathil.c"
    "${_src}/libultra/gu/mtxcatf.c"
    "${_src}/libultra/gu/mtxutil.c"
    "${_src}/libultra/gu/rotate.c"
    "${_src}/libultra/gu/scale.c"
    "${_src}/libultra/gu/translate.c"
    "${_src}/libultra/gu/normalize.c")
set(_common "${CMAKE_CURRENT_SOURCE_DIR}/stubs/common.c")
set(_vecmath "${_src}/common/vecmath.c")

# Add a test from NAME_test.c and the sources after it.
function(coastline_add_test name)
  add_executable(${name}_test ${name}_test.c ${ARGN})
  target_include_directories(${name}_test PRIVATE "${_src}" "${_src}/n64" "${_src}/libultra/include")
  # The routines a test does not reach are dropped, so they need no stubs.
  target_compile_options(${name}_test PRIVATE -w -fno-pie -fsanitize=address,undefined
                                              -ffunction-sections -fdata-sections)
  target_link_options(${name}_test PRIVATE -no-pie -fsanitize=address,undefined
                      -Wl,--gc-sections)
  target_link_libraries(${name}_test PRIVATE m)
  # A test may be given arguments in NAME_TEST_ARGS.
  add_test(NAME ${name} COMMAND ${name}_test ${${name}_TEST_ARGS})
endfunction()

coastline_add_test(audio)
coastline_add_test(camera ${_vecmath} ${_gu} "${_src}/libultra/gu/lookat.c"
                   "${_src}/libultra/gu/perspective.c" "${_src}/common/camerarender.c")
coastline_add_test(carselect stubs/carselect.c ${_vecmath} ${_gu} "${_src}/common/laptime.c")
coastline_add_test(controllerscreen "${_src}/n64/controllerscreen.c")
coastline_add_test(debugtext "${_src}/n64/debugtext.c" "${_src}/common/debuginstance.c")
coastline_add_test(decaldraw "${_src}/n64/decaldraw.c")
coastline_add_test(decaleditor "${_src}/n64/decaleditor.c" "${_src}/n64/decaldraw.c")
coastline_add_test(drawtexturedrect "${_src}/n64/texrect.c")
coastline_add_test(driving ${_vecmath} ${_common} "${_src}/common/frameclear.c" "${_src}/common/routeposition.c"
                   "${_src}/common/tracetrack.c")
coastline_add_test(
  effects
  ${_vecmath}
  "${_src}/common/billboardparticles.c"
  "${_src}/common/exhaustspawn.c"
  "${_src}/common/particleexhaustrender.c"
  "${_src}/common/particleinit.c"
  "${_src}/common/particlesmokerender.c"
  "${_src}/common/particleupdate.c"
  "${_src}/common/tiresmokespawn.c"
  "${_src}/common/tirespray.c"
  "${_src}/common/tiresprayrender.c"
  "${_src}/common/vehicleeffects.c")
coastline_add_test(fade ${_common} "${_src}/common/fadeupdate.c" "${_src}/common/headlight.c"
                   "${_src}/common/headlightglare.c")
coastline_add_test(
  hud
  "${_src}/common/dashboard.c"
  "${_src}/common/hudmessage.c"
  "${_src}/common/hudoverlay.c"
  "${_src}/common/hudrace.c"
  "${_src}/common/hudtime.c")
coastline_add_test(input "${_src}/n64/input.c" "${_src}/common/controllerread.c" "${_src}/common/inputtranslate.c"
                   "${_src}/common/playerinput.c")
coastline_add_test(
  model
  "${_src}/common/materialcolor.c"
  "${_src}/common/modelanimation.c"
  "${_src}/common/modelanimationflags.c"
  "${_src}/common/modelrelocation.c"
  "${_src}/common/vehiclepaint.c")
coastline_add_test(options "${_src}/common/volumecontrols.c")
coastline_add_test(paintshop "${_src}/n64/paintshop.c")
coastline_add_test(pakmenu "${_src}/n64/pakmenu.c")
coastline_add_test(physics ${_common} "${_src}/common/rigidbody.c" "${_src}/common/cubeclip.c" "${_src}/common/tricache.c")
coastline_add_test(
  race
  stubs/race.c
  ${_common}
  ${_vecmath}
  ${_gu}
  "${_src}/common/racecontrols.c"
  "${_src}/common/racecredits.c"
  "${_src}/common/racefinish.c"
  "${_src}/common/raceinit.c"
  "${_src}/common/raceload.c"
  "${_src}/common/racerender.c"
  "${_src}/common/raceresults.c"
  "${_src}/common/racespecials.c"
  "${_src}/common/seasontrack.c"
  "${_src}/common/startlights.c")
coastline_add_test(racemenu stubs/racemenu.c ${_vecmath} ${_gu} "${_src}/common/modeldraw.c")
coastline_add_test(
  racers
  ${_vecmath}
  ${_common}
  "${_src}/common/raceprogress.c"
  "${_src}/common/racersharing.c"
  "${_src}/common/routewalk.c"
  "${_src}/common/savestate.c")
coastline_add_test(racesound stubs/racesound.c ${_vecmath} ${_common} "${_src}/common/soundpan.c"
                   "${_src}/common/soundsource.c")
coastline_add_test(replay "${_src}/common/rledecompress.c")
coastline_add_test(rigidbody ${_common} "${_src}/common/rigidbody.c")
coastline_add_test(sky ${_common} "${_src}/common/sky.c" "${_src}/common/skystate.c")
coastline_add_test(sort "${_src}/common/util.c")
coastline_add_test(splash ${_common})
coastline_add_test(sprite "${_src}/n64/sprite.c")
coastline_add_test(
  system
  ${_vecmath}
  "${_src}/common/assetdma.c"
  "${_src}/common/displaylistops.c"
  "${_src}/common/fog.c"
  "${_src}/common/framebegin.c"
  "${_src}/common/frameflush.c"
  "${_src}/common/frametime.c"
  "${_src}/common/iomessage.c"
  "${_src}/common/screen.c"
  "${_src}/common/screendraw.c"
  "${_src}/common/texturebind.c"
  "${_src}/common/texturestream.c"
  "${_src}/common/util.c")
# The test counts the calls the other units make to these through its own versions.
target_link_options(
  system_test PRIVATE -Wl,--wrap=Vec2fDistance -Wl,--wrap=Vec3fNormalize
  -Wl,--wrap=Vec3fTransformByMatrix -Wl,--wrap=ResetFrameBlocks)
coastline_add_test(texrect "${_src}/n64/texrect.c")
coastline_add_test(text "${_src}/n64/text.c" "${_src}/common/textrender.c" "${_src}/common/textstyle.c")
coastline_add_test(timing "${_src}/n64/timing.c" "${_src}/common/timingframe.c")
coastline_add_test(track ${_vecmath} ${_common} "${_src}/common/trackgrid.c" "${_src}/common/trackvalid.c")
coastline_add_test(trackrender ${_vecmath} "${_src}/common/screenrect.c" "${_src}/common/shadowreset.c"
                   "${_src}/common/trackinstance.c" "${_src}/common/visiblecompare.c")
coastline_add_test(vecmath ${_vecmath})
coastline_add_test(
  vehicle
  ${_vecmath}
  "${_src}/common/rigidbody.c"
  "${_src}/common/mathutil.c"
  "${_src}/common/trackgrid.c"
  "${_src}/common/tricache.c"
  "${_src}/common/vehicleengine.c"
  "${_src}/common/vehicletrackpos.c")
coastline_add_test(vehiclerender ${_vecmath} ${_gu} "${_src}/common/headlight.c"
                   "${_src}/common/vehiclevisibility.c" "${_src}/common/wheelrender.c")
# The test keeps the light matrices as it sets them through its own version of this routine.
target_link_options(vehiclerender_test PRIVATE -Wl,--wrap=NormalizeLightMatrix)
coastline_add_test(visgrid "${_src}/common/visgrid.c" "${_src}/common/mathutil.c")
coastline_add_test(weather ${_common} "${_src}/common/lightning.c" "${_src}/common/raininit.c")

# The libultra C library, renamed apart from the host's.
coastline_add_test(bcopy stubs/libc.c)
coastline_add_test(llcvt "${_src}/libultra/libc/llcvt.c")
coastline_add_test(
  printf "${_src}/libultra/libc/sprintf.c" "${_src}/libultra/libc/xprintf.c"
  "${_src}/libultra/libc/xlitob.c" "${_src}/libultra/libc/xldtob.c")
set_source_files_properties("${_src}/libultra/libc/sprintf.c" PROPERTIES COMPILE_DEFINITIONS
                                                                         sprintf=lu_sprintf)
coastline_add_test(gu "${_src}/libultra/gu/sinf.c" "${_src}/libultra/gu/cosf.c"
                   "${_src}/libultra/gu/mtxutil.c" "${_src}/libultra/gu/rotate.c"
                   "${_src}/libultra/gu/normalize.c")
set_source_files_properties("${_src}/libultra/gu/sinf.c" PROPERTIES COMPILE_DEFINITIONS sinf=gu_sinf)
set_source_files_properties("${_src}/libultra/gu/cosf.c" PROPERTIES COMPILE_DEFINITIONS cosf=gu_cosf)

# The directories and the configuration file of the SDL game, which need neither SDL nor a display.
enable_language(CXX)
add_executable(sdl_test sdl_test.cpp "${_src}/sdl/config.cpp" "${_src}/sdl/paths.cpp")
set_target_properties(sdl_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(sdl_test PRIVATE "${_src}/sdl")
target_compile_options(sdl_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(sdl_test PRIVATE -fsanitize=address,undefined)
add_test(NAME sdl COMMAND sdl_test)

# The test of zlib inflates the assets extracted from a ROM, so it runs only when they are given.
set(TGR_TEST_ASSETS
    ""
    CACHE PATH "Directory of the assets extracted from a ROM, for the test of zlib. Empty skips it.")
if(TGR_TEST_ASSETS)
  set(_zlib "${CMAKE_SOURCE_DIR}/3rdparty/zlib-1.0.4")
  set(zlib_TEST_ARGS "${TGR_TEST_ASSETS}")
  coastline_add_test(
    zlib "${_zlib}/adler32.c" "${_zlib}/infblock.c" "${_zlib}/infcodes.c" "${_zlib}/inffast.c"
    "${_zlib}/inflate.c" "${_zlib}/inftrees.c" "${_zlib}/infutil.c" "${_zlib}/zutil.c")
  target_include_directories(zlib_test PRIVATE "${_zlib}")
  set_tests_properties(zlib PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
