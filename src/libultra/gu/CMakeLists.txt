# The graphics utilities: matrices, views, and the trigonometry they use.
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The matrices and views the PC port retained.
  add_library(
    coastline_libultra_gu STATIC
    lookat.c
    lookathil.c
    lookatref.c
    mtxcatf.c
    mtxutil.c
    perspective.c
    rotate.c
    scale.c
    translate.c)
  target_link_libraries(coastline_libultra_gu PRIVATE coastline_options)
  return()
endif()
add_library(
  coastline_libultra_gu STATIC
          cosf.c
          lookat.c
          lookathil.c
          lookatref.c
          mtxcatf.c
          mtxcatl.c
          mtxutil.c
          normalize.c
          perspective.c
          rotate.c
          scale.c
          sinf.c
          sins.c
          sqrtf.c
          translate.c)
target_link_libraries(coastline_libultra_gu PRIVATE coastline_libultra_options)
