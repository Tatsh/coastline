# The matrices and views of the graphics utilities.
add_library(coastline_common_libultra_gu STATIC)
if(NOT TGR_PLATFORM STREQUAL "n64")
  target_sources(coastline_common_libultra_gu PRIVATE lookat.c lookathil.c lookatref.c mtxcatf.c
                                                     mtxutil.c perspective.c rotate.c scale.c translate.c)
endif()
target_link_libraries(coastline_common_libultra_gu PRIVATE coastline_common_libultra_options)
