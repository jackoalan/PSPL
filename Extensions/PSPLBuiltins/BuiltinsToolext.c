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

/* Global command names */
static const char* claimed_commands[] = {
    "PSPL_PUSH_HEADING",
    "PSPL_POP_HEADING",
    "PSPL",
    NULL};

/* Heading names */
static const char* claimed_headings[] = {
    "VERTEX",
    "DEPTH",
    "FRAGMENT",
    "BLEND",
    NULL};

/* Preprocessor directives */
static const char* claimed_pp_direc[] = {"MESSAGE", NULL};

/* Main extension bindings */
pspl_toolchain_extension_t BUILTINS_toolext = {
    .claimed_global_preprocessor_directives = claimed_pp_direc,
    .claimed_global_command_names = claimed_commands,
    .claimed_heading_names = claimed_headings,
    .line_preprocessor_hook = BUILTINS_pp_hook,
    .command_call_hook = BUILTINS_command_call_hook
};
