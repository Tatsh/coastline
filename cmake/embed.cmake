# Write the bytes of INPUT to the C++ header OUTPUT as the array NAME.
#
#   cmake -DINPUT=file -DOUTPUT=header.h -DNAME=ARRAY_NAME -P embed.cmake
file(READ "${INPUT}" _hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
string(REGEX REPLACE "(0x[0-9a-f][0-9a-f],){16}" "\\0\n    " _bytes "${_bytes}")
file(WRITE "${OUTPUT}" "#pragma once\n\ninline constexpr unsigned char ${NAME}[] = {\n    ${_bytes}\n};\n")
