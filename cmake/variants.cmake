# Builds every variant TGR_VARIANTS lists as its own configure and build under
# ${CMAKE_BINARY_DIR}/<variant>, from this source tree. Each variant is <platform>-<choice>, with
# n64-us, n64-jp, and n64-eu for the Nintendo 64 releases and win-d3d and win-glide for the
# Windows libraries. The word all selects the five.

include(ExternalProject)

set(_tgr_all_variants n64-us n64-jp n64-eu win-d3d win-glide)
if(TGR_VARIANTS STREQUAL "all")
  set(TGR_VARIANTS ${_tgr_all_variants})
endif()

foreach(_variant ${TGR_VARIANTS})
  if(NOT _variant IN_LIST _tgr_all_variants)
    message(FATAL_ERROR "TGR_VARIANTS has ${_variant}. Each entry must be one of "
                        "${_tgr_all_variants}, or the list must be all.")
  endif()
  string(REGEX MATCH "^([a-z0-9]+)-([a-z0-9]+)$" _ "${_variant}")
  set(_platform "${CMAKE_MATCH_1}")
  set(_choice "${CMAKE_MATCH_2}")
  if(_platform STREQUAL "n64")
    set(_choice_arg "-DTGR_REGION=${_choice}")
  else()
    set(_choice_arg "-DTGR_RENDERER=${_choice}")
  endif()
  # The sources change between builds of the superbuild, and the variant rebuilds every time.
  ExternalProject_Add(
    ${_variant}
    SOURCE_DIR "${CMAKE_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_BINARY_DIR}/${_variant}"
    CMAKE_ARGS -DTGR_PLATFORM=${_platform} ${_choice_arg}
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    BUILD_ALWAYS ON
    BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --target archives
    INSTALL_COMMAND ""
    TEST_COMMAND "")
endforeach()
