//
//  CompilerBuiltins.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//

#include <PSPL/PSPLExtension.h>
#include "../../PSPLHash.h"
#include <string.h>
#include <stdio.h>

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
            int i;
            for (i = 0; i < PSPL_HASH_LENGTH; i++) {
                if (i > 0) fprintf(stderr, ":");
                fprintf(stderr, "%02X", (unsigned int)(result[i]));
            }
            fprintf(stderr, "\n");
            
            // Test ref gathering
            //pspl_gather_referenced_file("test.txt");
            //pspl_gather_referenced_file("test12.txt");
        }
        
    }
    
    return 0;
}
