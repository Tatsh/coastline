# The trigonometry of the graphics utilities, and the matrices the PC port did not retain. The
# routines every release links are in src/common/libultra/gu.
add_library(
  coastline_libultra_gu STATIC
  cosf.c
  mtxcatl.c
  normalize.c
  sinf.c
  sins.c
  sqrtf.c)
target_link_libraries(coastline_libultra_gu PRIVATE coastline_libultra_options)
