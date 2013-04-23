cmake_minimum_required(VERSION 2.8)

# Create a PSPL extension
macro(pspl_add_extension extension_name)
  get_filename_component(SELF_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
  get_filename_component(SELF_DIR ${SELF_DIR} NAME)
  list(FIND pspl_extension_name_list ${extension_name} existing)
  if(existing GREATER -1)
    message(AUTHOR_WARNING "WARNING: Unable to add '${extension_name}' in '${SELF_DIR}'; already added")
  else()
    list(APPEND pspl_extension_name_list ${extension_name})
    set(pspl_extension_name_list ${pspl_extension_name_list} CACHE INTERNAL 
        "Ordered extension name list, augmented by `pspl_add_extension`")
    list(APPEND pspl_extension_dir_list ${SELF_DIR})
    set(pspl_extension_dir_list ${pspl_extension_dir_list} CACHE INTERNAL 
        "Ordered extension dir list, augmented by `pspl_add_extension`")
  endif()
endmacro()

# Augment a PSPL extension with an archivable object class
macro(pspl_add_extension_class extension_name class_id)
  
endmacro()


# Augment a PSPL extension with a toolchain extension
macro(pspl_add_toolchain_extension extension_name)
  
endmacro(pspl_add_toolchain_extension)

