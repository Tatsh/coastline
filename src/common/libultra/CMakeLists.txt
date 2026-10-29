# The routines of libultra every release links: the matrices and views of the graphics utilities,
# the reading of the controllers, and the messages and debug output of the operating system. The
# SDK's file names are retained.
add_library(coastline_common_libultra_options INTERFACE)
if(TGR_PLATFORM STREQUAL "n64")
  # The Nintendo 64 releases compile them as the rest of libultra.
  target_link_libraries(coastline_common_libultra_options INTERFACE coastline_libultra_options)
else()
  target_link_libraries(coastline_common_libultra_options INTERFACE coastline_options)
endif()

add_subdirectory(gu)
add_subdirectory(io)
add_subdirectory(os)
set(COASTLINE_COMMON_LIBULTRA_LIBRARIES coastline_common_libultra_gu coastline_common_libultra_io
                                        coastline_common_libultra_os PARENT_SCOPE)
