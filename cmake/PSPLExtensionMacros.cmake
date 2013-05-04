cmake_minimum_required(VERSION 2.8)



#############
# Platforms #
#############


# Add a PSPL Platform
macro(pspl_add_platform platform_name platform_desc byte_order)
  
  list(FIND pspl_platform_list ${platform_name} existing)
  if(existing GREATER -1)
    message(AUTHOR_WARNING "WARNING: Unable to add platform '${platform_name}'; already added")
  else()
    list(APPEND pspl_platform_list ${platform_name})
    list(APPEND pspl_platform_desc_list ${platform_desc})
    list(APPEND pspl_platform_bo_list ${byte_order})
    set(pspl_platform_list ${pspl_platform_list} CACHE INTERNAL
        "Ordered plaform name list, augmented by `pspl_add_platform`")
    set(pspl_platform_desc_list ${pspl_platform_desc_list} CACHE INTERNAL
        "Ordered plaform description list, augmented by `pspl_add_platform`")
    set(pspl_platform_bo_list ${pspl_platform_bo_list} CACHE INTERNAL
        "Ordered plaform byte-order list, augmented by `pspl_add_platform`")
  endif()

endmacro(pspl_add_platform)



# Add toolchain sources to a PSPL Platform
macro(pspl_add_platform_toolchain platform_name)

  list(FIND pspl_platform_list ${platform_name} plat_exist)
  if(plat_exist LESS 0)
    message(AUTHOR_WARNING "WARNING: Unable to add platform toolchain '${platform_name}'; `pspl_add_platform` hasn't been called first")
  else()
    add_library("${platform_name}_toolplat" STATIC ${ARGN})
  endif()

endmacro(pspl_add_platform_toolchain)



# Add runtime sources to a PSPL Platform
macro(pspl_add_platform_runtime platform_name)

  list(FIND pspl_platform_list ${platform_name} plat_exist)
  if(plat_exist LESS 0)
    message(AUTHOR_WARNING "WARNING: Unable to add platform runtime '${platform_name}'; `pspl_add_platform` hasn't been called first")
  else()
    add_library("${platform_name}_runplat" STATIC ${ARGN})
  endif()

endmacro(pspl_add_platform_runtime)



##############
# Extensions #
##############


# Add a PSPL Extension
macro(pspl_add_extension extension_name extension_desc)

  list(FIND pspl_extension_list ${extension_name} existing)
  if(existing GREATER -1)
    message(AUTHOR_WARNING "WARNING: Unable to add extension '${extension_name}'; already added")
  else()
    list(APPEND pspl_extension_list ${extension_name})
    list(APPEND pspl_extension_desc_list ${extension_desc})
    set(pspl_extension_list ${pspl_extension_list} CACHE INTERNAL
        "Ordered extension name list, augmented by `pspl_add_extension`")
    set(pspl_extension_desc_list ${pspl_extension_desc_list} CACHE INTERNAL
        "Ordered extension description list, augmented by `pspl_add_extension`")
  endif()

endmacro(pspl_add_extension)



# Add a PSPL Toolchain extension to an extension
macro(pspl_add_extension_toolchain extension_name)

  list(FIND pspl_extension_list ${extension_name} ext_exist)
  if(ext_exist LESS 0)
    message(AUTHOR_WARNING "WARNING: Unable to add toolchain extension '${extension_name}'; `pspl_add_extension` hasn't been called first")
  else()
    list(FIND pspl_toolchain_extension_list ${extension_name} existing)
    if(existing GREATER -1)
      message(AUTHOR_WARNING "WARNING: Unable to add toolchain extension '${extension_name}'; already added")
    else()
      list(APPEND pspl_toolchain_extension_list ${extension_name})
      set(pspl_toolchain_extension_list ${pspl_toolchain_extension_list} CACHE INTERNAL
          "Ordered toolchain extension name list, augmented by `pspl_add_extension_toolchain`")
      add_library("${extension_name}_toolext" STATIC ${ARGN})
    endif()
  endif()
  
endmacro(pspl_add_extension_toolchain)



# Add a PSPL Runtime extension to an extension
macro(pspl_add_extension_runtime extension_name)
  
  list(FIND pspl_extension_list ${extension_name} ext_exist)
  if(ext_exist LESS 0)
    message(AUTHOR_WARNING "WARNING: Unable to add runtime extension '${extension_name}'; `pspl_add_extension` hasn't been called first")
  else()
    list(FIND pspl_runtime_extension_list ${extension_name} existing)
    if(existing GREATER -1)
      message(AUTHOR_WARNING "WARNING: Unable to add runtime extension '${extension_name}'; already added")
    else()
      list(APPEND pspl_runtime_extension_list ${extension_name})
      set(pspl_runtime_extension_list ${pspl_runtime_extension_list} CACHE INTERNAL
          "Ordered runtime extension name list, augmented by `pspl_add_extension_runtime`")
      add_library("${extension_name}_runext" STATIC ${ARGN})
    endif()
  endif()

endmacro(pspl_add_extension_runtime)

