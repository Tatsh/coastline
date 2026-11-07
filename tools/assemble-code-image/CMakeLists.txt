add_executable(assemble-code-image main.cpp)
target_link_libraries(assemble-code-image PRIVATE argparse::argparse coastline_tools_common
                                                  nlohmann_json::nlohmann_json)
