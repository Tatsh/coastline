# The code of the Nintendo 64 releases alone. A routine joins this list when its body moves from the
# assembly of the original into C.
set(_n64_sources
    audiomix.s
    camera.c
    carselect.c
    controllerscreen.c
    debugtext.c
    decaldraw.c
    decaleditor.c
    effects.c
    hud.c
    input.c
    mainmenu.c
    model.c
    options.c
    paintshop.c
    pakmenu.c
    physics.c
    playerselect.c
    race.c
    racemenu.c
    racesound.c
    rumble.c
    snapshot.c
    splash.c
    sprite.c
    system.c
    texrect.c
    text.c
    timing.c
    track.c
    vehicle.c
    vehiclerender.c
    vehiclesetup.c
    weather.c
    weatherselect.c)
# These sources write the Japanese text in UTF-8. The Japanese release compiles copies with the text
# in the glyph codes of its font.
if(TGR_REGION STREQUAL "jp")
  foreach(_source carselect.c decaleditor.c options.c pakmenu.c paintshop.c playerselect.c splash.c
                  text.c)
    set(_encoded "${CMAKE_CURRENT_BINARY_DIR}/encoded/${_source}")
    add_custom_command(
      OUTPUT "${_encoded}"
      COMMAND "${COASTLINE_TOOL_ENCODE_TEXT}" "${CMAKE_CURRENT_SOURCE_DIR}/${_source}" "${_encoded}"
      DEPENDS "${_source}" "${COASTLINE_TOOL_ENCODE_TEXT}" coastline_tools
      COMMENT "Encoding the Japanese text of ${_source}"
      VERBATIM)
    list(TRANSFORM _n64_sources REPLACE "^${_source}$" "${_encoded}")
  endforeach()
endif()
add_library(coastline_n64 STATIC ${_n64_sources})
target_link_libraries(coastline_n64 PRIVATE coastline_options)

# The entry point, which the link script places first.
add_library(coastline_main OBJECT main.s)
target_link_libraries(coastline_main PRIVATE coastline_options)
