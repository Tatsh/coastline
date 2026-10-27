enable_language(RC)

add_executable(coastline_brally WIN32 brally.c brally.rc ini.c)
set_target_properties(coastline_brally PROPERTIES OUTPUT_NAME BRally C_STANDARD 23)
target_compile_options(coastline_brally PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_brally PRIVATE advapi32 kernel32 user32)

add_executable(coastline_bossrally WIN32 bossrally.c bossrally.rc)
set_target_properties(coastline_bossrally PROPERTIES OUTPUT_NAME BossRally C_STANDARD 23)
target_compile_options(coastline_bossrally PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(coastline_bossrally PRIVATE gdi32 kernel32 ole32 strmiids user32)
