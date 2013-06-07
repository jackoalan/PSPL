//
//  CompilerBuiltins.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//

#include <PSPLExtension.h>
#include <string.h>
#include <stdio.h>

int BUILTINS_init(const pspl_toolchain_context_t* driver_context) {
    return 0;
}

static int converter_hook(char* path_out, const char* path_in, const char* suggested_path) {
    strcpy(path_out, suggested_path);
    FILE* file = fopen(suggested_path, "w");
    fwrite("Some stuff", 1, sizeof("Some stuff"), file);
    fclose(file);
    return 0;
}

int BUILTINS_command_call_hook(const pspl_toolchain_context_t* driver_context,
                               const pspl_toolchain_heading_context_t* current_heading,
                               const char* command_name,
                               unsigned int command_argc,
                               const char** command_argv) {
    
    
    
}
