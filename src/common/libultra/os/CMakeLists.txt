# The messages and the debug output of the operating system.
add_library(coastline_common_libultra_os STATIC recvmesg.c sendmesg.c syncprintf.c)
target_link_libraries(coastline_common_libultra_os PRIVATE coastline_common_libultra_options)
