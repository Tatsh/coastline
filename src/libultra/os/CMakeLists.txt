# The operating system: threads, messages, interrupts and exceptions, timers, and the caches.
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The messages, the count, and the debug output the PC port retained.
  add_library(coastline_libultra_os STATIC getcount.c recvmesg.c sendmesg.c syncprintf.c)
  target_link_libraries(coastline_libultra_os PRIVATE coastline_options)
  return()
endif()
add_library(
  coastline_libultra_os STATIC
          createmesgqueue.c
          createthread.c
          destroythread.c
          exceptasm.s
          getcount.s
          getcurrfaultedthread.c
          getthreadpri.c
          gettime.c
          initialize.c
          interrupt.s
          invaldcache.s
          invalicache.s
          jammesg.c
          maptlbrdb.s
          probetlb.s
          recvmesg.c
          resetglobalintmask.c
          sendmesg.c
          setcompare.s
          seteventmesg.c
          setfpccsr.s
          setglobalintmask.c
          setintmask.s
          setsr.s
          setthreadpri.c
          settimer.c
          startthread.c
          syncprintf.c
          thread.c
          timerintr.c
          virtualtophysical.c
          writebackdcache.s
          writebackdcacheall.s
          yieldthread.c)
target_link_libraries(coastline_libultra_os PRIVATE coastline_libultra_options)
