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
set(_vecmath "${_src}/vecmath.c")

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
                   "${_src}/libultra/gu/perspective.c" "${_src}/camerarender.c")
coastline_add_test(carselect stubs/carselect.c ${_vecmath} ${_gu} "${_src}/laptime.c")
coastline_add_test(controllerscreen "${_src}/n64/controllerscreen.c")
coastline_add_test(debugtext "${_src}/n64/debugtext.c" "${_src}/debuginstance.c")
coastline_add_test(decaldraw "${_src}/n64/decaldraw.c")
coastline_add_test(decaleditor "${_src}/n64/decaleditor.c" "${_src}/n64/decaldraw.c")
coastline_add_test(drawtexturedrect "${_src}/n64/texrect.c")
coastline_add_test(driving ${_vecmath} ${_common} "${_src}/frameclear.c" "${_src}/routeposition.c"
                   "${_src}/tracetrack.c")
coastline_add_test(
  effects
  ${_vecmath}
  "${_src}/billboardparticles.c"
  "${_src}/exhaustspawn.c"
  "${_src}/particleexhaustrender.c"
  "${_src}/particleinit.c"
  "${_src}/particlesmokerender.c"
  "${_src}/particleupdate.c"
  "${_src}/tiresmokespawn.c"
  "${_src}/tirespray.c"
  "${_src}/tiresprayrender.c"
  "${_src}/vehicleeffects.c")
coastline_add_test(fade ${_common} "${_src}/fadeupdate.c" "${_src}/headlight.c"
                   "${_src}/headlightglare.c")
coastline_add_test(
  hud
  "${_src}/dashboard.c"
  "${_src}/hudmessage.c"
  "${_src}/hudoverlay.c"
  "${_src}/hudrace.c"
  "${_src}/hudtime.c")
coastline_add_test(input "${_src}/n64/input.c" "${_src}/controllerread.c" "${_src}/inputtranslate.c"
                   "${_src}/playerinput.c")
coastline_add_test(
  model
  "${_src}/materialcolor.c"
  "${_src}/modelanimation.c"
  "${_src}/modelanimationflags.c"
  "${_src}/modelrelocation.c"
  "${_src}/vehiclepaint.c")
coastline_add_test(options "${_src}/volumecontrols.c")
coastline_add_test(paintshop "${_src}/n64/paintshop.c")
coastline_add_test(pakmenu "${_src}/n64/pakmenu.c")
coastline_add_test(physics ${_common} "${_src}/rigidbody.c" "${_src}/cubeclip.c" "${_src}/tricache.c")
coastline_add_test(
  race
  stubs/race.c
  ${_common}
  ${_vecmath}
  ${_gu}
  "${_src}/racecontrols.c"
  "${_src}/racecredits.c"
  "${_src}/racefinish.c"
  "${_src}/raceinit.c"
  "${_src}/raceload.c"
  "${_src}/racerender.c"
  "${_src}/raceresults.c"
  "${_src}/racespecials.c"
  "${_src}/seasontrack.c"
  "${_src}/startlights.c")
coastline_add_test(racemenu stubs/racemenu.c ${_vecmath} ${_gu} "${_src}/modeldraw.c")
coastline_add_test(
  racers
  ${_vecmath}
  ${_common}
  "${_src}/raceprogress.c"
  "${_src}/racersharing.c"
  "${_src}/routewalk.c"
  "${_src}/savestate.c")
coastline_add_test(racesound stubs/racesound.c ${_vecmath} ${_common} "${_src}/soundpan.c"
                   "${_src}/soundsource.c")
coastline_add_test(replay "${_src}/rledecompress.c")
coastline_add_test(rigidbody ${_common} "${_src}/rigidbody.c")
coastline_add_test(sky ${_common} "${_src}/sky.c" "${_src}/skystate.c")
coastline_add_test(sort "${_src}/util.c")
coastline_add_test(splash ${_common})
coastline_add_test(sprite "${_src}/n64/sprite.c")
coastline_add_test(
  system
  ${_vecmath}
  "${_src}/assetdma.c"
  "${_src}/displaylistops.c"
  "${_src}/fog.c"
  "${_src}/framebegin.c"
  "${_src}/frameflush.c"
  "${_src}/frametime.c"
  "${_src}/iomessage.c"
  "${_src}/screen.c"
  "${_src}/screendraw.c"
  "${_src}/texturebind.c"
  "${_src}/texturestream.c"
  "${_src}/util.c")
# The test counts the calls the other units make to these through its own versions.
target_link_options(
  system_test PRIVATE -Wl,--wrap=Vec2fDistance -Wl,--wrap=Vec3fNormalize
  -Wl,--wrap=Vec3fTransformByMatrix -Wl,--wrap=ResetFrameBlocks)
coastline_add_test(texrect "${_src}/n64/texrect.c")
coastline_add_test(text "${_src}/n64/text.c" "${_src}/textrender.c" "${_src}/textstyle.c")
coastline_add_test(timing "${_src}/n64/timing.c" "${_src}/timingframe.c")
coastline_add_test(track ${_vecmath} ${_common} "${_src}/trackgrid.c" "${_src}/trackvalid.c")
coastline_add_test(trackrender ${_vecmath} "${_src}/screenrect.c" "${_src}/shadowreset.c"
                   "${_src}/trackinstance.c" "${_src}/visiblecompare.c")
coastline_add_test(vecmath ${_vecmath})
coastline_add_test(
  vehicle
  ${_vecmath}
  "${_src}/rigidbody.c"
  "${_src}/mathutil.c"
  "${_src}/trackgrid.c"
  "${_src}/tricache.c"
  "${_src}/vehicleengine.c"
  "${_src}/vehicletrackpos.c")
coastline_add_test(vehiclerender ${_vecmath} ${_gu} "${_src}/headlight.c"
                   "${_src}/vehiclevisibility.c" "${_src}/wheelrender.c")
# The test keeps the light matrices as it sets them through its own version of this routine.
target_link_options(vehiclerender_test PRIVATE -Wl,--wrap=NormalizeLightMatrix)
coastline_add_test(visgrid "${_src}/visgrid.c" "${_src}/mathutil.c")
coastline_add_test(weather ${_common} "${_src}/lightning.c" "${_src}/raininit.c")

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
