cmake_minimum_required(VERSION 2.8)

# Target list
set(PSPL_PACKAGE_TARGETS "" CACHE INTERNAL "" FORCE)

# Add package target
macro(add_pspl_package target_name)
  list(FIND PSPL_PACKAGE_TARGETS ${target_name} found)
  if(found GREATER -1)
      message(WARNING "Unable to add PSPL package; `${target_name}` already added")
  else()
  
    list(APPEND PSPL_PACKAGE_TARGETS ${target_name})
    set(PSPL_PACKAGE_TARGETS ${PSPL_PACKAGE_TARGETS} CACHE INTERNAL "" FORCE)
    
    # Make list of generated objects
    unset(objects)
    foreach(source ${ARGN})
      get_filename_component(ext ${source} EXT)
      string(TOLOWER ${ext} ext)
      if(${ext} STREQUAL .pspl)
        get_filename_component(base ${source} NAME_WE)
        get_filename_component(path ${source} ABSOLUTE)
        include(${CMAKE_CURRENT_BINARY_DIR}/${base}.psplg OPTIONAL RESULT_VARIABLE gather_file)
        if(gather_file STREQUAL NOTFOUND)
          file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${base}.psplg "")
          include(${CMAKE_CURRENT_BINARY_DIR}/${base}.psplg)
        endif()
        add_custom_command(OUTPUT ${base}.psplc COMMAND ${PSPL_BINARY_LOC} ARGS 
                           -c -o ${base}.psplc -G ${base}.psplg -S ${CMAKE_BINARY_DIR} ${path}
                           DEPENDS ${PSPL_REFLIST})
        list(APPEND objects ${base}.psplc)
      else()
        message(WARNING "Unable to add ${source} to PSPL target ${target_name}; file extension must be `.pspl`")
      endif()
    endforeach(source)
    
    # Package rule
    add_custom_command(OUTPUT ${target_name}.psplp COMMAND ${PSPL_BINARY_LOC} ARGS 
                       -o ${target_name}.psplp -S ${CMAKE_BINARY_DIR} ${objects}
                       DEPENDS ${objects})
    add_custom_target(${target_name} ALL DEPENDS ${target_name}.psplp)
    
  endif()
endmacro(add_pspl_package)
