if(NOT WIN32)
  return()
endif()

# Scan actual PE imports, including libraries represented by UNKNOWN imported targets such as
# ZLIB::ZLIB. TARGET_RUNTIME_DLLS omits UNKNOWN imported libraries and their transitive dependencies.
set(_runtime_directories
    "$<TARGET_FILE_DIR:SDL3::SDL3>"
    "$<TARGET_FILE_DIR:FLAC::FLAC>"
    "$<TARGET_FILE_DIR:fmt::fmt>"
    "$<TARGET_FILE_DIR:ZLIB::ZLIB>")
if(MINGW)
  # The MinGW runtime DLLs sit beside the compiler rather than in the vcpkg install tree.
  get_filename_component(_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
  list(APPEND _runtime_directories "${_mingw_bin}")
endif()
if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
  list(APPEND _runtime_directories
       "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin")
endif()
install(
  RUNTIME_DEPENDENCY_SET coastline_runtime
  DIRECTORIES ${_runtime_directories}
  PRE_EXCLUDE_REGEXES "[Aa][Pp][Ii]-[Mm][Ss]-.*" "[Ee][Xx][Tt]-[Mm][Ss]-.*"
  POST_EXCLUDE_REGEXES
    ".*[/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"
    ".*[/\\\\][Ss][Yy][Ss][Ww][Oo][Ww]64[/\\\\].*"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(MSVC)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_INSTALL_DEBUG_LIBRARIES ON)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY ON)
    set(CMAKE_INSTALL_UCRT_LIBRARIES ON)
  endif()
  include(InstallRequiredSystemLibraries)
endif()
