# The messages and the debug output of the operating system.
add_library(coastline_common_libultra_os STATIC recvmesg.c sendmesg.c syncprintf.c)
target_link_libraries(coastline_common_libultra_os PRIVATE coastline_common_libultra_options)
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The count of the PC port, from the clock the game emulates.
  target_sources(coastline_common_libultra_os PRIVATE getcount.c)
endif()
