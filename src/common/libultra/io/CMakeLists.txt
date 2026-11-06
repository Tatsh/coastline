# The reading of the controllers.
add_library(coastline_common_libultra_io STATIC)
if(TGR_PLATFORM STREQUAL "n64")
  target_sources(coastline_common_libultra_io PRIVATE "${CMAKE_SOURCE_DIR}/src/n64/libultra/io/contreaddata.c"
                                                     "${CMAKE_SOURCE_DIR}/src/n64/libultra/io/controller.c")
else()
  target_sources(coastline_common_libultra_io PRIVATE contreaddata.c controller.c)
endif()
target_link_libraries(coastline_common_libultra_io PRIVATE coastline_common_libultra_options)
