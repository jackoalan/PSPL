include_directories(${PSPL_SOURCE_DIR})

# Ensure toolchain macro set for `pspl_available_extensions`
set_source_files_properties(${PSPL_BINARY_DIR}/pspl_available_extensions.c 
                            PROPERTIES COMPILE_DEFINITIONS PSPL_TOOLCHAIN=1)

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
