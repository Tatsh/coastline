# The C library: memory and string routines, formatted output, and 64-bit arithmetic.
add_library(
  coastline_libultra_libc STATIC
          bcmp.s
          bcopy.s
          bzero.s
          # The 64-bit division GCC calls, which code IDO compiles calls in ll.c instead.
          $<$<NOT:$<BOOL:${TGR_IDO}>>:divdi3.c>
          ldiv.c
          ll.c
          llcvt.c
          sprintf.c
          string.c
          xldtob.c
          xlitob.c
          xprintf.c)
target_link_libraries(coastline_libultra_libc PRIVATE coastline_libultra_options)
