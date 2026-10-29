# The flags of IDO for each source, from tools/ido_flags.txt: the first line whose glob matches the
# path of the source from the root of the tree applies, with its first alternative.
file(STRINGS "${CMAKE_SOURCE_DIR}/tools/ido_flags.txt" _ido_lines REGEX "^[^#].*[^ ]")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                                       "${CMAKE_SOURCE_DIR}/tools/ido_flags.txt")

function(coastline_apply_ido_flags target)
  get_target_property(_sources ${target} SOURCES)
  get_target_property(_directory ${target} SOURCE_DIR)
  foreach(_source ${_sources})
    if(NOT _source MATCHES "\\.c$")
      continue()
    endif()
    cmake_path(ABSOLUTE_PATH _source BASE_DIRECTORY "${_directory}" OUTPUT_VARIABLE _path)
    cmake_path(RELATIVE_PATH _path BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE _relative)
    # A source no line matches compiles at -O2, the level of the game.
    set(_flags -O2)
    foreach(_line ${_ido_lines})
      string(REGEX MATCH "^([^ ]+)[ ]+([^|]+)" _parts "${_line}")
      set(_glob "${CMAKE_MATCH_1}")
      string(STRIP "${CMAKE_MATCH_2}" _line_flags)
      # A glob of fnmatch, as matchscan.py reads it: * matches any characters, / included.
      string(REGEX REPLACE "([.+^$()])" "\\\\\\1" _pattern "${_glob}")
      string(REPLACE "*" ".*" _pattern "${_pattern}")
      string(REPLACE "?" "." _pattern "${_pattern}")
      if(_relative MATCHES "^${_pattern}$")
        separate_arguments(_flags UNIX_COMMAND "${_line_flags}")
        break()
      endif()
    endforeach()
    # MIPS II unless the flags give the level.
    if(NOT _flags MATCHES "-mips")
      list(APPEND _flags -mips2)
    endif()
    set_source_files_properties("${_path}" TARGET_DIRECTORY ${target} PROPERTIES COMPILE_OPTIONS
                                                                                 "${_flags}")
  endforeach()
endfunction()
