enable_language(RC)

add_executable(coastline_brally WIN32 brally.c brally.rc ini.c)
set_target_properties(coastline_brally PROPERTIES OUTPUT_NAME BRally C_STANDARD 23)
target_compile_options(coastline_brally PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_brally PRIVATE advapi32 kernel32 user32)
