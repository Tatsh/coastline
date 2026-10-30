# The libraries and tools the builds depend on. Each has a USE_SYSTEM_<NAME> option. With it on,
# the dependency must come from the system, and nothing is downloaded. With it off, the dependency
# comes from vcpkg when the vcpkg toolchain is in use, and otherwise from the pinned release, built
# with the project. A distribution package turns every option on.

include(FetchContent)

# Sets each <variable>=<value> of a list as a cache variable, the options of a pinned release.
function(_coastline_set_release_options)
  foreach(_option IN LISTS ARGN)
    string(FIND "${_option}" "=" _at)
    string(SUBSTRING "${_option}" 0 ${_at} _variable)
    math(EXPR _at "${_at} + 1")
    string(SUBSTRING "${_option}" ${_at} -1 _value)
    set(${_variable}
        "${_value}"
        CACHE INTERNAL "")
  endforeach()
endfunction()

# coastline_dependency(<name> PACKAGE <package> [FIND_ARGS <args>...] URL <url> HASH <hash>
#                      [OPTIONS <variable>=<value>...])
#
# Makes <package> available, as find_package(<package> <args>) finds it from the system or vcpkg,
# or from the pinned release at <url>. The OPTIONS set cache variables of the release before it
# is configured. <name> is the upper-case name of the USE_SYSTEM_ option.
function(coastline_dependency name)
  cmake_parse_arguments(PARSE_ARGV 1 _dependency "" "PACKAGE;URL;HASH" "FIND_ARGS;OPTIONS")
  option(USE_SYSTEM_${name} "Take ${_dependency_PACKAGE} from the system, without downloading it."
         OFF)
  if(USE_SYSTEM_${name} OR VCPKG_TOOLCHAIN)
    find_package(${_dependency_PACKAGE} ${_dependency_FIND_ARGS} REQUIRED)
    if(USE_SYSTEM_${name})
      message(STATUS "${_dependency_PACKAGE} comes from the system.")
    else()
      message(STATUS "${_dependency_PACKAGE} comes from vcpkg.")
    endif()
    return()
  endif()
  _coastline_set_release_options(${_dependency_OPTIONS})
  FetchContent_Declare(${_dependency_PACKAGE} URL "${_dependency_URL}" URL_HASH
                                                  "SHA256=${_dependency_HASH}" SYSTEM)
  FetchContent_MakeAvailable(${_dependency_PACKAGE})
  message(STATUS "${_dependency_PACKAGE} comes from the pinned release ${_dependency_URL}.")
endfunction()

# coastline_tool(<name> <variable> PROGRAM <program> [HINT <directory>] TARGET <target> URL <url>
#                HASH <hash> [OPTIONS <variable>=<value>...])
#
# vcpkg installs the program among the tools of its host triplet, and the tools of every installed
# triplet are searched besides <directory>.
#
# Sets <variable> to a program the build runs on the host: the system or vcpkg <program>, or the
# <target> of the pinned release at <url>, built with the project. A cross build cannot run a
# program it builds, and takes the program from the system or vcpkg.
function(coastline_tool name variable)
  cmake_parse_arguments(PARSE_ARGV 2 _tool "" "PROGRAM;HINT;TARGET;URL;HASH" "OPTIONS")
  option(USE_SYSTEM_${name} "Take ${_tool_PROGRAM} from the system, without downloading it." OFF)
  if(USE_SYSTEM_${name} OR VCPKG_TOOLCHAIN OR CMAKE_CROSSCOMPILING)
    set(_hints "${_tool_HINT}")
    if(VCPKG_INSTALLED_DIR)
      get_filename_component(_tool_directory "${_tool_HINT}" NAME)
      file(GLOB _triplet_tools LIST_DIRECTORIES true
           "${VCPKG_INSTALLED_DIR}/*/tools/${_tool_directory}")
      list(APPEND _hints ${_triplet_tools})
    endif()
    find_program(${variable} ${_tool_PROGRAM} HINTS ${_hints} REQUIRED)
    message(STATUS "${_tool_PROGRAM} is ${${variable}}.")
    return()
  endif()
  _coastline_set_release_options(${_tool_OPTIONS})
  string(TOLOWER "${name}" _content)
  FetchContent_Declare(${_content} URL "${_tool_URL}" URL_HASH "SHA256=${_tool_HASH}" SYSTEM)
  FetchContent_MakeAvailable(${_content})
  set(${variable}
      ${_tool_TARGET}
      PARENT_SCOPE)
  message(STATUS "${_tool_PROGRAM} is built from the pinned release ${_tool_URL}.")
endfunction()
