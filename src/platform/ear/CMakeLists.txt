add_library(coastline_ear STATIC backend.cpp distance.cpp environment.cpp error.cpp exports.def
                                memorybudget.cpp runtime.cpp version.cpp window.cpp)
target_include_directories(coastline_ear PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(coastline_ear PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_ear PRIVATE kernel32 user32 version)
