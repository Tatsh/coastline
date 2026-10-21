# Each subsystem directory builds its static library and lists its sources explicitly. A
# recursive glob would hide what is built. A source added to the tree would join the build with no
# record of the decision, and a source removed would disappear the same way.
#
# Static libraries are deliberate. Archiving produces the object files and the archives without
# linking a ROM. The reconstruction is partial, and no link can succeed until every routine a call
# arrives at has a body.

# Everything the subsystem libraries share. An interface library provides the settings without
# compiling anything itself.
add_library(coastline_common INTERFACE)
# A reconstructed routine retains the parameters of the original, used or not.
target_compile_options(coastline_common INTERFACE -Wall -Wextra -Wno-unused-parameter)
target_include_directories(coastline_common INTERFACE "${CMAKE_SOURCE_DIR}/src")
if(TGR_PLATFORM STREQUAL "n64")
  target_include_directories(coastline_common INTERFACE "${CMAKE_SOURCE_DIR}/src/libultra/include")
endif()

# Subsystems join this list as their directories are reconstructed.
set(COASTLINE_SUBSYSTEMS)

foreach(SUBSYSTEM ${COASTLINE_SUBSYSTEMS})
  add_subdirectory(${SUBSYSTEM})
endforeach()

# The entry points sit at the root of the tree rather than in a subsystem. A routine that differs
# substantially between targets has one translation unit per target, suffixed with the target, and
# the build selects the unit. A routine whose difference is a few lines, DllMain() for one, stays
# in one unit. The Windows port is C++ and exports RallyMain from each renderer's library. The
# Nintendo 64 entry is C.
if(TGR_PLATFORM STREQUAL "win")
  enable_language(CXX)
  add_library(coastline_main OBJECT dllmain.c winmain-${TGR_RENDERER}.cpp)
else()
  add_library(coastline_main OBJECT main.c)
endif()
target_link_libraries(coastline_main PRIVATE coastline_common)

# Every archive the build produces, for a target that builds all of them at once.
set(COASTLINE_LIBRARIES coastline_main)
foreach(SUBSYSTEM ${COASTLINE_SUBSYSTEMS})
  list(APPEND COASTLINE_LIBRARIES coastline_${SUBSYSTEM})
endforeach()

if(TGR_PLATFORM STREQUAL "n64")
  # The data and BSS of the original image, exported from Ghidra as assembly, until every datum is
  # converted to C. A release without an export builds its code alone.
  set(_data "${CMAKE_CURRENT_SOURCE_DIR}/platform/n64/data-${TGR_REGION}.s")
  if(EXISTS "${_data}")
    add_library(coastline_data OBJECT "${_data}")
    list(APPEND COASTLINE_LIBRARIES coastline_data)
  endif()

  # The inflate side of the vendored zlib 1.0.4 that the game links. The warnings concern upstream
  # style.
  set(_zlib "${CMAKE_SOURCE_DIR}/3rdparty/zlib-1.0.4")
  add_library(
    coastline_zlib STATIC
    "${_zlib}/adler32.c" "${_zlib}/infblock.c" "${_zlib}/infcodes.c" "${_zlib}/inffast.c"
    "${_zlib}/inflate.c" "${_zlib}/inftrees.c" "${_zlib}/infutil.c" "${_zlib}/zutil.c")
  target_include_directories(coastline_zlib PUBLIC "${_zlib}")
  target_compile_options(coastline_zlib PRIVATE -w)
  target_link_libraries(coastline_common INTERFACE coastline_zlib)
  list(APPEND COASTLINE_LIBRARIES coastline_zlib)
endif()

add_custom_target(archives DEPENDS ${COASTLINE_LIBRARIES})

# The final link cannot succeed until every routine has a body. The archives are the check until
# then, and TGR_LINK turns the link on.
if(TGR_LINK)
  if(TGR_PLATFORM STREQUAL "win")
    if(TGR_RENDERER STREQUAL "d3d")
      set(_library BRD3D)
    else()
      set(_library BRGlide)
    endif()
    add_library(${_library} SHARED $<TARGET_OBJECTS:coastline_main>)
    set_target_properties(${_library} PROPERTIES PREFIX "")
    target_link_libraries(${_library} PRIVATE ${COASTLINE_LIBRARIES} ole32 user32)
  else()
    # The link script pins the original data at its address, and the program is flattened into the
    # bytes the IPL3 loads. build-rom puts them after the header and the IPL3 of the original and
    # the assets at their offsets, and writes the checksum.
    if(NOT COASTLINE_ASSETS_DIR)
      message(FATAL_ERROR "TGR_LINK needs the assets of the original cartridge. Set TGR_ROM.")
    endif()
    set(_link_script "${CMAKE_CURRENT_SOURCE_DIR}/platform/n64/coastline-${TGR_REGION}.ld")
    add_executable(${CMAKE_PROJECT_NAME} $<TARGET_OBJECTS:coastline_main>)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${COASTLINE_LIBRARIES})
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -nostdlib -nostartfiles "-T${_link_script}")
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS "${_link_script}")
    set(_program "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin")
    set(_rom "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${TGR_REGION}.z64")
    add_custom_command(
      OUTPUT "${_program}"
      COMMAND "${CMAKE_OBJCOPY}" -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> "${_program}"
      DEPENDS ${CMAKE_PROJECT_NAME}
      COMMENT "Flattening the program"
      VERBATIM)
    add_custom_command(
      OUTPUT "${_rom}"
      COMMAND "${COASTLINE_TOOL_BUILD_ROM}" "${COASTLINE_ASSETS_DIR}/boot.bin" "${_program}"
              "${COASTLINE_ASSETS_DIR}" "${_rom}"
      DEPENDS "${_program}" "${COASTLINE_ASSETS_DIR}/manifest.txt" "${COASTLINE_TOOL_BUILD_ROM}"
              coastline_tools
      COMMENT "Building ${_rom}"
      VERBATIM)
    add_custom_target(rom ALL DEPENDS "${_rom}")
  endif()
endif()
