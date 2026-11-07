# The code of the Nintendo 64 releases alone. A routine joins this list when its body moves from the
# assembly of the original into C.
set(_n64_sources
    audiomix.s
    balance.c
    billboard-particles.c
    camera.c
    carselect.c
    collision-triangles.c
    collidevehicles.c
    controllerscreen.c
    dashboard.c
    debugtext.c
    decaldraw.c
    decaleditor.c
    decompress.c
    depth.c
    depth-stub.c
    drivewheel.c
    effects.c
    frame.c
    ground-ray.c
    headlight-beams.c
    hud.c
    impulse.c
    input.c
    input-init.c
    inputtranslate.c
    mainmenu.c
    modeldraw.c
    obb.c
    options.c
    options-pak.c
    options-volume.c
    paintshop.c
    pakmenu.c
    physics.c
    playercontrol.c
    playerselect.c
    precipitation-update.c
    precipitationrender.c
    race.c
    race-bgm.c
    race-demo.c
    racer-slot-motion.c
    racersharing.c
    racetick.c
    raceprogress.c
    racemenu.c
    racemenu-draw.c
    racesound.c
    rumble.c
    sidegrip.c
    sky.c
    snapshot.c
    splash.c
    splash-controller.c
    sprite.c
    system.c
    boot-object.c
    texrect.c
    text.c
    tirespray.c
    timing.c
    trackinstance.c
    trackrender.c
    vehicle.c
    vehicleengine.c
    vehicle-mesh-deform.c
    vehicle-physics-step.c
    vehicle-track-position.c
    vehiclemodelrender.c
    vehiclerender.c
    vehiclesetup.c
    vehiclesound.c
    visibleinstances.c
    weather.c
    weatherselect.c)
if(TGR_ENABLE_PATCHES)
  list(APPEND _n64_sources paintinput.c)
endif()
if(TGR_IDO AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT TGR_ENABLE_PATCHES)
  list(REMOVE_ITEM _n64_sources depth-stub.c)
  coastline_add_ido_trimmed_object(coastline_depth_stub depth-stub.c 0x10 0x8 depth-stub)
  list(REMOVE_ITEM _n64_sources input-init.c)
  coastline_add_ido_trimmed_object(coastline_input_init input-init.c 0x2f0 0x2e4 input-init)
  list(REMOVE_ITEM _n64_sources depth.c)
  coastline_add_ido_trimmed_object(coastline_depth depth.c 0x330 0x32c depth)
  list(REMOVE_ITEM _n64_sources frame.c)
  if(TGR_REGION STREQUAL "eu")
    coastline_add_ido_trimmed_object(coastline_frame frame.c 0x2b80 0x2b78 frame)
  elseif(TGR_REGION STREQUAL "jp")
    coastline_add_ido_trimmed_object(coastline_frame frame.c 0x2b00 0x2af8 frame)
  else()
    coastline_add_ido_trimmed_object(coastline_frame frame.c 0x29c0 0x29b8 frame)
  endif()
  list(REMOVE_ITEM _n64_sources options-volume.c)
  coastline_add_ido_trimmed_object(coastline_options_volume options-volume.c 0x50 0x44 options-volume)
  list(REMOVE_ITEM _n64_sources modeldraw.c)
  coastline_add_ido_trimmed_object(coastline_modeldraw modeldraw.c 0x7c0 0x7b4 modeldraw)
  list(REMOVE_ITEM _n64_sources race-bgm.c)
  coastline_add_ido_trimmed_object(coastline_race_bgm race-bgm.c 0x60 0x5c race-bgm)
  if(TGR_REGION STREQUAL "jp")
    list(REMOVE_ITEM _n64_sources racetick.c race.c)
    coastline_add_ido_trimmed_object(coastline_racetick racetick.c 0x5a60 0x5a54 racetick)
    coastline_add_ido_trimmed_object(coastline_race race.c 0x460 0x458 race)
  endif()
endif()
if(TGR_REGION STREQUAL "eu" OR TGR_REGION STREQUAL "jp")
  list(APPEND _n64_sources options-regional.c)
endif()
if(TGR_IDO AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT TGR_ENABLE_PATCHES)
  if(TGR_REGION STREQUAL "asia" OR TGR_REGION STREQUAL "eu")
    list(REMOVE_ITEM _n64_sources options.c)
    if(TGR_REGION STREQUAL "asia")
      coastline_add_ido_trimmed_object(coastline_options_trimmed options.c 0x30a0 0x309c options)
    else()
      coastline_add_ido_trimmed_object(coastline_options_trimmed options.c 0x9b0 0x9ac options)
    endif()
    if(TGR_REGION STREQUAL "asia")
      list(REMOVE_ITEM _n64_sources options-pak.c)
      coastline_add_ido_trimmed_object(coastline_options_pak_trimmed options-pak.c 0x1da0 0x1d98
                                      options-pak)
    endif()
  elseif(TGR_REGION STREQUAL "jp")
    list(REMOVE_ITEM _n64_sources options-regional.c)
    coastline_add_ido_trimmed_object(coastline_options_regional_trimmed options-regional.c 0x3580
                                    0x3574 options-regional)
  endif()
endif()
# These sources write the Japanese text in UTF-8. The Japanese release compiles copies with the text
# in the glyph codes of its font.
if(TGR_REGION STREQUAL "jp")
  foreach(_source carselect.c decaleditor.c options.c options-pak.c pakmenu.c paintshop.c
                  playerselect.c splash.c splash-controller.c text.c)
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
if(TGR_REGION STREQUAL "jp" AND TGR_IDO AND CMAKE_BUILD_TYPE STREQUAL "Release"
   AND NOT TGR_ENABLE_PATCHES)
  set(_encoded_options "${CMAKE_CURRENT_BINARY_DIR}/encoded/options.c")
  list(REMOVE_ITEM _n64_sources "${_encoded_options}")
  coastline_add_ido_trimmed_object(coastline_options_trimmed "${_encoded_options}" 0x910 0x90c
                                  options)
endif()
if(TGR_IDO)
  if(TGR_REGION STREQUAL "jp")
    set(_splash_source "${CMAKE_CURRENT_BINARY_DIR}/encoded/splash.c")
    set(_paintshop_source "${CMAKE_CURRENT_BINARY_DIR}/encoded/paintshop.c")
  else()
    set(_splash_source splash.c)
    set(_paintshop_source paintshop.c)
  endif()
  set_source_files_properties("${_splash_source}" PROPERTIES
                            COMPILE_DEFINITIONS COASTLINE_ROM_POINTERS)
  set_source_files_properties(racemenu.c racemenu-draw.c PROPERTIES
                              COMPILE_DEFINITIONS COASTLINE_ROM_POINTERS)
  set_source_files_properties("${_paintshop_source}" PROPERTIES
                            COMPILE_DEFINITIONS COASTLINE_PAINTSHOP_COLOR_RETURN)
endif()
add_library(coastline_n64 STATIC ${_n64_sources})
target_link_libraries(coastline_n64 PRIVATE coastline_options)

# The entry point, which the link script places first.
set_source_files_properties(main.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
add_library(coastline_main OBJECT main.s)
target_link_libraries(coastline_main PRIVATE coastline_options)
if(NOT TGR_REGION STREQUAL "us")
  set_source_files_properties(game-mode-main-loop.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
  target_sources(coastline_main PRIVATE game-mode-main-loop.s)
endif()
