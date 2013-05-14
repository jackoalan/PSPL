//
//  BuiltinsToolext.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//


#include <PSPL/PSPLExtension.h>
#include "PreprocessorBuiltins.h"
#include "CompilerBuiltins.h"

/* Command names */
static const char* claimed_commands[] = {"TEST_HASH", "TEST_FILE", NULL};

/* Preprocessor directives */
static const char* claimed_pp_direc[] = {"MESSAGE", NULL};

/* Main extension bindings */
pspl_toolchain_extension_t BUILTINS_toolext = {
    .claimed_global_preprocessor_directives = claimed_pp_direc,
    .claimed_global_command_names = claimed_commands,
    .line_preprocessor_hook = BUILTINS_pp_hook,
    .command_call_hook = BUILTINS_command_call_hook
};
