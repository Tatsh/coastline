# libultra as the game links it. A routine joins this list when its body moves from the assembly
# of the original into C. The SDK's file names are retained.
add_library(
  coastline_libultra STATIC
  os/createmesgqueue.c
  os/createthread.c
  os/destroythread.c
  os/getthreadpri.c
  os/jammesg.c
  os/recvmesg.c
  os/sendmesg.c
  os/seteventmesg.c
  os/setthreadpri.c
  os/startthread.c
  os/yieldthread.c)
target_link_libraries(coastline_libultra PRIVATE coastline_common)
