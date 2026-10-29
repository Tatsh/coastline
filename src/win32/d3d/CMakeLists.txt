# The DirectDraw and Direct3D renderer of the Direct3D library.
add_library(
  coastline_win32_d3d STATIC
  ddutil.cpp
  devicecaps.cpp
  driver.cpp
  renderer.cpp
  texture.cpp)
target_link_libraries(coastline_win32_d3d PRIVATE coastline_options)
