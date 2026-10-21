add_executable(build-rom main.cpp rombuilder.cpp)
target_link_libraries(build-rom PRIVATE argparse::argparse coastline_tools_common spdlog::spdlog)
