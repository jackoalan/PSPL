include_directories(${PSPL_SOURCE_DIR})

# Hashing library define
unset(HASH_LIB)
if(PSPL_RUNTIME_HASHING STREQUAL BUILTIN)
  add_definitions(-D PSPL_HASHING_BUILTIN)
  set(HASH_LIB hash_builtin)
elseif(PSPL_RUNTIME_HASHING STREQUAL COMMON_CRYPTO)
  add_definitions(-D PSPL_HASHING_COMMON_CRYPTO)
elseif(PSPL_RUNTIME_HASHING STREQUAL OPENSSL)
  add_definitions(-D PSPL_HASHING_OPENSSL)
  find_library(HASH_LIB crypto)
elseif(PSPL_RUNTIME_HASHING STREQUAL WINDOWS)
  add_definitions(-D PSPL_HASHING_WINDOWS)
endif()

# Include threading library in runtime (if available)
unset(THREAD_LIB)
if(PSPL_WII)
  add_definitions(-D PSPL_THREADING_OGC)
elseif(GCD_FOUND)
  include_directories(${GCD_INCLUDE_DIR})
  set(THREAD_LIB ${GCD_LINK_LIBRARIES})
  add_definitions(-D PSPL_THREADING_GCD)
else()
  find_package(Threads)
  set(THREAD_LIB ${CMAKE_THREAD_LIBS_INIT})
  if(CMAKE_USE_PTHREADS_INIT)
    add_definitions(-D PSPL_THREADING_PTHREAD)
  elseif(CMAKE_USE_WIN32_THREADS_INIT)
    add_definitions(-D PSPL_THREADING_WINDOWS)
  endif()
endif()


# Ensure typefile for *specified runtime platform only* is included in runtime sources
list(GET pspl_platform_typefile_list ${runtime_idx} CONF_RUNTIME_PLATFORM_TYPEFILE)
list(LENGTH pspl_extension_list PSPL_RUNTIME_EXTENSION_COUNT)
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_runtime_platform_typefile.pch.in
               pspl_runtime_platform_typefile.pch)

# Define runtime library
add_library(pspl-rt STATIC
            RuntimeCore.c
            RuntimeThreads.c
            ${PSPL_BINARY_DIR}/pspl_available_runtime_extensions.c)
            
# All sources in `pspl-rt` should be exposed to platform-specific types
set_target_properties(pspl-rt PROPERTIES
                      COMPILE_FLAGS "-include pspl_runtime_platform_typefile.pch"
                      COMPILE_DEFINITIONS "PSPL_RUNTIME=1;PSPL_RUNTIME_PLATFORM_${PSPL_RUNTIME_PLATFORM}=1")

# Link runtime extensions
target_link_libraries(pspl-rt
                      ${pspl_extension_runtime_link_list} 
                      ${HASH_LIB} 
                      ${THREAD_LIB} 
                      pspl_common)


