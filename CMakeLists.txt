cmake_minimum_required(VERSION 2.8)

# Determine which primary platform to build
option(PSPL_WINDOWS "Build with Windows support using mingw32-gcc (both Toolchain and Runtime)" OFF)
option(PSPL_WII "Build with Wii support using WiiStep's llvm macros (Runtime only)" OFF)
if(PSPL_WINDOWS)
  set(CMAKE_TOOLCHAIN_FILE cmake/ToolchainCrossDarwinMinGW32.cmake)
  set(PSPL_TOOLCHAIN_TARGET_SYSTEM WINDOWS CACHE STRING
      "Select which platform to (cross) compile PSPL Toolchain for [NATIVE, WII, WINDOWS] (default WINDOWS)")
  set(PSPL_RUNTIME_TARGET_SYSTEM WINDOWS CACHE STRING
      "Select which platform to (cross) compile PSPL Runtime for [NATIVE, WII, WINDOWS] (default WINDOWS)")
elseif(PSPL_WII)
  find_package(WiiStep REQUIRED)
endif()

project(PSPL)

# Codebase `include` directory
include_directories(include)

# Cross compiling options
set(PSPL_TOOLCHAIN_TARGET_SYSTEM NATIVE CACHE STRING
    "Select which platform to (cross) compile PSPL Toolchain for [NATIVE, WII, WINDOWS] (default NATIVE)")
set(PSPL_RUNTIME_TARGET_SYSTEM NATIVE CACHE STRING
    "Select which platform to (cross) compile PSPL Runtime for [NATIVE, WII, WINDOWS] (default NATIVE)")
    

# Hashing library options
if(PSPL_TOOLCHAIN_TARGET_SYSTEM STREQUAL WINDOWS)
  set(PSPL_TOOLCHAIN_HASHING WINDOWS CACHE STRING 
      "Select which hashing library to use for compile-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (WINDOWS selected by default)")
else()
  if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    set(PSPL_TOOLCHAIN_HASHING COMMON_CRYPTO CACHE STRING 
        "Select which hashing library to use for compile-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (COMMON_CRYPTO selected by default)")
  else()
    set(PSPL_TOOLCHAIN_HASHING BUILTIN CACHE STRING 
        "Select which hashing library to use for compile-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (BUILTIN selected by default)")
  endif()
endif()

if(PSPL_RUNTIME_TARGET_SYSTEM STREQUAL WINDOWS)
  set(PSPL_RUNTIME_HASHING WINDOWS CACHE STRING 
      "Select which hashing library to use for run-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (WINDOWS selected by default)")
else()
  if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    set(PSPL_RUNTIME_HASHING COMMON_CRYPTO CACHE STRING 
        "Select which hashing library to use for run-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (COMMON_CRYPTO selected by default)")
  else()
    set(PSPL_RUNTIME_HASHING BUILTIN CACHE STRING
        "Select which hashing library to use for run-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (BUILTIN selected by default)")
  endif()
endif()

add_subdirectory(Hash)

# Common routines
add_library(pspl_common STATIC PSPLCommon.c)


############################
# Platforms and Extensions #
############################

# Runtime platform options
if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set(PSPL_RUNTIME_PLATFORM GL2 CACHE STRING 
      "Select which platform to build runtime for [${pspl_platform_list}] (GL2 selected by default)")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(PSPL_RUNTIME_PLATFORM D3D9 CACHE STRING 
      "Select which platform to build runtime for [${pspl_platform_list}] (D3D9 selected by default)")
else()
  set(PSPL_RUNTIME_PLATFORM "" CACHE STRING 
      "Select which platform to build runtime for [${pspl_platform_list}] (Runtime not built by default)")
endif()

# Set out platform list scope here
set(pspl_platform_list "" CACHE INTERNAL
    "Ordered plaform name list, augmented by `pspl_add_platform`")
set(pspl_platform_desc_list "" CACHE INTERNAL
    "Ordered plaform description list, augmented by `pspl_add_platform`")
set(pspl_platform_bo_list "" CACHE INTERNAL
    "Ordered plaform byte-order list, augmented by `pspl_add_platform`")
set(pspl_platform_typefile_list "" CACHE INTERNAL
    "Ordered plaform type-definition-file list, augmented by `pspl_add_platform`")

# Set our extension list scope here
set(pspl_extension_list "" CACHE INTERNAL
    "Ordered extension name list, augmented by `pspl_add_extension`")
set(pspl_extension_desc_list "" CACHE INTERNAL
    "Ordered extension description list, augmented by `pspl_add_extension`")
set(pspl_toolchain_extension_list "" CACHE INTERNAL
    "Ordered toolchain extension name list, augmented by `pspl_add_extension_toolchain`")
set(pspl_runtime_extension_list "" CACHE INTERNAL
    "Ordered runtime extension name list, augmented by `pspl_add_extension_runtime`")
    
# Final link lists for core targets
set(pspl_extension_toolchain_link_list "")
set(pspl_extension_runtime_link_list "")

# Include Extension authoring macros here
include(${PSPL_SOURCE_DIR}/cmake/PSPLExtensionMacros.cmake)

# Platform and Extension subdirectories
add_subdirectory(Extensions)
add_subdirectory(Platforms)

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
      set(CONF_PLAT_BYTE_ORDER "PSPL_UNSPEC_ENDIAN")
    endif()
    
    list(APPEND pspl_extension_toolchain_link_list ${PLAT_NAME}_toolplat)

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
      set(CONF_EXT_EXTERNS "${CONF_EXT_EXTERNS}\n#ifdef PSPL_TOOLCHAIN\nextern pspl_toolchain_extension_t ${EXT_NAME}_toolext;\n#endif\n")
      list(APPEND pspl_extension_toolchain_link_list ${EXT_NAME}_toolext)
    else()
      set(CONF_EXT_TOOLCHAIN "NULL")
    endif()

    list(FIND pspl_runtime_extension_list ${EXT_NAME} RUNEXT_EXIST)
    if(RUNEXT_EXIST GREATER -1)
      set(CONF_EXT_RUNTIME "&${EXT_NAME}_runext")
      set(CONF_EXT_EXTERNS "${CONF_EXT_EXTERNS}\n#ifdef PSPL_RUNTIME\nextern pspl_toolchain_extension_t ${EXT_NAME}_runext;\n#endif\n")
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

# Generate the C config file
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_available_extensions.c.in
               pspl_available_extensions.c)





################
# Core Targets #
################



# Target subdirectories
add_subdirectory(Toolchain)

list(FIND pspl_platform_list ${PSPL_RUNTIME_PLATFORM} runtime_idx)
if(runtime_idx GREATER -1)
  add_subdirectory(Runtime)
else()
  message(WARNING "Unable to build runtime; '${PSPL_RUNTIME_PLATFORM}' platform not available")
endif()

