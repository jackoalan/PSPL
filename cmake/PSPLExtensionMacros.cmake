cmake_minimum_required(VERSION 2.8)



#############
# Platforms #
#############


# Add a PSPL Platform
macro(pspl_add_platform platform_name platform_desc)
  
  list(FIND pspl_platform_list ${platform_name} existing)
  if(existing GREATER -1)
    message(AUTHOR_WARNING "WARNING: Unable to add platform '${platform_name}'; already added")
  else()
    get_filename_component(typefile "${platform_name}_PSPLTypes.h" ABSOLUTE)
    if(NOT EXISTS ${typefile})
      message(AUTHOR_WARNING "WARNING: Unable to add platform '${platform_name}'; '${typefile}' doesn't exist")
    else()
      list(APPEND pspl_platform_list ${platform_name})
      list(APPEND pspl_platform_desc_list ${platform_desc})
      if(${ARGC} GREATER 2)
        set(byte_order ${ARGN})
      else()
        set(byte_order "UNSPEC")
      endif()
      list(APPEND pspl_platform_bo_list ${byte_order})
      list(APPEND pspl_platform_typefile_list ${typefile})
      set(pspl_platform_list ${pspl_platform_list} CACHE INTERNAL
          "Ordered plaform name list, augmented by `pspl_add_platform`")
      set(pspl_platform_desc_list ${pspl_platform_desc_list} CACHE INTERNAL
          "Ordered plaform description list, augmented by `pspl_add_platform`")
      set(pspl_platform_bo_list ${pspl_platform_bo_list} CACHE INTERNAL
          "Ordered plaform byte-order list, augmented by `pspl_add_platform`")
      set(pspl_platform_typefile_list ${pspl_platform_typefile_list} CACHE INTERNAL
          "Ordered plaform type-definition-file list, augmented by `pspl_add_platform`")
    endif()
  endif()

endmacro(pspl_add_platform)



# Add toolchain sources to a PSPL Platform
macro(pspl_add_platform_toolchain platform_name)

  list(FIND pspl_platform_list ${platform_name} plat_exist)
  if(plat_exist LESS 0)
    message(AUTHOR_WARNING "WARNING: Unable to add platform toolchain '${platform_name}'; `pspl_add_platform` hasn't been called first")
  else()
    set_source_files_properties(${ARGN} PROPERTIES COMPILE_DEFINITIONS PSPL_TOOLCHAIN=1)
    add_library("${platform_name}_toolplat" STATIC ${ARGN})
  endif()

endmacro(pspl_add_platform_toolchain)



# Add runtime sources to a PSPL Platform
macro(pspl_add_platform_runtime platform_name)

  list(FIND pspl_platform_list ${platform_name} plat_exist)
  if(plat_exist LESS 0)
    message(AUTHOR_WARNING "WARNING: Unable to add platform runtime '${platform_name}'; `pspl_add_platform` hasn't been called first")
  else()
    set_source_files_properties(${ARGN} PROPERTIES COMPILE_DEFINITIONS "PSPL_RUNTIME=1;PSPL_RUNTIME_PLATFORM_${PSPL_RUNTIME_PLATFORM}=1")
    if(${platform_name} STREQUAL ${PSPL_RUNTIME_PLATFORM})
      add_library("${platform_name}_runplat" STATIC ${ARGN})
      set_target_properties("${platform_name}_runplat" PROPERTIES
                            COMPILE_FLAGS "-include ${PSPL_BINARY_DIR}/Runtime/pspl_runtime_platform_typefile.pch"
                            COMPILE_DEFINITIONS "PSPL_RUNTIME=1;PSPL_RUNTIME_PLATFORM_${PSPL_RUNTIME_PLATFORM}=1")
    endif()
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


# Hashing library define (for toolchain extensions)
unset(HASH_LIB)
unset(TOOL_HASH_DEF)
if(PSPL_TOOLCHAIN_HASHING STREQUAL BUILTIN)
  set(HASH_LIB hash_builtin)
  set(TOOL_HASH_DEF PSPL_HASHING_BUILTIN)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL COMMON_CRYPTO)
  set(TOOL_HASH_DEF PSPL_HASHING_COMMON_CRYPTO)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL OPENSSL)
  find_library(HASH_LIB crypto)
  set(TOOL_HASH_DEF PSPL_HASHING_OPENSSL)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL WINDOWS)
  set(TOOL_HASH_DEF PSPL_HASHING_WINDOWS)
endif()

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
      #set_source_files_properties(${ARGN} PROPERTIES COMPILE_DEFINITIONS PSPL_TOOLCHAIN=1)
      add_library("${extension_name}_toolext" STATIC ${ARGN})
      set_target_properties("${extension_name}_toolext" PROPERTIES
                            COMPILE_DEFINITIONS "PSPL_TOOLCHAIN=1;${TOOL_HASH_DEF}=1")
    endif()
  endif()
  
endmacro(pspl_add_extension_toolchain)


# Hashing library define (for runtime extensions)
unset(HASH_LIB)
unset(RUN_HASH_DEF)
if(PSPL_RUNTIME_HASHING STREQUAL BUILTIN)
  set(HASH_LIB hash_builtin)
  set(RUN_HASH_DEF PSPL_HASHING_BUILTIN)
elseif(PSPL_RUNTIME_HASHING STREQUAL COMMON_CRYPTO)
  set(RUN_HASH_DEF PSPL_HASHING_COMMON_CRYPTO)
elseif(PSPL_RUNTIME_HASHING STREQUAL OPENSSL)
  find_library(HASH_LIB crypto)
  set(RUN_HASH_DEF PSPL_HASHING_OPENSSL)
elseif(PSPL_RUNTIME_HASHING STREQUAL WINDOWS)
  set(RUN_HASH_DEF PSPL_HASHING_WINDOWS)
endif()

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
      #set_source_files_properties(${ARGN} PROPERTIES COMPILE_DEFINITIONS "PSPL_RUNTIME=1;PSPL_RUNTIME_PLATFORM_${PSPL_RUNTIME_PLATFORM}=1")
      add_library("${extension_name}_runext" STATIC ${ARGN})
      set_target_properties("${extension_name}_runext" PROPERTIES
                            COMPILE_FLAGS "-include ${PSPL_BINARY_DIR}/Runtime/pspl_runtime_platform_typefile.pch"
                            COMPILE_DEFINITIONS "PSPL_RUNTIME=1;PSPL_RUNTIME_PLATFORM_${PSPL_RUNTIME_PLATFORM}=1;${RUN_HASH_DEF}=1;${RUN_THREAD_DEF}=1")
    endif()
  endif()

endmacro(pspl_add_extension_runtime)

