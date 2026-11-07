add_executable(check-matching main.cpp)
target_link_libraries(check-matching PRIVATE argparse::argparse coastline_tools_common
                                             nlohmann_json::nlohmann_json)
