add_executable(generate-code-relocations main.cpp)
target_link_libraries(generate-code-relocations PRIVATE argparse::argparse coastline_tools_common
                                                       nlohmann_json::nlohmann_json)
