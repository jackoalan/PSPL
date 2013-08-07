include_directories(${PSPL_SOURCE_DIR})

# Ensure typefile for *specified runtime platform only* is included in runtime sources
list(GET pspl_platform_typefile_list ${runtime_idx} CONF_RUNTIME_PLATFORM_TYPEFILE)
list(LENGTH pspl_extension_list PSPL_RUNTIME_EXTENSION_COUNT)
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_runtime_platform_typefile.pch.in
               pspl_runtime_platform_typefile.pch)

# Define runtime library
pspl_add_library(pspl-rt STATIC
            RuntimeCore.c
            RuntimeThreads.c
            ${PSPL_BINARY_DIR}/pspl_available_runtime_extensions.c)

set_target_properties(pspl-rt PROPERTIES
                      COMPILE_FLAGS "-include ${PSPL_BINARY_DIR}/Runtime/pspl_runtime_platform_typefile.pch"
                      COMPILE_DEFINITIONS "PSPL_RUNTIME=1;PSPL_RUNTIME_PLATFORM_${PSPL_RUNTIME_PLATFORM}=1;${RUN_HASH_DEF}=1;${RUN_THREAD_DEF}=1")            

# Link runtime extensions
pspl_target_link_libraries(pspl-rt
                           ${pspl_extension_runtime_link_list} 
                           ${HASH_LIB} 
                           ${THREAD_LIB} 
                           pspl_common)


