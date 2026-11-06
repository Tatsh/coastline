# The matrices and views of the graphics utilities.
add_library(
  coastline_common_libultra_gu STATIC
  lookathil.c
  lookatref.c
  mtxutil.c
  perspective.c
  scale.c
  translate.c)
if(TGR_PLATFORM STREQUAL "n64")
  target_sources(coastline_common_libultra_gu PRIVATE "${CMAKE_SOURCE_DIR}/src/n64/libultra/gu/lookat.c"
                                                      "${CMAKE_SOURCE_DIR}/src/n64/libultra/gu/mtxcatf.c"
                                                      "${CMAKE_SOURCE_DIR}/src/n64/libultra/gu/rotate.c")
else()
  target_sources(coastline_common_libultra_gu PRIVATE lookat.c mtxcatf.c rotate.c)
endif()
target_link_libraries(coastline_common_libultra_gu PRIVATE coastline_common_libultra_options)
