# Toolchain file for the Windows libraries. It selects an i686 mingw-w64 cross compiler, from
# MINGW_ROOT or the llvm-mingw distribution unpacked under .wiswa-ci.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)

if(DEFINED ENV{MINGW_ROOT})
  set(MINGW_ROOT
      "$ENV{MINGW_ROOT}"
      CACHE PATH "mingw-w64 distribution root that provides the i686 cross compiler.")
endif()
file(GLOB _mingw_bins "${CMAKE_CURRENT_LIST_DIR}/../.wiswa-ci/toolchains/mingw/*/bin")
find_program(
  CMAKE_C_COMPILER i686-w64-mingw32-gcc
  PATHS "${MINGW_ROOT}/bin" ${_mingw_bins}
  REQUIRED)
find_program(
  CMAKE_CXX_COMPILER i686-w64-mingw32-g++
  PATHS "${MINGW_ROOT}/bin" ${_mingw_bins}
  REQUIRED)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
