# The reading of the controllers.
add_library(coastline_common_libultra_io STATIC contreaddata.c controller.c)
target_link_libraries(coastline_common_libultra_io PRIVATE coastline_common_libultra_options)
