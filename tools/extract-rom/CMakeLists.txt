add_executable(extract-rom assets.cpp main.cpp rom.cpp)
target_link_libraries(extract-rom PRIVATE argparse::argparse coastline_tools_common spdlog::spdlog
                                          ZLIB::ZLIB)
