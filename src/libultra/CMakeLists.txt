# libultra as the game links it. Each directory of the SDK builds its own archive from its own list.
# A routine joins its list when its body moves from the assembly of the original into C. The SDK's
# file names are retained.
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The PC port retained a few routines of the graphics utilities, the controllers, and the
  # operating system, and none of the C library.
  add_subdirectory(gu)
  add_subdirectory(io)
  add_subdirectory(os)
  set(COASTLINE_LIBULTRA_LIBRARIES coastline_libultra_gu coastline_libultra_io coastline_libultra_os
      PARENT_SCOPE)
  return()
endif()

add_library(coastline_libultra_options INTERFACE)
target_link_libraries(coastline_libultra_options INTERFACE coastline_options)
# These routines are the C library, so GCC must not turn their loops into calls to memcpy or memset.
target_compile_options(coastline_libultra_options INTERFACE -fno-tree-loop-distribute-patterns)
# Data the SDK initialises, to zero included, belongs in the data, as the BSS may not grow.
target_compile_options(coastline_libultra_options INTERFACE -fno-zero-initialized-in-bss)

add_subdirectory(gu)
add_subdirectory(io)
add_subdirectory(libc)
add_subdirectory(os)
set(COASTLINE_LIBULTRA_LIBRARIES coastline_libultra_gu coastline_libultra_io coastline_libultra_libc
                                 coastline_libultra_os PARENT_SCOPE)
