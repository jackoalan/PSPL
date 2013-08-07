include_directories(${PSPL_SOURCE_DIR})

# Hashing library define
unset(HASH_LIB)
if(PSPL_TOOLCHAIN_HASHING STREQUAL BUILTIN)
  add_definitions(-D PSPL_HASHING_BUILTIN)
  set(HASH_LIB hash_builtin)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL COMMON_CRYPTO)
  add_definitions(-D PSPL_HASHING_COMMON_CRYPTO)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL OPENSSL)
  add_definitions(-D PSPL_HASHING_OPENSSL)
  find_library(HASH_LIB crypto)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL WINDOWS)
  add_definitions(-D PSPL_HASHING_WINDOWS)
endif()

# Define toolchain executable
pspl_add_executable(pspl
               Driver.c
               Preprocessor.c
               Compiler.c
               Packager.c
               Buffer.c
               ReferenceGatherer.c
               ObjectIndexer.c
               ${PSPL_BINARY_DIR}/pspl_available_toolchain_extensions.c)
               
set_target_properties(pspl PROPERTIES
                      COMPILE_DEFINITIONS PSPL_TOOLCHAIN=1)

# Link toolchain extensions
pspl_target_link_libraries(pspl ${pspl_extension_toolchain_link_list} ${HASH_LIB} pspl_common)
