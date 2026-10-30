# libultra as the game links it. Each directory of the SDK builds its own archive from its own list.
# A routine joins its list when its body moves from the assembly of the original into C. The SDK's
# file names are retained.
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The PC port links only the routines every release shares, in src/common/libultra.
  set(COASTLINE_LIBULTRA_LIBRARIES "" PARENT_SCOPE)
  return()
endif()

add_library(coastline_libultra_options INTERFACE)
target_link_libraries(coastline_libultra_options INTERFACE coastline_options)
if(NOT TGR_IDO)
  # These routines are the C library, so GCC must not turn their loops into calls to memcpy or
  # memset.
  target_compile_options(coastline_libultra_options INTERFACE -fno-tree-loop-distribute-patterns)
  # Data the SDK initialises, to zero included, belongs in the data, as the BSS may not grow.
  target_compile_options(coastline_libultra_options INTERFACE -fno-zero-initialized-in-bss)
endif()

add_subdirectory(gu)
add_subdirectory(io)
add_subdirectory(libc)
add_subdirectory(os)
set(COASTLINE_LIBULTRA_LIBRARIES coastline_libultra_gu coastline_libultra_io coastline_libultra_libc
                                 coastline_libultra_os PARENT_SCOPE)
