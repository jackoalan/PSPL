cmake_minimum_required(VERSION 2.8)

# Determine which primary platform to build
option(PSPL_CROSS_WIN32 "Build with 32-bit Windows support using mingw32-gcc (both Toolchain and Runtime)" OFF)
option(PSPL_CROSS_WIN64 "Build with 64-bit Windows support using mingw32-gcc (both Toolchain and Runtime)" OFF)
option(PSPL_CROSS_WII "Build with Wii support using WiiStep's llvm macros (Runtime only)" OFF)
if(PSPL_CROSS_WIN32 OR PSPL_CROSS_WIN64)
  set(CMAKE_TOOLCHAIN_FILE cmake/ToolchainCrossWindows.cmake)
  set(PSPL_TOOLCHAIN_TARGET_SYSTEM WINDOWS CACHE STRING
      "Select which platform to (cross) compile PSPL Toolchain for [NATIVE, WII, WINDOWS] (default WINDOWS)")
  set(PSPL_RUNTIME_TARGET_SYSTEM WINDOWS CACHE STRING
      "Select which platform to (cross) compile PSPL Runtime for [NATIVE, WII, WINDOWS] (default WINDOWS)")
  macro(pspl_add_library name)
    add_library(${name} ${ARGN})
  endmacro(pspl_add_library)
  macro(pspl_add_executable name)
    add_executable(${name} ${ARGN})
  endmacro(pspl_add_executable)
  macro(pspl_target_link_libraries name)
    target_link_libraries(${name} ${ARGN})
  endmacro(pspl_target_link_libraries)
  
elseif(WIN32)
  set(PSPL_TOOLCHAIN_TARGET_SYSTEM WINDOWS CACHE STRING
      "Select which platform to (cross) compile PSPL Toolchain for [NATIVE, WII, WINDOWS] (default WINDOWS)")
  set(PSPL_RUNTIME_TARGET_SYSTEM WINDOWS CACHE STRING
      "Select which platform to (cross) compile PSPL Runtime for [NATIVE, WII, WINDOWS] (default WINDOWS)")
  macro(pspl_add_library name)
    add_library(${name} ${ARGN})
  endmacro(pspl_add_library)
  macro(pspl_add_executable name)
    add_executable(${name} ${ARGN})
  endmacro(pspl_add_executable)
  macro(pspl_target_link_libraries name)
    get_target_property(new_link_flags ${name} LINK_FLAGS)
    if(${new_link_flags} STREQUAL new_link_flags-NOTFOUND)
      unset(new_link_flags)
    endif()
    set(${name}_new_post_link_flags "" CACHE INTERNAL "")
    foreach(lib ${ARGN})
      get_target_property(lib_type ${lib} TYPE)
      if(lib_type STREQUAL STATIC_LIBRARY)
        get_target_property(lib_dir ${lib} LOCATION)
        get_filename_component(lib_dir ${lib_dir} PATH)
        get_target_property(lib_flags ${lib} LINK_FLAGS)
        if("${lib_flags}" STREQUAL "lib_flags-NOTFOUND")
          unset(lib_flags)
        endif()
        set(new_link_flags "${lib_flags} ${new_link_flags}")
        set(new_link_flags "-L${lib_dir} -l${lib} ${new_link_flags}")
        message("FLAGS: ${lib_flags}")
      elseif(lib_type STREQUAL lib_type-NOTFOUND)
        set(${name}_new_post_link_flags "${${name}_new_post_link_flags} \"${lib}\"" CACHE INTERNAL "")
        message("ADDED ${${name}_new_post_link_flags}")
      endif()
      target_link_libraries(${name} ${lib})
    endforeach(lib)
    
    get_target_property(type ${name} TYPE)
    if(${type} STREQUAL Executable)
      set_target_properties(${name} PROPERTIES LINK_FLAGS "${new_link_flags} ${${name}_new_post_link_flags}")
    else()
      set_target_properties(${name} PROPERTIES LINK_FLAGS "${new_link_flags}")
    endif()
  endmacro(pspl_target_link_libraries)

elseif(PSPL_CROSS_WII)
  enable_testing()
  find_package(WiiStep REQUIRED)
  set(PSPL_TOOLCHAIN_TARGET_SYSTEM WII CACHE STRING
      "Select which platform to (cross) compile PSPL Toolchain for [NATIVE, WII, WINDOWS] (default WII)")
  set(PSPL_RUNTIME_TARGET_SYSTEM WII CACHE STRING
      "Select which platform to (cross) compile PSPL Runtime for [NATIVE, WII, WINDOWS] (default WII)")
  macro(pspl_add_library name)
    add_wii_library(${name} ${ARGN})
  endmacro(pspl_add_library)
  macro(pspl_add_executable name)
    add_wii_executable(${name} ${ARGN})
  endmacro(pspl_add_executable)
  macro(pspl_target_link_libraries name)
    target_link_wii_llvm_libraries(${name} ${ARGN})
  endmacro(pspl_target_link_libraries)

else()
  enable_testing()
  macro(pspl_add_library name)
    add_library(${name} ${ARGN})
  endmacro(pspl_add_library)
  macro(pspl_add_executable name)
    add_executable(${name} ${ARGN})
  endmacro(pspl_add_executable)
  macro(pspl_target_link_libraries name)
    target_link_libraries(${name} ${ARGN})
  endmacro(pspl_target_link_libraries)

endif()


project(PSPL)

# Build-platform specifics
if(APPLE)
  set(OSX_SDK macosx10.9 CACHE STRING "Selected Mac OS X SDK")
  include(cmake/FindMacSDK.cmake)
  include(cmake/FindGCD.cmake)
  
elseif(WIN32)
  include(cmake/FindDirectX11.cmake)
  include_directories(${DirectX11_INCLUDE_DIR})
  add_definitions(-D __LITTLE_ENDIAN__=1)
  #list(APPEND CMAKE_C_FLAGS "-mno-ms-bitfields")
  SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
  SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
  SET(CMAKE_C_LINK_EXECUTABLE
      "<CMAKE_C_COMPILER> <FLAGS> <OBJECTS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS>  -o <TARGET> -Wl,--out-implib,<TARGET_IMPLIB> ${CMAKE_GNULD_IMAGE_VERSION} <LINK_LIBRARIES>")
  SET(CMAKE_CXX_LINK_EXECUTABLE
      "<CMAKE_CXX_COMPILER> <FLAGS> <OBJECTS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS>  -o <TARGET> -Wl,--out-implib,<TARGET_IMPLIB> ${CMAKE_GNULD_IMAGE_VERSION} <LINK_LIBRARIES>")
  
endif()



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
elseif(PSPL_RUNTIME_TARGET_SYSTEM STREQUAL WII)
  set(PSPL_RUNTIME_HASHING BUILTIN CACHE STRING 
      "Select which hashing library to use for run-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (BUILTIN selected by default)")
else()
  if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
    set(PSPL_RUNTIME_HASHING COMMON_CRYPTO CACHE STRING 
        "Select which hashing library to use for run-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (COMMON_CRYPTO selected by default)")
  else()
    set(PSPL_RUNTIME_HASHING BUILTIN CACHE STRING
        "Select which hashing library to use for run-time object hashing [BUILTIN, COMMON_CRYPTO, OPENSSL, WINDOWS] (BUILTIN selected by default)")
  endif()
endif()

if(PSPL_TOOLCHAIN_HASHING STREQUAL BUILTIN OR PSPL_RUNTIME_HASHING STREQUAL BUILTIN)
  pspl_add_library(hash_builtin hash_builtin.c)
endif()

# Runtime Threading API
unset(RUN_THREAD_LIB)
unset(RUN_THREAD_DEF)
if(PSPL_CROSS_WII)
  set(RUN_THREAD_DEF PSPL_THREADING_OGC)
elseif(GCD_FOUND)
  include_directories(${GCD_INCLUDE_DIR})
  set(RUN_THREAD_LIB ${GCD_LINK_LIBRARIES})
  set(RUN_THREAD_DEF PSPL_THREADING_GCD)
elseif(WIN32)
  set(RUN_THREAD_DEF PSPL_THREADING_WINDOWS)
else()
  find_package(Threads)
  set(RUN_THREAD_LIB ${CMAKE_THREAD_LIBS_INIT})
  if(CMAKE_USE_PTHREADS_INIT)
    set(RUN_THREAD_DEF PSPL_THREADING_PTHREAD)
  elseif(CMAKE_USE_WIN32_THREADS_INIT)
    set(RUN_THREAD_DEF PSPL_THREADING_WINDOWS)
  endif()
endif()

# Common routines
pspl_add_library(pspl_common STATIC PSPLCommon.c)


############################
# Platforms and Extensions #
############################

# Runtime platform options
if(PSPL_CROSS_WII)
  set(PSPL_RUNTIME_PLATFORM GX CACHE STRING 
      "Select which platform to build runtime for [${pspl_platform_list}] (GX selected by default)")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set(PSPL_RUNTIME_PLATFORM GL2 CACHE STRING 
      "Select which platform to build runtime for [${pspl_platform_list}] (GL2 selected by default)")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(PSPL_RUNTIME_PLATFORM D3D11 CACHE STRING 
      "Select which platform to build runtime for [${pspl_platform_list}] (D3D11 selected by default)")
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
    if(PSPL_RUNTIME_PLATFORM STREQUAL PLAT_NAME)
      list(APPEND pspl_extension_runtime_link_list ${PLAT_NAME}_runplat)
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
      set(CONF_EXT_EXTERNS "${CONF_EXT_EXTERNS}\n#ifdef PSPL_RUNTIME\nextern pspl_runtime_extension_t ${EXT_NAME}_runext;\n#endif\n")
      list(APPEND pspl_extension_runtime_link_list ${EXT_NAME}_runext)
    else()
      set(CONF_EXT_RUNTIME "NULL")
    endif()
    
    set(CONF_EXT_RUNTIME_IDX ${extension})

    # Generate extension definition
    configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_extension_def.h.in
                   "${EXT_NAME}_extension_def.h")
                   
    # Ensure the definition is included
    set(CONF_EXT_DEF_INCLUDES "${CONF_EXT_DEF_INCLUDES}#include \"${EXT_NAME}_extension_def.h\"\n")
    set(CONF_EXT_ARRAY_INIT_LIST "${CONF_EXT_ARRAY_INIT_LIST}\n    &${EXT_NAME}_extension,")

  endforeach(extension)
endif()

# Generate the C config files
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_available_extensions.c.in
               pspl_available_toolchain_extensions.c)
set(CONF_PLAT_DEF_INCLUDES "#include \"${PSPL_RUNTIME_PLATFORM}_platform_def.h\"\n")
set(CONF_PLAT_ARRAY_INIT_LIST "\n    &${PSPL_RUNTIME_PLATFORM}_platform,")
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_available_extensions.c.in
               pspl_available_runtime_extensions.c)





################
# Core Targets #
################

# Target subdirectories
if(NOT (PSPL_CROSS_WII OR PSPL_CROSS_WIN32 OR PSPL_CROSS_WIN64))
  add_subdirectory(Toolchain)
endif()

list(FIND pspl_platform_list ${PSPL_RUNTIME_PLATFORM} runtime_idx)
if(runtime_idx GREATER -1)
  add_subdirectory(Runtime)
else()
  message(WARNING "Unable to build runtime; '${PSPL_RUNTIME_PLATFORM}' platform not available")
endif()



##########
# Export #
##########

message("HASH: ${PSPL_TOOLCHAIN_HASHING}")
unset(HASH_LIB)
if(PSPL_TOOLCHAIN_HASHING STREQUAL BUILTIN)
  set(HASH_LIB hash_builtin)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL COMMON_CRYPTO)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL OPENSSL)
  find_library(HASH_LIB crypto)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL WINDOWS)
endif()

message("${HASH_LIB}")

export(TARGETS pspl-rt
       ${pspl_extension_runtime_link_list}
       ${HASH_LIB}
       pspl_common 
       FILE pspl-rt-targets.cmake)
get_filename_component(PSPL_RT_TARGET_FILE ${PSPL_BINARY_DIR}/pspl-rt-targets.cmake ABSOLUTE)
if(NOT (PSPL_CROSS_WII OR PSPL_CROSS_WIN32 OR PSPL_CROSS_WIN64))
  get_target_property(PSPL_BINARY_LOC pspl LOCATION)
  configure_file(${PSPL_SOURCE_DIR}/cmake/PSPLConfig.cmake.in
                 PSPLConfig.cmake)
  export(PACKAGE PSPL)
endif()

if(PSPL_CROSS_WII)
  configure_file(${PSPL_SOURCE_DIR}/cmake/PSPL-RTConfig.cmake.in
                 PSPL-WiiConfig.cmake)
  export(PACKAGE PSPL-Wii)
elseif(PSPL_CROSS_WIN32 OR PSPL_CROSS_WIN64)
  configure_file(${PSPL_SOURCE_DIR}/cmake/PSPL-RTConfig.cmake.in
                 PSPL-WinConfig.cmake)
  export(PACKAGE PSPL-Win)
else()
  configure_file(${PSPL_SOURCE_DIR}/cmake/PSPL-RTConfig.cmake.in
                 PSPL-NativeConfig.cmake)
  export(PACKAGE PSPL-Native)
endif()



#########
# Tests #
#########

add_subdirectory(Test)

