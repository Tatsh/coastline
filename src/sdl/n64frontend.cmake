# The front end of the Nintendo 64 release, which takes the place of the menus of the PC release in
# the SDL game. Its sources compile with the types of the Nintendo 64 release, as C++ for the
# 32-bit offsets of Ptr32. The library is not linked yet: it still lacks routines of other units and
# the calls of the Controller Pak.
set(_src "${CMAKE_SOURCE_DIR}/src")
set(COASTLINE_N64_FRONTEND_SOURCES
    n64/controllerscreen.c n64/carselect.c n64/debugtext.c n64/decaldraw.c n64/decaleditor.c
    n64/input.c n64/model.c
    n64/mainmenu.c n64/options.c n64/paintshop.c n64/pakmenu.c n64/playerselect.c n64/race.c
    n64/racemenu.c
    n64/splash.c n64/sprite.c n64/text.c n64/texrect.c n64/timing.c n64/vehicle.c
    n64/vehiclerender.c n64/vehiclesetup.c n64/weatherselect.c common/driving.c common/fade.c common/game.c common/heap.c
    common/vehiclereset.c
    # The shared units that work on the vehicles, the players, and their profiles, whose types
    # differ between the releases.
    common/camerachase.c common/camerarender.c common/controllerread.c common/framebegin.c common/frametime.c common/inputtranslate.c
    common/modelanimation.c common/modelanimationflags.c common/modeldraw.c
    common/playercontrol.c
    common/playerinput.c
    common/profileinit.c common/replay.c
    common/rigidbody.c common/routeposition.c common/seasontrack.c common/textrender.c common/textstyle.c common/tracetrack.c common/vehiclechoice.c
    common/vehiclemodel.c
    common/vehiclemodelrender.c common/vehiclepaint.c common/vehiclephysics.c common/vehiclevisibility.c)
list(TRANSFORM COASTLINE_N64_FRONTEND_SOURCES PREPEND "${_src}/")
set_source_files_properties(${COASTLINE_N64_FRONTEND_SOURCES} PROPERTIES LANGUAGE CXX)

# n64frontenddata.cpp is written by tools/sdl/n64frontenddata.py from the symbols the front end
# needs, with the extents of tools/sdl/n64frontend-extents.txt.
add_library(coastline_n64frontend STATIC ${COASTLINE_N64_FRONTEND_SOURCES} n64frontenddata.cpp
                                           n64frontendroutines.cpp n64model.cpp n64racescreen.cpp
                                           n64system.cpp)
target_include_directories(coastline_n64frontend PRIVATE "${_src}" "${_src}/n64"
                                                         "${_src}/libultra/include")
# The symbols both releases define, and the data whose types differ between them, are the front
# end's own, under other names.
file(STRINGS "${CMAKE_SOURCE_DIR}/tools/sdl/n64frontend-renames.txt" _n64_renames REGEX "^[^#]")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                                       "${CMAKE_SOURCE_DIR}/tools/sdl/n64frontend-renames.txt")
list(TRANSFORM _n64_renames REPLACE "^(.+)$" "-D\\1=\\1N64")
# The channels of the sound effects of the console, which src/audio.c names as it does here, beside
# the channels of the same names of the PC release.
list(APPEND _n64_renames -DInitSoundChannel=N64InitSoundChannel
     -DStartSoundChannelMuted=N64StartSoundChannelMuted)
target_compile_options(
  coastline_n64frontend
  PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-x;c++>" -UTGR_BOSS_RALLY -DTGR_BOSS_RALLY=0 -UTGR_BRD3D -DTGR_BRD3D=0
          -fpermissive -Wno-unused-parameter ${_n64_renames})
target_link_libraries(coastline_n64frontend PUBLIC SDL3::SDL3 coastline_cartridgedata coastline_pcgame
                                                    coastline_sdl)

# The data of the front end is read from the cartridge at start-up, by the symbols of splat.
set(_symbols_header "${CMAKE_CURRENT_BINARY_DIR}/generated/symbol_addrs_us.h")
add_custom_command(
  OUTPUT "${_symbols_header}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated"
  COMMAND "${CMAKE_COMMAND}" -DINPUT=${CMAKE_SOURCE_DIR}/tools/splat/symbol_addrs.us.txt
          -DOUTPUT=${_symbols_header} -DNAME=SYMBOL_ADDRS_US -P "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
  DEPENDS "${CMAKE_SOURCE_DIR}/tools/splat/symbol_addrs.us.txt" "${CMAKE_SOURCE_DIR}/cmake/embed.cmake"
  COMMENT "Embedding the symbols of the USA release"
  VERBATIM)
find_package(Boost CONFIG REQUIRED COMPONENTS pfr)
add_library(coastline_cartridgedata STATIC cartridgedata.cpp "${_symbols_header}")
target_include_directories(coastline_cartridgedata PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${_src}"
                           PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(coastline_cartridgedata PUBLIC Boost::pfr SDL3::SDL3)
