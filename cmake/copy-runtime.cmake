foreach(_file IN LISTS FILES)
  get_filename_component(_name "${_file}" NAME)
  file(COPY_FILE "${_file}" "${DESTINATION}/${_name}" ONLY_IF_DIFFERENT)
endforeach()
