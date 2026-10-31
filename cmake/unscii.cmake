# Write the 8 by 16 glyphs of the 256 characters of code page 437 from a font in the hex format of
# GNU Unifont, such as unscii-16.hex, to the C++ header OUTPUT as the array NAME. Each glyph uses
# 16 bytes from the top row, with the leftmost pixel in the high bit. TABLE is the source whose array
# CP437_CODEPOINTS lists the code point of each character in order. A character the font lacks, and
# the first character, take the glyph of the question mark in reverse.
#
#   cmake -DINPUT=unscii-16.hex -DTABLE=textgrid.cpp -DOUTPUT=font.h -DNAME=ARRAY_NAME -P unscii.cmake
file(READ "${INPUT}" _font)
set(_font "\n${_font}")
file(READ "${TABLE}" _table)
string(REGEX MATCH "CP437_CODEPOINTS\\{([^}]*)\\}" _list "${_table}")
string(REGEX MATCHALL "0x[0-9A-F]+" _codes "${CMAKE_MATCH_1}")
list(LENGTH _codes _count)
if(NOT _count EQUAL 256)
  message(FATAL_ERROR "${TABLE} lists ${_count} code points of code page 437, not 256.")
endif()

# The 32 hex digits of the glyph of a code point, or nothing in OUT when the font has no 8 by 16
# glyph for it.
function(_glyph code out)
  string(SUBSTRING "${code}" 2 -1 _digits)
  string(LENGTH "${_digits}" _length)
  while(_length LESS 5)
    string(PREPEND _digits "0")
    math(EXPR _length "${_length} + 1")
  endwhile()
  string(FIND "${_font}" "\n${_digits}:" _at)
  set(${out}
      ""
      PARENT_SCOPE)
  if(_at GREATER_EQUAL 0)
    math(EXPR _start "${_at} + 7")
    string(SUBSTRING "${_font}" ${_start} 33 _bits)
    # A glyph 16 pixels across has 64 digits.
    if(_bits MATCHES "^[0-9A-F]+\n$" OR _bits MATCHES "^[0-9A-F]+$")
      string(LENGTH "${_bits}" _length)
    else()
      set(_length 0)
    endif()
    if(_length EQUAL 33 AND _bits MATCHES "\n$" OR _length EQUAL 32)
      string(SUBSTRING "${_bits}" 0 32 _bits)
      set(${out}
          "${_bits}"
          PARENT_SCOPE)
    endif()
  endif()
endfunction()

_glyph(0x003F _question)
set(_replacement "")
foreach(_index RANGE 0 30 2)
  string(SUBSTRING "${_question}" ${_index} 2 _byte)
  math(EXPR _byte "0xFF & ~0x${_byte}" OUTPUT_FORMAT HEXADECIMAL)
  string(SUBSTRING "${_byte}" 2 -1 _byte)
  string(LENGTH "${_byte}" _length)
  if(_length LESS 2)
    string(PREPEND _byte "0")
  endif()
  string(APPEND _replacement "${_byte}")
endforeach()

set(_hex "")
set(_missing "")
set(_first TRUE)
foreach(_code IN LISTS _codes)
  set(_bits "")
  if(NOT _first)
    _glyph(${_code} _bits)
  endif()
  if(_bits STREQUAL "")
    if(NOT _first)
      list(APPEND _missing ${_code})
    endif()
    set(_bits "${_replacement}")
  endif()
  set(_first FALSE)
  string(APPEND _hex "${_bits}")
endforeach()
if(_missing)
  message(STATUS "${INPUT} has no glyph for ${_missing}, drawn as the replacement glyph.")
endif()
string(TOLOWER "${_hex}" _hex)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
string(REGEX REPLACE "(0x[0-9a-f][0-9a-f],){16}" "\\0\n    " _bytes "${_bytes}")
file(WRITE "${OUTPUT}"
     "#pragma once\n\ninline constexpr unsigned char ${NAME}[] = {\n    ${_bytes}\n};\n")
