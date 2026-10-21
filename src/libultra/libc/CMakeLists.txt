# The C library: memory and string routines, formatted output, and 64-bit arithmetic.
add_library(
  coastline_libultra_libc STATIC
          bcmp.c
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
target_link_libraries(coastline_libultra_libc PRIVATE coastline_libultra_options)
