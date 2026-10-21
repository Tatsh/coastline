# Toolchain file for the Nintendo 64. It selects the cross compiler of the libdragon distribution
# and nothing else from it. libultra, its headers, and the boot code are reconstructed in this
# tree. No libdragon include or library directory is added here.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)

if(DEFINED ENV{N64_INST})
  set(N64_INST
      "$ENV{N64_INST}"
      CACHE PATH "libdragon distribution root that provides the cross compiler.")
endif()
# Packaged builds of the libdragon toolchain name the compiler mips64-libdragon-elf-gcc, and the
# libdragon distribution names it mips64-elf-gcc. The assembler, linker, and object tools share the
# prefix of the compiler that is found.
find_program(
  CMAKE_C_COMPILER
  NAMES mips64-libdragon-elf-gcc mips64-elf-gcc
  PATHS "${N64_INST}/bin" "${CMAKE_CURRENT_LIST_DIR}/../.wiswa-ci/toolchains/n64/opt/libdragon/bin"
  REQUIRED)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}")

# The code targets the VR4300 with the o32 ABI, as the original does. The ROM has no dynamic
# linking and no C library from the compiler. The small data section is unused. The original refers
# to every datum by its full address.
set(_n64_flags "-march=vr4300 -mtune=vr4300 -mabi=32 -mno-abicalls -fno-pic -ffreestanding -G0")
# The program must fit before the first asset of the cartridge. The code is built for size. IDO
# does not assume that stores through one type leave objects of another type unchanged. The
# original stores offsets and pointers through a u32 pointer and reads the field back as a pointer.
# IDO places a datum initialised to zero in the data section, and the BSS may not grow.
set(CMAKE_C_FLAGS_INIT "${_n64_flags} -Os -fno-strict-aliasing -fno-zero-initialized-in-bss")
set(CMAKE_ASM_FLAGS_INIT "${_n64_flags}")
# The game is C89 as IDO accepted it, with the GNU extensions the reconstruction uses.
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
