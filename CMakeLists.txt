cmake_minimum_required(VERSION 3.25)
project(coastline VERSION 0.0.0 LANGUAGES C ASM)

option(BUILD_DOCS "Build documentation alongside the sources." OFF)
option(BUILD_DOCS_ONLY "Build documentation and nothing else." OFF)
option(INSTALL_DOCS "Install documentation." OFF)
set(REGION
    "USA"
    CACHE STRING "Release to build: USA for North America, JPN for Japan, or EUR for Europe. USA \
is the naming master, and the other two carry every change of their release.")
set_property(CACHE REGION PROPERTY STRINGS USA JPN EUR)
string(TOUPPER "${REGION}" REGION)
if(NOT REGION MATCHES "^(USA|JPN|EUR)$")
  message(FATAL_ERROR "REGION must be USA, JPN, or EUR.")
endif()

add_compile_definitions(REGION_${REGION})

if(BUILD_DOCS OR BUILD_DOCS_ONLY)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_HTML_OUTPUT "${CMAKE_BINARY_DIR}/html")
  set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
  set(DOXYGEN_MARKDOWN_ID_STYLE GITHUB)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
  set(DOXYGEN_INTERACTIVE_SVG YES)
  set(DOXYGEN_GENERATE_TAGFILE "${CMAKE_BINARY_DIR}/html/${CMAKE_PROJECT_NAME}.tags")
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_SORT_MEMBER_DOCS NO)
  set(DOXYGEN_ENABLE_PREPROCESSING YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  # Every reconstructed routine records the address it was recovered from with this tag.
  set(DOXYGEN_VERBATIM_VARS DOXYGEN_ALIASES)
  # A routine whose body is only a placeholder carries @stub until it is reconstructed.
  set(DOXYGEN_ALIASES [[ghidraAddress="\par Ghidra address:^^"]]
                      [[stub="\par Stub:^^The body is a placeholder and is not reconstructed yet."]])
  set(DOXYGEN_EXCLUDE_PATTERNS "*/node_modules/*" "*/build/*")
  # Headers sit beside their implementations under src, and only headers are documented.
  set(DOXYGEN_FILE_PATTERNS "*.h")
  find_package(Doxygen REQUIRED COMPONENTS dot)
  doxygen_add_docs(docs README.md src ALL)
  if(INSTALL_DOCS)
    install(
      DIRECTORY "${CMAKE_BINARY_DIR}/html"
      DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}")
  endif()
endif()

# A docs-only configure must not require the cross toolchain. The sources are skipped entirely.
if(NOT BUILD_DOCS_ONLY)
  include(FeatureSummary)
  include(GNUInstallDirs)
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
  add_subdirectory(src)
endif()
