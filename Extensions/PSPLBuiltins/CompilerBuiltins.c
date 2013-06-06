//
//  CompilerBuiltins.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//

#include <PSPLExtension.h>
#include "../../PSPLHash.h"
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
    if (!strcasecmp(command_name, "TEST_HASH")) {
        if (!command_argc)
            fprintf(stderr, "`TEST_HASH` needs one argument\n");
        else {
            pspl_hash_ctx_t hash_ctx;
            pspl_hash_init(&hash_ctx);
            pspl_hash_write(&hash_ctx, command_argv[0], strlen(command_argv[0]));
            pspl_hash* result;
            pspl_hash_result(&hash_ctx, result);
            fprintf(stderr, "Hash of '%s':\n", command_argv[0]);
            char string[PSPL_HASH_STRING_LEN];
            pspl_hash_fmt(string, result);
            fprintf(stderr, "%s\n", string);
        }
    } else if (!strcasecmp(command_name, "TEST_FILE")) {
        if (!command_argc)
            fprintf(stderr, "`TEST_FILE` needs one argument\n");
        else {
            fprintf(stderr, "Received: %s\n", command_argv[0]);
            pspl_hash* result_hash;
            pspl_package_file_augment(NULL, command_argv[0], NULL, NULL, 1, NULL, &result_hash);
        }
    } else if (!strcasecmp(command_name, "RUN_TEST")) {
        if (!command_argc)
            fprintf(stderr, "`RUN_TEST` needs one argument\n");
        pspl_embed_hash_keyed_object(NULL, "test", command_argv[0], command_argv[0], strlen(command_argv[0]));
    }
    
    return 0;
}
