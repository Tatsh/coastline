# The operating system: threads, messages, interrupts and exceptions, timers, and the caches. The
# messages and the debug output every release links are in src/common/libultra/os.
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The count of the PC port, from the clock of the system.
  add_library(coastline_libultra_os STATIC getcount.c)
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
          resetglobalintmask.c
          setcompare.s
          seteventmesg.c
          setfpccsr.s
          setglobalintmask.c
          setintmask.s
          setsr.s
          setthreadpri.c
          settimer.c
          startthread.c
          thread.c
          timerintr.c
          virtualtophysical.c
          writebackdcache.s
          writebackdcacheall.s
          yieldthread.c)
target_link_libraries(coastline_libultra_os PRIVATE coastline_libultra_options)
