# Extracts the assets of the original cartridge into ${CMAKE_BINARY_DIR}/assets with extract-rom.
# The ROM is required to build a ROM, and the extraction runs as part of every build. Without a
# ROM, the sources still compile.

set(TGR_ROM
    ""
    CACHE FILEPATH "Original cartridge image of the TGR_REGION release, in .z64, .v64, or .n64 \
order. Its assets are extracted into the build directory.")

if(NOT TGR_ROM)
  message(STATUS "TGR_ROM is not set. The assets are not extracted, and no ROM can be built.")
  return()
endif()

set(COASTLINE_ASSETS_DIR "${CMAKE_BINARY_DIR}/assets")
add_custom_command(
  OUTPUT "${COASTLINE_ASSETS_DIR}/manifest.txt"
  COMMAND "${COASTLINE_TOOL_EXTRACT_ROM}" --region "${TGR_REGION}" "${TGR_ROM}"
          "${COASTLINE_ASSETS_DIR}"
  DEPENDS "${TGR_ROM}" "${COASTLINE_TOOL_EXTRACT_ROM}" coastline_tools
  COMMENT "Extracting the assets of ${TGR_ROM}"
  VERBATIM)
add_custom_target(assets ALL DEPENDS "${COASTLINE_ASSETS_DIR}/manifest.txt")
