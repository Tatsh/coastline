# The messages and the debug output of the operating system.
add_library(coastline_common_libultra_os STATIC syncprintf.c)
target_link_libraries(coastline_common_libultra_os PRIVATE coastline_common_libultra_options)
if(TGR_PLATFORM STREQUAL "n64")
  target_sources(coastline_common_libultra_os PRIVATE "${CMAKE_SOURCE_DIR}/src/n64/libultra/os/recvmesg.c"
                                                     "${CMAKE_SOURCE_DIR}/src/n64/libultra/os/sendmesg.c")
else()
  # The count of the PC port, from the clock the game emulates.
  target_sources(coastline_common_libultra_os PRIVATE getcount.c recvmesg.c sendmesg.c)
endif()
