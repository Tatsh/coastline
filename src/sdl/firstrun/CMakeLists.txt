# The setup wizard of the first run of the SDL game. The glyphs of unscii-16 are converted to a
# header at build time.
set(_font_source "${CMAKE_SOURCE_DIR}/3rdparty/unscii/unscii-16.hex")
set(_font_header "${CMAKE_CURRENT_BINARY_DIR}/firstrun/unscii16.h")
add_custom_command(
  OUTPUT "${_font_header}"
  COMMAND "${CMAKE_COMMAND}" -DINPUT=${_font_source} -DTABLE=${CMAKE_CURRENT_SOURCE_DIR}/textgrid.cpp
          -DOUTPUT=${_font_header} -DNAME=UNSCII_16 -P "${CMAKE_SOURCE_DIR}/cmake/unscii.cmake"
  DEPENDS "${_font_source}" "${CMAKE_CURRENT_SOURCE_DIR}/textgrid.cpp"
          "${CMAKE_SOURCE_DIR}/cmake/unscii.cmake"
  COMMENT "Converting the glyphs of unscii-16"
  VERBATIM)

find_package(Threads REQUIRED)
add_library(coastline_firstrun STATIC assetsearch.cpp filebrowser.cpp setupwizard.cpp textfont.cpp textgrid.cpp
                                      wizardrun.cpp ziproms.cpp "${_font_header}")
target_include_directories(coastline_firstrun PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                           PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(coastline_firstrun PRIVATE ${COASTLINE_WARNINGS})
string(TIMESTAMP _build_timestamp "%Y-%m-%dT%H:%M:%SZ" UTC)
target_compile_definitions(coastline_firstrun PRIVATE COASTLINE_VERSION="${CMAKE_PROJECT_VERSION}"
                                                   COASTLINE_BUILD_TIMESTAMP="${_build_timestamp}")
target_link_libraries(coastline_firstrun PUBLIC coastline_sdl Threads::Threads PRIVATE ZLIB::ZLIB)
