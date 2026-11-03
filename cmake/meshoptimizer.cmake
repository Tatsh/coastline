include_guard(GLOBAL)
include("${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake")
coastline_dependency(
  MESHOPTIMIZER
  PACKAGE meshoptimizer
  FIND_ARGS CONFIG
  URL https://github.com/zeux/meshoptimizer/archive/refs/tags/v1.3.tar.gz
  HASH ca4f47d451201593904b9e3067fa54b65db94bc34ab5173864950862b748a145
  OPTIONS MESHOPT_BUILD_DEMO=OFF MESHOPT_BUILD_GLTFPACK=OFF MESHOPT_BUILD_SHARED_LIBS=OFF
          MESHOPT_INSTALL=OFF)
if(TARGET meshoptimizer AND NOT TARGET meshoptimizer::meshoptimizer)
  add_library(meshoptimizer::meshoptimizer ALIAS meshoptimizer)
endif()
if(NOT USE_SYSTEM_MESHOPTIMIZER)
  include(GNUInstallDirs)
  if(VCPKG_TOOLCHAIN)
    set(_meshoptimizer_license "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/meshoptimizer/copyright")
  else()
    FetchContent_GetProperties(meshoptimizer SOURCE_DIR _meshoptimizer_source)
    set(_meshoptimizer_license "${_meshoptimizer_source}/LICENSE.md")
  endif()
  if(APPLE)
    set(_meshoptimizer_licenses "Coastline.app/Contents/Resources/licenses")
  else()
    set(_meshoptimizer_licenses "${CMAKE_INSTALL_DATADIR}/licenses/coastline")
  endif()
  install(FILES "${_meshoptimizer_license}" DESTINATION "${_meshoptimizer_licenses}"
          RENAME meshoptimizer.txt)
endif()
