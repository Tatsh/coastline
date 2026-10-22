add_executable(encode-text main.cpp encoder.cpp)
target_link_libraries(encode-text PRIVATE argparse::argparse coastline_tools_common spdlog::spdlog)
