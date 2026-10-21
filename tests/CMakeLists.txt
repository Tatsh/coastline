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
  target_include_directories(${name}_test PRIVATE "${_src}" "${_src}/libultra/include")
  target_compile_options(${name}_test PRIVATE -w -fno-pie -fsanitize=address,undefined)
  target_link_options(${name}_test PRIVATE -no-pie -fsanitize=address,undefined)
  target_link_libraries(${name}_test PRIVATE m)
  # A test may be given arguments in NAME_TEST_ARGS.
  add_test(NAME ${name} COMMAND ${name}_test ${${name}_TEST_ARGS})
endfunction()

coastline_add_test(audio)
coastline_add_test(carselect stubs/carselect.c ${_vecmath} ${_gu})
coastline_add_test(controllerscreen "${_src}/controllerscreen.c")
coastline_add_test(debugtext "${_src}/debugtext.c")
coastline_add_test(drawtexturedrect "${_src}/texrect.c")
coastline_add_test(driving ${_vecmath} ${_common})
coastline_add_test(effects ${_vecmath})
coastline_add_test(fade ${_common})
coastline_add_test(hud)
coastline_add_test(input "${_src}/input.c")
coastline_add_test(options)
coastline_add_test(pakmenu "${_src}/pakmenu.c")
coastline_add_test(physics ${_common} "${_src}/rigidbody.c")
coastline_add_test(race stubs/race.c ${_common} ${_vecmath} ${_gu})
coastline_add_test(racemenu stubs/racemenu.c ${_vecmath} ${_gu})
coastline_add_test(racers ${_vecmath} ${_common})
coastline_add_test(racesound stubs/racesound.c ${_vecmath} ${_common})
coastline_add_test(replay)
coastline_add_test(rigidbody ${_common} "${_src}/rigidbody.c")
coastline_add_test(sky ${_common} "${_src}/sky.c")
coastline_add_test(sort "${_src}/util.c")
coastline_add_test(splash ${_common})
coastline_add_test(texrect "${_src}/texrect.c")
coastline_add_test(text "${_src}/text.c")
coastline_add_test(timing "${_src}/timing.c")
coastline_add_test(track ${_vecmath} ${_common})
coastline_add_test(trackrender ${_vecmath})
coastline_add_test(vecmath ${_vecmath})
coastline_add_test(vehicle ${_vecmath} "${_src}/rigidbody.c" "${_src}/mathutil.c")
coastline_add_test(vehiclerender ${_vecmath} ${_gu})
coastline_add_test(visgrid "${_src}/visgrid.c" "${_src}/mathutil.c")
coastline_add_test(weather ${_common})

# The libultra C library, renamed apart from the host's.
coastline_add_test(bcopy "${_src}/libultra/libc/bcopy.c" "${_src}/libultra/libc/bzero.c"
                   "${_src}/libultra/libc/bcmp.c")
set_source_files_properties(
  "${_src}/libultra/libc/bcopy.c" "${_src}/libultra/libc/bzero.c" "${_src}/libultra/libc/bcmp.c"
  PROPERTIES COMPILE_DEFINITIONS "bcopy=lu_bcopy;bzero=lu_bzero;bcmp=lu_bcmp")
coastline_add_test(
  printf "${_src}/libultra/libc/sprintf.c" "${_src}/libultra/libc/xprintf.c"
  "${_src}/libultra/libc/xlitob.c" "${_src}/libultra/libc/xldtob.c")
set_source_files_properties("${_src}/libultra/libc/sprintf.c" PROPERTIES COMPILE_DEFINITIONS
                                                                         sprintf=lu_sprintf)
coastline_add_test(gu "${_src}/libultra/gu/sinf.c" "${_src}/libultra/gu/cosf.c"
                   "${_src}/libultra/gu/mtxutil.c")
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
