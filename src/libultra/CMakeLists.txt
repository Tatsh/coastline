# This target builds libultra as the game links it. Each directory of the SDK lists its sources in
# its own file. A routine joins its list when its body moves from the assembly of the original into
# C. The SDK's file names are retained.
add_library(coastline_libultra STATIC)
add_subdirectory(gu)
add_subdirectory(io)
add_subdirectory(libc)
add_subdirectory(os)
target_link_libraries(coastline_libultra PRIVATE coastline_common)
# These routines are the C library, so GCC must not turn their loops into calls to memcpy or memset.
target_compile_options(coastline_libultra PRIVATE -fno-tree-loop-distribute-patterns)
