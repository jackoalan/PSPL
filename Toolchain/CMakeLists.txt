include_directories(${PSPL_SOURCE_DIR})

add_executable(pspl
               Driver.c
               Preprocessor.c
               Compiler.c
               Packager.c
               Buffer.c)
