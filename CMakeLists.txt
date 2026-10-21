cmake_minimum_required(VERSION 3.25)
project(coastline VERSION 0.0.0 LANGUAGES C ASM)

option(BUILD_DOCS "Build documentation alongside the sources." OFF)
option(BUILD_DOCS_ONLY "Build documentation and nothing else." OFF)
option(INSTALL_DOCS "Install documentation." OFF)
set(TGR_VARIANTS
    ""
    CACHE STRING "Variants to build in one step, as a list of n64-us, n64-jp, n64-eu, win-d3d, \
and win-glide, or all for the five. Empty builds the one variant TGR_PLATFORM, TGR_REGION, and \
TGR_RENDERER select.")
set(TGR_PLATFORM
    "n64"
    CACHE STRING "Platform to build: n64 for the Nintendo 64 ROM or win for the Windows library.")
set_property(CACHE TGR_PLATFORM PROPERTY STRINGS n64 win)
string(TOLOWER "${TGR_PLATFORM}" TGR_PLATFORM)
if(NOT TGR_PLATFORM MATCHES "^(n64|win)$")
  message(FATAL_ERROR "TGR_PLATFORM must be n64 or win.")
endif()
set(TGR_REGION
    "us"
    CACHE STRING "Nintendo 64 release to build: us for North America, jp for Japan, or eu for \
Europe. The USA release is the naming master, and the other two include every change of their \
release.")
set_property(CACHE TGR_REGION PROPERTY STRINGS us jp eu)
string(TOLOWER "${TGR_REGION}" TGR_REGION)
if(NOT TGR_REGION MATCHES "^(us|jp|eu)$")
  message(FATAL_ERROR "TGR_REGION must be us, jp, or eu.")
endif()
set(TGR_RENDERER
    "d3d"
    CACHE STRING "Windows library to build: d3d for BRD3D.dll or glide for BRGlide.dll.")
set_property(CACHE TGR_RENDERER PROPERTY STRINGS d3d glide)
string(TOLOWER "${TGR_RENDERER}" TGR_RENDERER)
if(NOT TGR_RENDERER MATCHES "^(d3d|glide)$")
  message(FATAL_ERROR "TGR_RENDERER must be d3d or glide.")
endif()

# Every variant TGR_VARIANTS lists is configured and built on its own under the build directory.
if(TGR_VARIANTS)
  include(cmake/variants.cmake)
  return()
endif()

# A routine that differs substantially between targets has one translation unit per target, and
# the build selects the unit. These definitions are for a header whose prototype differs between
# targets, and for a routine whose difference is a few lines. The preprocessor cannot compare
# strings, and each choice is a number.
string(TOUPPER "${TGR_PLATFORM}" _tgr_platform)
string(TOUPPER "${TGR_REGION}" _tgr_region)
string(TOUPPER "${TGR_RENDERER}" _tgr_renderer)
add_compile_definitions(
  TGR_PLATFORM_N64=1
  TGR_PLATFORM_WIN=2
  TGR_PLATFORM=TGR_PLATFORM_${_tgr_platform}
  TGR_REGION_US=1
  TGR_REGION_JP=2
  TGR_REGION_EU=3
  TGR_REGION=TGR_REGION_${_tgr_region}
  TGR_RENDERER_D3D=1
  TGR_RENDERER_GLIDE=2
  TGR_RENDERER=TGR_RENDERER_${_tgr_renderer})

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
  # A routine whose body is only a placeholder is tagged @stub until it is reconstructed.
  set(DOXYGEN_ALIASES [[ghidraAddress="\par Ghidra address:^^"]]
                      [[stub="\par Stub:^^The body is a placeholder, not reconstructed yet."]])
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

# A docs-only configure must not require the cross toolchain. The sources are skipped.
if(NOT BUILD_DOCS_ONLY)
  include(FeatureSummary)
  include(GNUInstallDirs)
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
  add_subdirectory(src)
  if(TGR_PLATFORM STREQUAL "n64")
    include(cmake/tools.cmake)
    include(cmake/assets.cmake)
  endif()
endif()
