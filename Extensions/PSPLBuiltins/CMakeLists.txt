# Hashing library define
if(PSPL_TOOLCHAIN_HASHING STREQUAL BUILTIN)
  add_definitions(-D PSPL_HASHING_BUILTIN)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL COMMON_CRYPTO)
  add_definitions(-D PSPL_HASHING_COMMON_CRYPTO)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL OPENSSL)
  add_definitions(-D PSPL_HASHING_OPENSSL)
elseif(PSPL_TOOLCHAIN_HASHING STREQUAL WINDOWS)
  add_definitions(-D PSPL_HASHING_WINDOWS)
endif()

pspl_add_extension(BUILTINS "PSPL built-in directives and commands")
pspl_add_extension_toolchain(BUILTINS BuiltinsToolchain.c)
