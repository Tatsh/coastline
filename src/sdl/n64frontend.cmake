# The front end of the Nintendo 64 release, which takes the place of the menus of the PC release in
# the SDL game. Its sources compile with the types of the Nintendo 64 release, as C++ for the
# 32-bit offsets of Ptr32. The library is not linked yet: its data is still only in the assembly of
# the cartridge.
set(_src "${CMAKE_SOURCE_DIR}/src")
set(COASTLINE_N64_FRONTEND_SOURCES
    controllerscreen.c carselect.c decaldraw.c decaleditor.c mainmenu.c options.c pakmenu.c
    playerselect.c racemenu.c splash.c sprite.c text.c texrect.c weatherselect.c)
list(TRANSFORM COASTLINE_N64_FRONTEND_SOURCES PREPEND "${_src}/n64/")
set_source_files_properties(${COASTLINE_N64_FRONTEND_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(coastline_n64frontend STATIC ${COASTLINE_N64_FRONTEND_SOURCES})
target_include_directories(coastline_n64frontend PRIVATE "${_src}" "${_src}/n64"
                                                         "${_src}/libultra/include")
# The data whose types differ between the two releases is the front end's own, under other names.
set(_n64_renames g_aPlayerInputState g_aVehicleModels g_aVehicles g_SeasonTable)
list(TRANSFORM _n64_renames REPLACE "^(.+)$" "-D\\1=\\1N64")
target_compile_options(
  coastline_n64frontend
  PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-x;c++>" -UTGR_PC -DTGR_PC=0 -UTGR_BRD3D -DTGR_BRD3D=0
          -fpermissive -Wno-unused-parameter ${_n64_renames})
target_link_libraries(coastline_n64frontend PUBLIC SDL3::SDL3)
