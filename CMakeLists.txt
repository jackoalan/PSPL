cmake_minimum_required(VERSION 2.8)
project(PSPL)

# Codebase `include` directory
include_directories(include)


############################
# Platforms and Extensions #
############################

# Set out platform list scope here
set(pspl_platform_list "" CACHE INTERNAL
    "Ordered plaform name list, augmented by `pspl_add_platform`")
set(pspl_platform_desc_list "" CACHE INTERNAL
    "Ordered plaform description list, augmented by `pspl_add_platform`")
set(pspl_platform_bo_list "" CACHE INTERNAL
    "Ordered plaform byte-order list, augmented by `pspl_add_platform`")

# Set our extension list scope here
set(pspl_extension_list "" CACHE INTERNAL
    "Ordered extension name list, augmented by `pspl_add_extension`")
set(pspl_extension_desc_list "" CACHE INTERNAL
    "Ordered extension description list, augmented by `pspl_add_extension`")
set(pspl_toolchain_extension_list "" CACHE INTERNAL
    "Ordered toolchain extension name list, augmented by `pspl_add_extension_toolchain`")
set(pspl_runtime_extension_list "" CACHE INTERNAL
    "Ordered runtime extension name list, augmented by `pspl_add_extension_runtime`")

# Include Extension authoring macros here
include(${PSPL_SOURCE_DIR}/cmake/PSPLExtensionMacros.cmake)

# Platform and Extension subdirectories
add_subdirectory(Platforms)
add_subdirectory(Extensions)

# Assemble platform definitions together
set(CONF_PLAT_DEF_INCLUDES "")
set(CONF_PLAT_ARRAY_INIT_LIST "")
list(LENGTH pspl_platform_list len)
if(len GREATER 0)
  math(EXPR len ${len}-1)
  foreach(platform RANGE ${len})
  
    # Extension member variables
    list(GET pspl_platform_list ${platform} PLAT_NAME)
    list(GET pspl_platform_desc_list ${platform} PLAT_DESC)
    list(GET pspl_platform_bo_list ${platform} PLAT_BO)
    
    string(TOLOWER ${PLAT_BO} PLAT_BO)
    if(PLAT_BO STREQUAL "little")
      set(CONF_PLAT_BYTE_ORDER "PSPL_LITTLE_ENDIAN")
    elseif(PLAT_BO STREQUAL "big")
      set(CONF_PLAT_BYTE_ORDER "PSPL_BIG_ENDIAN")
    else()
      set(CONF_PLAT_BYTE_ORDER "PSPL_LITTLE_ENDIAN")
      message(AUTHOR_WARNING "WARNING: Platform '${PLAT_NAME}' specified invalid byte-order; it must be specified as 'LITTLE' or 'BIG'; 'LITTLE' will be used by default")
    endif()

    # Generate extension definition
    configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_platform_def.h.in
                   "${PLAT_NAME}_platform_def.h")
                   
    # Ensure the definition is included
    set(CONF_PLAT_DEF_INCLUDES "${CONF_PLAT_DEF_INCLUDES}#include \"${PLAT_NAME}_platform_def.h\"\n")
    set(CONF_PLAT_ARRAY_INIT_LIST "${CONF_PLAT_ARRAY_INIT_LIST}\n    &${PLAT_NAME}_platform,")

  endforeach(platform)
endif()

# Assemble extension definitions together
set(CONF_EXT_DEF_INCLUDES "")
set(CONF_EXT_ARRAY_INIT_LIST "")
set(pspl_extension_toolchain_link_list "")
set(pspl_extension_runtime_link_list "")
list(LENGTH pspl_extension_list len)
if(len GREATER 0)
  math(EXPR len ${len}-1)
  foreach(extension RANGE ${len})
  
    # Extension member variables
    list(GET pspl_extension_list ${extension} EXT_NAME)
    list(GET pspl_extension_desc_list ${extension} EXT_DESC)
    
    set(CONF_EXT_EXTERNS "")
    
    list(FIND pspl_toolchain_extension_list ${EXT_NAME} TOOLEXT_EXIST)
    if(TOOLEXT_EXIST GREATER -1)
      set(CONF_EXT_TOOLCHAIN "&${EXT_NAME}_toolext")
      set(CONF_EXT_EXTERNS "${CONF_EXT_EXTERNS}extern pspl_toolchain_extension_t ${EXT_NAME}_toolext;\n")
      list(APPEND pspl_extension_toolchain_link_list ${EXT_NAME}_toolext)
    else()
      set(CONF_EXT_TOOLCHAIN "NULL")
    endif()

    list(FIND pspl_runtime_extension_list ${EXT_NAME} RUNEXT_EXIST)
    if(RUNEXT_EXIST GREATER -1)
      set(CONF_EXT_RUNTIME "&${EXT_NAME}_runext")
      set(CONF_EXT_EXTERNS "${CONF_EXT_EXTERNS}extern pspl_toolchain_extension_t ${EXT_NAME}_runext;\n")
      list(APPEND pspl_extension_runtime_link_list ${EXT_NAME}_runext)
    else()
      set(CONF_EXT_RUNTIME "NULL")
    endif()

    # Generate extension definition
    configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_extension_def.h.in
                   "${EXT_NAME}_extension_def.h")
                   
    # Ensure the definition is included
    set(CONF_EXT_DEF_INCLUDES "${CONF_EXT_DEF_INCLUDES}#include \"${EXT_NAME}_extension_def.h\"\n")
    set(CONF_EXT_ARRAY_INIT_LIST "${CONF_EXT_ARRAY_INIT_LIST}\n    &${EXT_NAME}_extension,")

  endforeach(extension)
endif()

# Default target platform
set(DEF_PLAT GX)

# Generate the C config file
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_available_extensions.c.in
               pspl_available_extensions.c)





################
# Core Targets #
################


# Runtime platform options
if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set(PSPL_RUNTIME_PLATFORM MAC_GL CACHE STRING 
  "Select which platform to build runtime for [NONE, MAC_GL, EGL_GL, GLX_GL, WIN_GL, WIN_D3D, GX] (Mac OpenGL selected by default)")
else()
  set(PSPL_RUNTIME_PLATFORM NONE CACHE STRING 
  "Select which platform to build runtime for [NONE, MAC_GL, EGL_GL, GLX_GL, WIN_GL, WIN_D3D, GX] (Runtime not built by default)")
endif()



# Target subdirectories
add_subdirectory(Toolchain)
if(NOT PSPL_RUNTIME_PLATFORM STREQUAL NONE)
  add_subdirectory(Runtime)
endif()

