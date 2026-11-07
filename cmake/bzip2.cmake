include_guard(GLOBAL)
include(GNUInstallDirs)

option(USE_SYSTEM_BZIP2 "Use system bzip2 without downloading it." OFF)
if(USE_SYSTEM_BZIP2 OR VCPKG_TOOLCHAIN)
  find_package(BZip2 REQUIRED)
else()
  include(FetchContent)
  FetchContent_Declare(
    coastline_bzip2
    URL https://sourceware.org/pub/bzip2/bzip2-1.0.8.tar.gz
    URL_HASH SHA256=ab5a03176ee106d3f0fa90e381da478ddae405918153cca248e682cd0c4a2269
    SYSTEM)
  FetchContent_MakeAvailable(coastline_bzip2)
  add_library(coastline_bzip2 STATIC
    "${coastline_bzip2_SOURCE_DIR}/blocksort.c"
    "${coastline_bzip2_SOURCE_DIR}/huffman.c"
    "${coastline_bzip2_SOURCE_DIR}/crctable.c"
    "${coastline_bzip2_SOURCE_DIR}/randtable.c"
    "${coastline_bzip2_SOURCE_DIR}/compress.c"
    "${coastline_bzip2_SOURCE_DIR}/decompress.c"
    "${coastline_bzip2_SOURCE_DIR}/bzlib.c")
  if(MSVC)
    target_compile_options(coastline_bzip2 PRIVATE /w)
  else()
    target_compile_options(coastline_bzip2 PRIVATE -w)
  endif()
  target_include_directories(coastline_bzip2 SYSTEM PUBLIC "${coastline_bzip2_SOURCE_DIR}")
  set_target_properties(coastline_bzip2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
  add_library(BZip2::BZip2 ALIAS coastline_bzip2)
  if(APPLE)
    set(_bzip2_licenses "Coastline.app/Contents/Resources/licenses")
  else()
    set(_bzip2_licenses "${CMAKE_INSTALL_DATADIR}/licenses/coastline")
  endif()
  install(FILES "${coastline_bzip2_SOURCE_DIR}/LICENSE"
          DESTINATION "${_bzip2_licenses}" RENAME BZip2.txt)
endif()
