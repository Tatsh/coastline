# Toolchain file for the Nintendo 64 with IDO 5.3, the compiler of the original. IDO compiles the C;
# the assembler, the archiver, and the linker are those of the libdragon distribution, as in
# n64-toolchain.cmake. Each source takes the flags tools/ido_flags.txt gives it (cmake/ido-flags.cmake).
#
#   cmake --preset n64-us -DCMAKE_TOOLCHAIN_FILE=cmake/n64-ido-toolchain.cmake -DIDO_ROOT=<ido 5.3>

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)
set(TGR_IDO ON)

if(DEFINED ENV{IDO_ROOT})
  set(IDO_ROOT
      "$ENV{IDO_ROOT}"
      CACHE PATH "Directory of IDO 5.3, with its cc.")
endif()
set(IDO_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../.wiswa-ci/ido/5.3"
    CACHE PATH "Directory of IDO 5.3, with its cc.")
if(DEFINED ENV{N64_INST})
  set(N64_INST
      "$ENV{N64_INST}"
      CACHE PATH "libdragon distribution root that provides the assembler and the linker.")
endif()

find_program(
  CMAKE_C_COMPILER
  NAMES cc
  PATHS "${IDO_ROOT}"
  NO_DEFAULT_PATH REQUIRED)
# IDO is not a compiler CMake knows, and it builds no program for this host to test.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER_ID IDO)
find_program(
  TGR_N64_GCC
  NAMES mips64-libdragon-elf-gcc mips64-elf-gcc
  PATHS "${N64_INST}/bin" "${CMAKE_CURRENT_LIST_DIR}/../.wiswa-ci/toolchains/n64/opt/libdragon/bin"
  REQUIRED)
get_filename_component(_n64_bin "${TGR_N64_GCC}" DIRECTORY)
get_filename_component(_n64_prefix "${TGR_N64_GCC}" NAME)
string(REGEX REPLACE "gcc$" "" _n64_prefix "${_n64_prefix}")
set(CMAKE_ASM_COMPILER "${TGR_N64_GCC}")
set(CMAKE_AR "${_n64_bin}/${_n64_prefix}ar" CACHE FILEPATH "")
set(CMAKE_RANLIB "${_n64_bin}/${_n64_prefix}ranlib" CACHE FILEPATH "")
set(CMAKE_OBJCOPY "${_n64_bin}/${_n64_prefix}objcopy" CACHE FILEPATH "")
# The objects IDO writes link with the linker of the libdragon distribution.
set(CMAKE_C_LINK_EXECUTABLE
    "${TGR_N64_GCC} <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# The flags of IDO for the game, as the original was built: code for the VR4300 without shared code
# or small data, C++ comments accepted, and the headers of the IDO distribution. The level of the
# instruction set comes with the flags of each source.
set(CMAKE_C_FLAGS_INIT
    "-nostdinc -Dinline= -non_shared -G 0 -Xcpluscomm -Wab,-r4300_mul -I${IDO_ROOT}/../include -D_LANGUAGE_C -DF3DEX_GBI -D_MIPS_SZLONG=32 -D__sgi -DNDEBUG"
)
set(CMAKE_ASM_FLAGS_INIT
    "-march=vr4300 -mtune=vr4300 -mabi=32 -mno-abicalls -fno-pic -ffreestanding -G0")
set(CMAKE_C_OUTPUT_EXTENSION .o)
# The 64-bit arithmetic of libultra is compiled for MIPS III in a 32-bit program, as in the SDK.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--no-warn-mismatch")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
