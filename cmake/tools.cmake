# Builds the host tools under tools/ with the host compiler, in a separate build under
# ${CMAKE_BINARY_DIR}/tools. The tools are configured and built here, at configure time, and
# rebuilt by the coastline_tools target before a command runs one of them.
#
# Each COASTLINE_TOOL_* variable is the path of one tool. A command that runs a tool lists the
# variable and coastline_tools in its DEPENDS.

set(COASTLINE_TOOLS_CMAKE_ARGS
    ""
    CACHE STRING "Extra arguments for configuring the host tools, for example \
-DZLIB_ROOT=/opt/zlib.")
set(COASTLINE_TOOLS_VCPKG_ROOT
    ""
    CACHE PATH "vcpkg checkout that provides the host tools' dependencies listed in vcpkg.json at \
the repository root. The VCPKG_ROOT environment variable is used when COASTLINE_TOOLS_VCPKG_ROOT \
is empty.")

set(_coastline_tools_vcpkg_root "${COASTLINE_TOOLS_VCPKG_ROOT}")
if(NOT _coastline_tools_vcpkg_root)
  set(_coastline_tools_vcpkg_root "$ENV{VCPKG_ROOT}")
endif()
set(_coastline_tools_args ${COASTLINE_TOOLS_CMAKE_ARGS})
if(_coastline_tools_vcpkg_root)
  list(APPEND _coastline_tools_args
       "-DCMAKE_TOOLCHAIN_FILE=${_coastline_tools_vcpkg_root}/scripts/buildsystems/vcpkg.cmake"
       "-DVCPKG_MANIFEST_DIR=${CMAKE_SOURCE_DIR}")
  message(STATUS "The host tools use vcpkg from ${_coastline_tools_vcpkg_root}.")
endif()

set(COASTLINE_TOOLS_DIR "${CMAKE_BINARY_DIR}/tools")
set(COASTLINE_TOOL_EXTRACT_ROM "${COASTLINE_TOOLS_DIR}/extract-rom/extract-rom")

# The host build does not receive the cross toolchain file, on the command line or through the
# environment. Its only toolchain file is the host vcpkg one.
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env --unset=CMAKE_TOOLCHAIN_FILE "${CMAKE_COMMAND}" -S
          "${CMAKE_SOURCE_DIR}/tools" -B "${COASTLINE_TOOLS_DIR}" -G "${CMAKE_GENERATOR}"
          ${_coastline_tools_args}
  RESULT_VARIABLE _coastline_tools_result)
if(NOT _coastline_tools_result EQUAL 0)
  message(FATAL_ERROR "Configuring the host tools in ${COASTLINE_TOOLS_DIR} failed.")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${COASTLINE_TOOLS_DIR}"
                RESULT_VARIABLE _coastline_tools_result)
if(NOT _coastline_tools_result EQUAL 0)
  message(FATAL_ERROR "Building the host tools in ${COASTLINE_TOOLS_DIR} failed.")
endif()

add_custom_target(
  coastline_tools
  COMMAND "${CMAKE_COMMAND}" --build "${COASTLINE_TOOLS_DIR}"
  BYPRODUCTS "${COASTLINE_TOOL_EXTRACT_ROM}"
  COMMENT "Building the host tools"
  VERBATIM)
