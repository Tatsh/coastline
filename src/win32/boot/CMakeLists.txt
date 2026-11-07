enable_language(RC)

add_executable(coastline_brally WIN32 brally.c brally.rc ini.c registry.c
               "${CMAKE_SOURCE_DIR}/src/common/fileio.c" ../diagnostics.c)
set_target_properties(coastline_brally PROPERTIES OUTPUT_NAME BRally C_STANDARD 23)
if(NOT MSVC)
  target_compile_options(coastline_brally PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
target_include_directories(coastline_brally PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(coastline_brally PRIVATE advapi32 kernel32 user32)

set(_setvideo_resource setvideo.rc)
if(MSVC)
  find_program(TGR_WINDRES windres REQUIRED)
  set(_setvideo_resource "${CMAKE_CURRENT_BINARY_DIR}/setvideo.res")
  add_custom_command(
    OUTPUT "${_setvideo_resource}"
    COMMAND "${TGR_WINDRES}" -O res -i "${CMAKE_CURRENT_SOURCE_DIR}/setvideo.rc" -o
            "${_setvideo_resource}"
    DEPENDS setvideo.rc
    VERBATIM)
endif()
add_executable(coastline_setvideo WIN32 setvideo.c "${_setvideo_resource}" ini.c registry.c filewrite.c
                                         "${CMAKE_SOURCE_DIR}/src/common/fileio.c")
set_target_properties(coastline_setvideo PROPERTIES OUTPUT_NAME SetVideo C_STANDARD 23)
if(NOT MSVC)
  target_compile_options(coastline_setvideo PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
target_include_directories(coastline_setvideo PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(coastline_setvideo PRIVATE advapi32 kernel32 user32)

add_executable(coastline_bossrally WIN32 bossrally.cpp bossrally.rc)
set_target_properties(coastline_bossrally PROPERTIES OUTPUT_NAME BossRally CXX_STANDARD 23)
if(NOT MSVC)
  target_compile_options(coastline_bossrally PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
target_link_libraries(coastline_bossrally PRIVATE gdi32 kernel32 ole32 strmiids user32)

add_library(coastline_boot_helpers STATIC installer.c)
set_target_properties(coastline_boot_helpers PROPERTIES C_STANDARD 23)
if(NOT MSVC)
  target_compile_options(coastline_boot_helpers PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
target_link_libraries(coastline_boot_helpers PRIVATE kernel32 user32)

option(TGR_BUILD_MFC_BOOT "Build Boot.exe with a compatible native MFC SDK." OFF)
if(TGR_BUILD_MFC_BOOT)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Boot.exe requires a 32-bit compatible MFC SDK.")
  endif()
  set(CMAKE_MFC_FLAG 1)
  add_executable(coastline_boot WIN32 bootapp.cpp bootdlg.cpp boot.rc)
  set_target_properties(coastline_boot PROPERTIES OUTPUT_NAME Boot CXX_STANDARD 23)
  target_compile_definitions(coastline_boot PRIVATE _MBCS)
  target_link_libraries(coastline_boot PRIVATE coastline_boot_helpers advapi32)
endif()
