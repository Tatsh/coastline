# The tests build the reconstructed C for the host and check it against the branches and words of
# the disassembly. Each test includes or links the sources it covers, with stubs for the rest.
#
# The sources assume 32-bit addresses in places: the display lists hold the addresses of globals as
# words, so the tests are linked as fixed executables that sit below 4 GB. The sources are checked
# for warnings by the build of the ROM; here they are compiled quietly.

set(_src "${CMAKE_SOURCE_DIR}/src")
set(_gu
    "${_src}/n64/libultra/gu/lookatref.c"
    "${_src}/n64/libultra/gu/lookathil.c"
    "${_src}/n64/libultra/gu/mtxcatf.c"
    "${_src}/n64/libultra/gu/mtxutil.c"
    "${_src}/n64/libultra/gu/rotate.c"
    "${_src}/n64/libultra/gu/scale.c"
    "${_src}/n64/libultra/gu/translate.c"
    "${_src}/n64/libultra/gu/normalize.c")
set(_common "${CMAKE_CURRENT_SOURCE_DIR}/stubs/common.c")
set(_vecmath "${_src}/common/vecmath.c" "${_src}/common/vecmath-light.c")

# Add a test from NAME_test.c and the sources after it.
function(coastline_add_test name)
  add_executable(${name}_test ${name}_test.c ${ARGN})
  target_include_directories(${name}_test PRIVATE "${_src}" "${_src}/n64"
                                                  "${_src}/common/libultra/include")
  # The routines a test does not reach are dropped, so they need no stubs.
  target_compile_options(${name}_test PRIVATE -w -fno-pie -fsanitize=address,undefined
                                              -ffunction-sections -fdata-sections)
  target_link_options(${name}_test PRIVATE -no-pie -fsanitize=address,undefined
                      -Wl,--gc-sections)
  target_link_libraries(${name}_test PRIVATE m)
  # A test may be given arguments in NAME_TEST_ARGS.
  add_test(NAME ${name} COMMAND ${name}_test ${${name}_TEST_ARGS})
endfunction()

add_executable(numericguards_test numericguards_test.cpp)
set_target_properties(numericguards_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(numericguards_test PRIVATE "${_src}" "${_src}/common/libultra/include")
target_compile_options(
  numericguards_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL -UTGR_BOSS_RALLY
                            -DTGR_BOSS_RALLY=1 -ffunction-sections -fdata-sections
                            -fsanitize=address,undefined,float-cast-overflow,float-divide-by-zero
                            -fno-sanitize-recover=all)
target_compile_definitions(numericguards_test PRIVATE ENABLE_PATCHES=1)
target_link_options(numericguards_test PRIVATE -Wl,--gc-sections
                      -fsanitize=address,undefined,float-cast-overflow,float-divide-by-zero)
add_test(NAME numericguards COMMAND numericguards_test)

add_executable(safetyguards_test safetyguards_test.cpp)
set_target_properties(safetyguards_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(safetyguards_test PRIVATE "${_src}" "${_src}/common"
                                                      "${_src}/common/libultra/include")
target_compile_definitions(safetyguards_test PRIVATE ENABLE_PATCHES=1 __stdcall=)
target_compile_options(
  safetyguards_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL -UTGR_BOSS_RALLY
                           -DTGR_BOSS_RALLY=1 -ffunction-sections -fdata-sections
                           -fsanitize=address,undefined -fno-sanitize-recover=all)
target_link_options(safetyguards_test PRIVATE -Wl,--gc-sections -fsanitize=address,undefined)
add_test(NAME safetyguards COMMAND safetyguards_test)

add_executable(ghostinput_test ghostinput_test.cpp)
set_target_properties(ghostinput_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(ghostinput_test PRIVATE "${_src}" "${_src}/common/libultra/include")
target_compile_definitions(ghostinput_test PRIVATE ENABLE_PATCHES=1 __stdcall=)
target_compile_options(
  ghostinput_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL -UTGR_BOSS_RALLY
                         -DTGR_BOSS_RALLY=1 -ffunction-sections -fdata-sections
                         -fsanitize=address,undefined -fno-sanitize-recover=all)
target_link_options(ghostinput_test PRIVATE -Wl,--gc-sections -fsanitize=address,undefined)
add_test(NAME ghostinput COMMAND ghostinput_test)

coastline_add_test(audio)
coastline_add_test(camera ${_vecmath} ${_gu} "${_src}/n64/libultra/gu/lookat.c"
                   "${_src}/n64/libultra/gu/perspective.c" "${_src}/common/camerarender.c")
coastline_add_test(carselect stubs/carselect.c ${_vecmath} ${_gu} "${_src}/common/laptime.c")
coastline_add_test(controllerscreen "${_src}/n64/controllerscreen.c")
coastline_add_test(debugtext "${_src}/n64/debugtext.c" "${_src}/common/debuginstance.c")
coastline_add_test(decaldraw "${_src}/n64/decaldraw.c")
coastline_add_test(decaleditor "${_src}/n64/decaleditor.c" "${_src}/n64/decaldraw.c"
                   "${_src}/common/paintshape.c")
target_compile_definitions(decaleditor_test PRIVATE ENABLE_PATCHES=1)
coastline_add_test(drawtexturedrect "${_src}/n64/texrect.c")
coastline_add_test(driving ${_vecmath} ${_common} "${_src}/common/frameclear.c" "${_src}/common/routeposition.c"
                   "${_src}/common/tracetrack.c")
coastline_add_test(
  effects
  ${_vecmath}
  "${_src}/n64/billboard-particles.c"
  "${_src}/common/exhaustspawn.c"
  "${_src}/common/particleexhaustrender.c"
  "${_src}/common/particleinit.c"
  "${_src}/common/particlesmokerender.c"
  "${_src}/common/particleupdate.c"
  "${_src}/common/tiresmokespawn.c"
  "${_src}/n64/tirespray.c"
  "${_src}/common/tiresprayrender.c"
  "${_src}/common/vehicleeffects.c")
coastline_add_test(fade ${_common} "${_src}/common/fadeupdate.c" "${_src}/common/headlight.c"
                   "${_src}/common/headlightglare.c")
coastline_add_test(
  hud
  "${_src}/n64/dashboard.c"
  "${_src}/common/hudmessage.c"
  "${_src}/common/hudoverlay.c"
  "${_src}/common/hudrace.c"
  "${_src}/common/hudtime.c")
coastline_add_test(input "${_src}/n64/input.c" "${_src}/n64/input-init.c"
                   "${_src}/common/controllerread.c" "${_src}/n64/inputtranslate.c"
                   "${_src}/common/playerinput.c")
coastline_add_test(
  model
  "${_src}/common/materialcolor.c"
  "${_src}/common/modelanimation.c"
  "${_src}/common/modelanimationflags.c"
  "${_src}/common/modelrelocation.c"
  "${_src}/common/vehiclepaint.c")
coastline_add_test(options "${_src}/common/volumecontrols.c"
                   "${_src}/common/volumecontrols-sfx.c" "${_src}/n64/options-volume.c")
coastline_add_test(paintshop "${_src}/n64/paintshop.c")
coastline_add_test(paintinput "${_src}/n64/paintinput.c")
coastline_add_test(paintshape "${_src}/common/paintshape.c")
coastline_add_test(pakmenu "${_src}/n64/pakmenu.c")
coastline_add_test(physics ${_common} "${_src}/common/rigidbody.c" "${_src}/common/cubeclip.c"
                   "${_src}/common/tricache.c" "${_src}/n64/collision-triangles.c")
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
target_sources(race_test PRIVATE "${_src}/n64/race-demo.c" "${_src}/n64/race-bgm.c")
coastline_add_test(qualityoflife "${_src}/common/qualityoflife.c")
coastline_add_test(racemenu stubs/racemenu.c ${_vecmath} ${_gu} "${_src}/n64/modeldraw.c")
target_sources(racemenu_test PRIVATE "${_src}/n64/racemenu-draw.c")
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
coastline_add_test(sky ${_common} "${_src}/n64/sky.c" "${_src}/common/skystate.c")
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
  "${_src}/common/stats.c"
  "${_src}/common/frametime.c"
  "${_src}/common/iomessage.c"
  "${_src}/common/screen.c"
  "${_src}/common/screen-stub.c"
  "${_src}/common/screendraw.c"
  "${_src}/common/fill.c"
  "${_src}/common/texturebind.c"
  "${_src}/common/texturestream.c"
  "${_src}/common/util.c"
  "${_src}/n64/depth.c"
  "${_src}/n64/framebuffer.c"
  "${_src}/n64/boot.c")
# The test counts the calls the other units make to these through its own versions.
target_link_options(
  system_test PRIVATE -Wl,--wrap=Vec2fDistance -Wl,--wrap=Vec3fNormalize
  -Wl,--wrap=Vec3fTransformByMatrix -Wl,--wrap=ResetFrameBlocks)
coastline_add_test(texrect "${_src}/n64/texrect.c")
coastline_add_test(text "${_src}/n64/text.c" "${_src}/common/textrender.c" "${_src}/common/textstyle.c")
coastline_add_test(timing "${_src}/n64/timing.c" "${_src}/common/timingframe.c")
coastline_add_test(track ${_vecmath} "${_src}/common/trackgrid.c" "${_src}/n64/boot.c")
coastline_add_test(trackrender ${_vecmath} "${_src}/common/screenrect.c" "${_src}/common/shadowreset.c"
                   "${_src}/n64/trackinstance.c" "${_src}/common/visiblecompare.c")
target_compile_definitions(trackrender_test PRIVATE ENABLE_PATCHES=1)
coastline_add_test(vecmath ${_vecmath})
coastline_add_test(
  vehicle
  ${_vecmath}
  "${_src}/common/rigidbody.c"
  "${_src}/common/mathutil.c"
  "${_src}/common/trackgrid.c"
  "${_src}/common/tricache.c"
  "${_src}/n64/ground-ray.c"
  "${_src}/common/vehicleengine.c"
  "${_src}/n64/vehicle-track-position.c")
coastline_add_test(vehiclerender ${_vecmath} ${_gu} "${_src}/common/headlight.c"
                   "${_src}/n64/headlight-beams.c"
                   "${_src}/common/vehiclevisibility.c" "${_src}/common/wheelrender.c")
# The test keeps the light matrices as it sets them through its own version of this routine.
target_link_options(vehiclerender_test PRIVATE -Wl,--wrap=NormalizeLightMatrix)
coastline_add_test(visgrid "${_src}/common/visgrid.c" "${_src}/common/mathutil.c")
coastline_add_test(visgrid_patched "${_src}/common/visgrid.c" "${_src}/common/mathutil.c")
target_compile_options(visgrid_patched_test PRIVATE -UTGR_PLATFORM
  -DTGR_PLATFORM=TGR_PLATFORM_SDL -fsanitize=float-cast-overflow,float-divide-by-zero
  -fno-sanitize-recover=all)
target_compile_definitions(visgrid_patched_test PRIVATE ENABLE_PATCHES=1)
target_link_options(visgrid_patched_test PRIVATE
  -fsanitize=float-cast-overflow,float-divide-by-zero)
coastline_add_test(weather ${_common} "${_src}/common/lightning.c" "${_src}/common/raininit.c"
                   "${_src}/n64/precipitation-update.c")

# The libultra C library, renamed apart from the host's.
coastline_add_test(bcopy stubs/libc.c)
coastline_add_test(llcvt "${_src}/n64/libultra/libc/llcvt.c")
coastline_add_test(
  printf "${_src}/n64/libultra/libc/sprintf.c" "${_src}/n64/libultra/libc/xprintf.c"
  "${_src}/n64/libultra/libc/xlitob.c" "${_src}/n64/libultra/libc/xldtob.c")
set_source_files_properties("${_src}/n64/libultra/libc/sprintf.c" PROPERTIES COMPILE_DEFINITIONS
                                                                         sprintf=lu_sprintf)
coastline_add_test(gu "${_src}/n64/libultra/gu/sinf.c" "${_src}/n64/libultra/gu/cosf.c"
                   "${_src}/n64/libultra/gu/mtxutil.c" "${_src}/n64/libultra/gu/rotate.c"
                   "${_src}/n64/libultra/gu/normalize.c")
set_source_files_properties("${_src}/n64/libultra/gu/sinf.c" PROPERTIES COMPILE_DEFINITIONS sinf=gu_sinf)
set_source_files_properties("${_src}/n64/libultra/gu/cosf.c" PROPERTIES COMPILE_DEFINITIONS cosf=gu_cosf)

# The directories, the configuration file, and the icon import of the SDL game, which need neither
# SDL nor a display.
enable_language(CXX)
add_executable(trackskidmarks_test trackskidmarks_test.cpp)
set_target_properties(trackskidmarks_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(trackskidmarks_test PRIVATE "${_src}" "${_src}/common/libultra/include")
target_compile_options(trackskidmarks_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL
                                                -UTGR_BOSS_RALLY -DTGR_BOSS_RALLY=1
                                                -ffunction-sections -fdata-sections
                                                -fsanitize=address,undefined)
target_link_options(trackskidmarks_test PRIVATE -Wl,--gc-sections -fsanitize=address,undefined)
add_test(NAME trackskidmarks COMMAND trackskidmarks_test)

add_executable(ptr32memory_test ptr32memory_test.cpp "${_src}/sdl/ptr32.cpp")
set_target_properties(ptr32memory_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(ptr32memory_test PRIVATE "${_src}" "${_src}/sdl")
target_compile_options(ptr32memory_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL
                                             -Wall -Wextra -fsanitize=address,undefined)
target_link_options(ptr32memory_test PRIVATE -fsanitize=address,undefined)
add_test(NAME ptr32memory COMMAND ptr32memory_test)

add_executable(freecamera_test freecamera_test.cpp "${_src}/sdl/freecamera.cpp")
set_target_properties(freecamera_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(freecamera_test PRIVATE "${_src}/sdl")
target_compile_options(freecamera_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(freecamera_test PRIVATE -fsanitize=address,undefined)
add_test(NAME freecamera COMMAND freecamera_test)

add_executable(
  sdl_test sdl_test.cpp "${_src}/sdl/config.cpp" "${_src}/sdl/paths.cpp" "${_src}/sdl/assetstorage.cpp" "${_src}/sdl/iconfile.cpp"
           "${_src}/sdl/importversion.cpp" "${_src}/sdl/language.cpp" "${_src}/sdl/consolestrings.cpp"
           "${_src}/sdl/runscript.cpp")
set_target_properties(sdl_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(sdl_test PRIVATE "${_src}/sdl")
target_compile_options(sdl_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(sdl_test PRIVATE -fsanitize=address,undefined)
add_test(NAME sdl COMMAND sdl_test)
get_target_property(_sdl_test_sources sdl_test SOURCES)
add_executable(sdl_paths_fixed_test ${_sdl_test_sources})
set_target_properties(sdl_paths_fixed_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(sdl_paths_fixed_test PRIVATE "${_src}/sdl")
target_compile_definitions(sdl_paths_fixed_test PRIVATE COASTLINE_PORTABLE=0)
target_compile_options(sdl_paths_fixed_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(sdl_paths_fixed_test PRIVATE -fsanitize=address,undefined)
add_test(NAME sdl_paths_fixed COMMAND sdl_paths_fixed_test)
set_tests_properties(sdl sdl_paths_fixed PROPERTIES RESOURCE_LOCK coastline-sdl-test)

include("${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake")
coastline_dependency(
  SDL3
  PACKAGE SDL3
  FIND_ARGS CONFIG
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz
  HASH 9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
  OPTIONS SDL_SHARED=OFF SDL_STATIC=ON SDL_TEST_LIBRARY=OFF SDL_EXAMPLES=OFF SDL_TESTS=OFF
          SDL_INSTALL=OFF SDL_VIDEO=OFF SDL_GPU=OFF SDL_RENDER=OFF SDL_CAMERA=OFF SDL_JOYSTICK=OFF
          SDL_HAPTIC=OFF SDL_HIDAPI=OFF SDL_POWER=OFF SDL_SENSOR=OFF SDL_DIALOG=OFF SDL_TRAY=OFF
          SDL_UNIX_CONSOLE_BUILD=ON)
add_executable(sdlmixer_test sdlmixer_test.cpp "${_src}/sdl/music.cpp" "${_src}/common/audio.c")
set_target_properties(sdlmixer_test PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED YES
                                              CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(sdlmixer_test PRIVATE "${_src}" "${_src}/sdl"
                                               "${_src}/common/libultra/include")
target_compile_options(sdlmixer_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL
                                           -UTGR_BOSS_RALLY -DTGR_BOSS_RALLY=1 -fsigned-char
                                           -Wall -Wextra -fsanitize=address,undefined)
target_link_options(sdlmixer_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(sdlmixer_test PRIVATE SDL3::SDL3)
add_test(NAME sdlmixer COMMAND sdlmixer_test)
set_tests_properties(sdlmixer PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")

add_executable(soundeffects_test soundeffects_test.cpp "${_src}/sdl/soundeffects.cpp")
set_target_properties(soundeffects_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(soundeffects_test PRIVATE "${_src}/sdl")
target_compile_options(soundeffects_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(soundeffects_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(soundeffects_test PRIVATE SDL3::SDL3)
add_test(NAME soundeffects COMMAND soundeffects_test)
set_tests_properties(soundeffects PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")

coastline_dependency(
  FLAC
  PACKAGE FLAC
  FIND_ARGS CONFIG
  URL https://github.com/xiph/flac/releases/download/1.5.0/flac-1.5.0.tar.xz
  HASH f2c1c76592a82ffff8413ba3c4a1299b6c7ab06c734dee03fd88630485c2b920
  OPTIONS BUILD_CXXLIBS=OFF BUILD_PROGRAMS=OFF BUILD_EXAMPLES=OFF BUILD_TESTING=OFF BUILD_DOCS=OFF
          INSTALL_MANPAGES=OFF WITH_OGG=OFF)
add_executable(flactrack_test flactrack_test.cpp "${_src}/sdl/flactrack.cpp")
set_target_properties(flactrack_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(flactrack_test PRIVATE "${_src}/sdl")
target_compile_options(flactrack_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(flactrack_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(flactrack_test PRIVATE SDL3::SDL3 FLAC::FLAC)
add_test(NAME flactrack COMMAND flactrack_test)
set_tests_properties(flactrack PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")

add_executable(sdltextures_test sdltextures_test.cpp "${_src}/sdl/enginetextures.cpp")
set_target_properties(sdltextures_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(sdltextures_test PRIVATE "${_src}" "${_src}/sdl"
                                                  "${_src}/common/libultra/include")
target_compile_options(sdltextures_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL
                                              -UTGR_BOSS_RALLY -DTGR_BOSS_RALLY=1
                                              -Wall -Wextra -fsanitize=address,undefined)
target_link_options(sdltextures_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(sdltextures_test PRIVATE SDL3::SDL3)
add_test(NAME sdltextures COMMAND sdltextures_test)
set_tests_properties(sdltextures PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")

add_executable(sdlruntime_test sdlruntime_test.cpp "${_src}/sdl/runtimebuffers.cpp")
set_target_properties(sdlruntime_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(sdlruntime_test PRIVATE "${_src}" "${_src}/sdl"
                                                 "${_src}/common/libultra/include")
target_compile_options(sdlruntime_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL
                                             -UTGR_BOSS_RALLY -DTGR_BOSS_RALLY=1
                                             -Wall -Wextra -fsanitize=address,undefined)
target_link_options(sdlruntime_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(sdlruntime_test PRIVATE SDL3::SDL3)
add_test(NAME sdlruntime COMMAND sdlruntime_test)
set_tests_properties(sdlruntime PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")

coastline_dependency(
  NLOHMANN_JSON
  PACKAGE nlohmann_json
  FIND_ARGS CONFIG
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  HASH 42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  OPTIONS JSON_BuildTests=OFF JSON_Install=OFF)
coastline_dependency(
  FMT
  PACKAGE fmt
  FIND_ARGS CONFIG
  URL https://github.com/fmtlib/fmt/releases/download/12.2.0/fmt-12.2.0.zip
  HASH a2f4a8d51178f954e4c339007f77edd76ba0cb2e36f87a48e5a5403d9be5878f
  OPTIONS FMT_INSTALL=OFF FMT_TEST=OFF FMT_DOC=OFF)
add_executable(disclayout_test disclayout_test.cpp "${_src}/sdl/disclayout.cpp"
                              "${_src}/sdl/isz.cpp"
                              "${_src}/sdl/pdi.cpp"
                              "${_src}/sdl/decodeddisc.cpp"
                              "${_src}/sdl/disctrackreader.cpp"
                              "${CMAKE_SOURCE_DIR}/tools/common/sha1.cpp")
set_target_properties(disclayout_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(disclayout_test PRIVATE "${_src}/sdl"
                                                "${CMAKE_SOURCE_DIR}/tools/common")
target_compile_options(disclayout_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(disclayout_test PRIVATE -fsanitize=address,undefined)
include("${CMAKE_SOURCE_DIR}/cmake/bzip2.cmake")
target_link_libraries(disclayout_test PRIVATE fmt::fmt BZip2::BZip2 ZLIB::ZLIB)
add_test(NAME disclayout COMMAND disclayout_test)

add_executable(isz_test isz_test.cpp "${_src}/sdl/isz.cpp" "${_src}/sdl/decodeddisc.cpp"
                        "${_src}/sdl/disctrackreader.cpp")
set_target_properties(isz_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(isz_test PRIVATE "${_src}/sdl")
target_compile_options(isz_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(isz_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(isz_test PRIVATE fmt::fmt BZip2::BZip2 ZLIB::ZLIB)
add_test(NAME isz COMMAND isz_test)

add_executable(pdi_test pdi_test.cpp "${_src}/sdl/pdi.cpp" "${_src}/sdl/decodeddisc.cpp")
set_target_properties(pdi_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(pdi_test PRIVATE "${_src}/sdl")
target_compile_options(pdi_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(pdi_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(pdi_test PRIVATE fmt::fmt)
add_test(NAME pdi COMMAND pdi_test)

add_executable(log_test log_test.cpp "${_src}/sdl/log.cpp")
set_target_properties(log_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(log_test PRIVATE "${_src}/sdl")
target_compile_options(log_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(log_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(log_test PRIVATE SDL3::SDL3 fmt::fmt)
add_test(NAME log COMMAND log_test)

add_executable(n64model_test n64model_test.cpp "${_src}/sdl/n64model.cpp")
set_target_properties(n64model_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(n64model_test PRIVATE "${_src}/sdl")
target_compile_options(n64model_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(n64model_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(n64model_test PRIVATE nlohmann_json::nlohmann_json fmt::fmt)
add_test(NAME n64model COMMAND n64model_test)

include("${CMAKE_SOURCE_DIR}/cmake/meshoptimizer.cmake")
add_executable(trackpreview_test trackpreview_test.cpp "${_src}/sdl/trackpreview.cpp")
set_target_properties(trackpreview_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(trackpreview_test PRIVATE "${_src}/sdl")
target_compile_options(trackpreview_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(trackpreview_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(trackpreview_test PRIVATE nlohmann_json::nlohmann_json meshoptimizer::meshoptimizer)
add_test(NAME trackpreview COMMAND trackpreview_test)

add_executable(decalcar_test decalcar_test.cpp "${_src}/sdl/decalcar.cpp"
  "${_src}/sdl/gltfdocument.cpp" "${_src}/sdl/indexedimage.cpp" "${_src}/sdl/n64model.cpp")
set_target_properties(decalcar_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(decalcar_test PRIVATE "${_src}" "${_src}/sdl"
                                                "${_src}/common/libultra/include")
target_compile_options(decalcar_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(decalcar_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(decalcar_test PRIVATE SDL3::SDL3 nlohmann_json::nlohmann_json fmt::fmt)
add_test(NAME decalcar COMMAND decalcar_test)

add_executable(gamesettings_test gamesettings_test.cpp "${_src}/sdl/gamesettings.cpp"
  "${_src}/sdl/config.cpp" "${_src}/sdl/language.cpp")
set_target_properties(gamesettings_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(gamesettings_test PRIVATE "${_src}/sdl")
target_compile_options(gamesettings_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(gamesettings_test PRIVATE -fsanitize=address,undefined)
add_test(NAME gamesettings COMMAND gamesettings_test)

add_executable(gltfdocument_test gltfdocument_test.cpp "${_src}/sdl/gltfdocument.cpp")
set_target_properties(gltfdocument_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(gltfdocument_test PRIVATE "${_src}/sdl")
target_link_libraries(gltfdocument_test PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(gltfdocument_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(gltfdocument_test PRIVATE -fsanitize=address,undefined)
add_test(NAME gltfdocument COMMAND gltfdocument_test)

add_executable(assetimport_test assetimport_test.cpp "${_src}/sdl/assetimport.cpp"
                              "${_src}/sdl/importjobs.cpp"
                              "${_src}/sdl/importversion.cpp" "${_src}/sdl/config.cpp"
                              "${_src}/sdl/assetstorage.cpp")
set_target_properties(assetimport_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(assetimport_test PRIVATE "${_src}/sdl")
target_link_libraries(assetimport_test PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(assetimport_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(assetimport_test PRIVATE -fsanitize=address,undefined)
add_test(NAME assetimport COMMAND assetimport_test)

add_executable(importjobs_test importjobs_test.cpp "${_src}/sdl/importjobs.cpp")
set_target_properties(importjobs_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(importjobs_test PRIVATE "${_src}/sdl")
target_compile_options(importjobs_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(importjobs_test PRIVATE -fsanitize=address,undefined)
add_test(NAME importjobs COMMAND importjobs_test)

add_executable(assetstorage_test assetstorage_test.cpp "${_src}/sdl/assetstorage.cpp"
                               "${_src}/sdl/paths.cpp" "${_src}/sdl/importversion.cpp"
                               "${_src}/sdl/config.cpp")
set_target_properties(assetstorage_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(assetstorage_test PRIVATE "${_src}/sdl")
target_compile_options(assetstorage_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(assetstorage_test PRIVATE -fsanitize=address,undefined)
add_test(NAME assetstorage COMMAND assetstorage_test)

find_package(Threads REQUIRED)
add_executable(firstrun_test firstrun_test.cpp "${_src}/sdl/firstrun/filebrowser.cpp"
               "${_src}/sdl/firstrun/setupwizard.cpp" "${_src}/sdl/firstrun/textgrid.cpp"
               "${_src}/sdl/firstrun/assetsearch.cpp")
set_target_properties(firstrun_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(firstrun_test PRIVATE "${_src}/sdl/firstrun")
target_compile_options(firstrun_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(firstrun_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(firstrun_test PRIVATE Threads::Threads)
add_test(NAME firstrun COMMAND firstrun_test)

find_package(ZLIB REQUIRED)
add_executable(seasonfiles_test seasonfiles_test.cpp "${_src}/common/season.cpp")
set_target_properties(seasonfiles_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(seasonfiles_test PRIVATE "${_src}" "${_src}/common/libultra/include")
target_compile_definitions(seasonfiles_test PRIVATE ENABLE_PATCHES=1)
target_compile_options(
  seasonfiles_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL -UTGR_BOSS_RALLY
                           -DTGR_BOSS_RALLY=1 -ffunction-sections
                           -fdata-sections -fsanitize=address,undefined)
target_link_options(seasonfiles_test PRIVATE -Wl,--gc-sections -fsanitize=address,undefined)
target_link_libraries(seasonfiles_test PRIVATE ZLIB::ZLIB)
add_test(NAME seasonfiles COMMAND seasonfiles_test)

add_executable(ziproms_test ziproms_test.cpp "${_src}/sdl/firstrun/ziproms.cpp")
set_target_properties(ziproms_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(ziproms_test PRIVATE "${_src}/sdl/firstrun")
target_compile_options(ziproms_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(ziproms_test PRIVATE -fsanitize=address,undefined)
target_link_libraries(ziproms_test PRIVATE ZLIB::ZLIB)
add_test(NAME ziproms COMMAND ziproms_test)

# The note of the ghost car of the SDL game, with the run-length routines as the SDL game builds
# them.
add_executable(ghostnote_test ghostnote_test.cpp "${_src}/sdl/ghostnote.cpp"
                              "${_src}/common/replay.c" "${_src}/common/rledecompress.c")
set_target_properties(ghostnote_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(ghostnote_test PRIVATE "${_src}" "${_src}/sdl"
                                                  "${_src}/common/libultra/include")
target_compile_options(
  ghostnote_test PRIVATE -UTGR_PLATFORM -DTGR_PLATFORM=TGR_PLATFORM_SDL -UTGR_BOSS_RALLY
                         -DTGR_BOSS_RALLY=1 -fsanitize=address,undefined)
target_link_options(ghostnote_test PRIVATE -fsanitize=address,undefined)
add_test(NAME ghostnote COMMAND ghostnote_test)

# The interpreter of the processor of the console that records the attract demos at import.
add_executable(mipscpu_test mipscpu_test.cpp "${_src}/sdl/mipscpu.cpp")
set_target_properties(mipscpu_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(mipscpu_test PRIVATE "${_src}/sdl")
target_compile_options(mipscpu_test PRIVATE -Wall -Wextra -ffp-contract=off
                                            -fsanitize=address,undefined)
target_link_options(mipscpu_test PRIVATE -fsanitize=address,undefined)
add_test(NAME mipscpu COMMAND mipscpu_test)

add_library(savefile_test_support STATIC "${_src}/sdl/savefile.cpp"
             "${CMAKE_SOURCE_DIR}/tools/common/sha1.cpp")
set_target_properties(savefile_test_support PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(savefile_test_support PUBLIC "${_src}/sdl"
                            "${CMAKE_SOURCE_DIR}/tools/common")
target_compile_options(savefile_test_support PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(savefile_test_support INTERFACE -fsanitize=address,undefined)
foreach(_target IN ITEMS sdl_test sdl_paths_fixed_test gamesettings_test assetimport_test
                          assetstorage_test ghostnote_test)
  target_link_libraries(${_target} PRIVATE savefile_test_support)
endforeach()
add_executable(savefile_test savefile_test.cpp)
set_target_properties(savefile_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_compile_options(savefile_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_libraries(savefile_test PRIVATE savefile_test_support)
add_test(NAME savefile COMMAND savefile_test)

add_executable(controllerpak_test controllerpak_test.cpp "${_src}/sdl/controllerpak.cpp")
set_target_properties(controllerpak_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_link_libraries(controllerpak_test PRIVATE savefile_test_support SDL3::SDL3 fmt::fmt)
target_compile_options(controllerpak_test PRIVATE -Wall -Wextra -fsanitize=address,undefined)
target_link_options(controllerpak_test PRIVATE -fsanitize=address,undefined)
add_test(NAME controllerpak COMMAND controllerpak_test)

add_executable(notesafety_test notesafety_test.cpp "${_src}/sdl/notesafety.cpp")
set_target_properties(notesafety_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(notesafety_test PRIVATE "${_src}/sdl" "${_src}"
                              "${_src}/common/libultra/include")
target_compile_options(notesafety_test PRIVATE -Wall -Wextra
    -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all)
target_link_options(notesafety_test PRIVATE -fsanitize=address,undefined,float-cast-overflow)
add_test(NAME notesafety COMMAND notesafety_test)

add_executable(configsafety_test configsafety_test.cpp "${_src}/sdl/config.cpp")
set_target_properties(configsafety_test PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES)
target_include_directories(configsafety_test PRIVATE "${_src}/sdl")
target_compile_options(configsafety_test PRIVATE -Wall -Wextra
                           -fsanitize=address,undefined,float-cast-overflow)
target_link_options(configsafety_test PRIVATE -fsanitize=address,undefined,float-cast-overflow)
target_link_libraries(configsafety_test PRIVATE savefile_test_support)
add_test(NAME configsafety COMMAND configsafety_test)

foreach(_enabled IN ITEMS 0 1)
  add_executable(discordpresence_${_enabled}_test discordpresence_test.cpp
                   "${_src}/sdl/discordpresence.cpp")
  set_target_properties(discordpresence_${_enabled}_test PROPERTIES CXX_STANDARD 23
                          CXX_STANDARD_REQUIRED YES)
  target_include_directories(discordpresence_${_enabled}_test PRIVATE "${_src}/sdl")
  target_compile_definitions(discordpresence_${_enabled}_test PRIVATE COASTLINE_DISCORD=${_enabled})
  target_compile_options(discordpresence_${_enabled}_test PRIVATE -Wall -Wextra
                            -fsanitize=address,undefined)
  target_link_options(discordpresence_${_enabled}_test PRIVATE -fsanitize=address,undefined)
  target_link_libraries(discordpresence_${_enabled}_test PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME discordpresence_${_enabled} COMMAND discordpresence_${_enabled}_test)
endforeach()
set_tests_properties(discordpresence_0 discordpresence_1 PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)

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
