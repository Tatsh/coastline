add_library(coastline_tools_common STATIC assetchain.cpp fileio.cpp logging.cpp sha1.cpp)
target_include_directories(coastline_tools_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(coastline_tools_common PUBLIC spdlog::spdlog ZLIB::ZLIB)
