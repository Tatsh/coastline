# The C library: memory and string routines, formatted output, and 64-bit arithmetic.
target_sources(
  coastline_libultra
  PRIVATE bcmp.c
          bcopy.c
          bzero.c
          divdi3.c
          ldiv.c
          ll.c
          llcvt.c
          sprintf.c
          string.c
          xldtob.c
          xlitob.c
          xprintf.c)
