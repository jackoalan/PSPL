include_directories(${PSPL_SOURCE_DIR})

# Ensure toolchain macro set for `pspl_available_extensions`
set_source_files_properties(${PSPL_BINARY_DIR}/pspl_available_extensions.c 
                            PROPERTIES COMPILE_DEFINITIONS PSPL_TOOLCHAIN=1)
                            
# Generate typefiles include file
set(CONF_PLATFORM_TYPEFILE_INCLUDES "")
set(CONF_PLATFORM_SHADER_OBJECT_TYPES "")
list(LENGTH pspl_platform_list len)
if(len GREATER 0)
  math(EXPR len ${len}-1)
  foreach(platform RANGE ${len})
  
    # Extension member variables
    list(GET pspl_platform_list ${platform} PLAT_NAME)
    list(GET pspl_platform_typefile_list ${platform} PLAT_TYPEFILE)
                   
    # Ensure the typefile is included
    set(CONF_PLATFORM_TYPEFILE_INCLUDES "${CONF_PLATFORM_TYPEFILE_INCLUDES}#include \"${PLAT_TYPEFILE}\"\n")
    set(CONF_PLATFORM_SHADER_OBJECT_TYPES "${CONF_PLATFORM_SHADER_OBJECT_TYPES}\n    ${PLAT_NAME}_shader_object_t ${PLAT_NAME}_shader;")

  endforeach(platform)
endif()
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_toolchain_platform_typefiles.h.in
               pspl_toolchain_platform_typefiles.h)

# Define toolchain executable
add_executable(pspl
               Driver.c
               Preprocessor.c
               Compiler.c
               Packager.c
               Buffer.c
               ${PSPL_BINARY_DIR}/pspl_available_extensions.c)

# Link toolchain extensions
target_link_libraries(pspl ${pspl_extension_toolchain_link_list})
