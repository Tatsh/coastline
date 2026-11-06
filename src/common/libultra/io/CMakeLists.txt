# The reading of the controllers.
add_library(coastline_common_libultra_io STATIC)
if(NOT TGR_PLATFORM STREQUAL "n64")
  target_sources(coastline_common_libultra_io PRIVATE contreaddata.c controller.c)
endif()
target_link_libraries(coastline_common_libultra_io PRIVATE coastline_common_libultra_options)
