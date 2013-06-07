//
//  BuiltinsToolext.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//


#include <PSPLExtension.h>
#include "PreprocessorBuiltins.h"
#include "CompilerBuiltins.h"

/* Heading names */
static const char* claimed_headings[] = {
    "VERTEX",
    "DEPTH",
    "FRAGMENT",
    "BLEND",
    NULL};

/* Preprocessor directives */
static const char* claimed_pp_direc[] = {
    "MESSAGE",
    "MSG",
    "ERROR",
    "ERR",
    "WARNING",
    "WARN",
    "DEFINE",
    "DEF",
    "UNDEF",
    "IF",
    "ELSEIF",
    "ELIF",
    "ELSE",
    "ENDIF",
    "FI",
    NULL};

/* Global command names */
static const char* claimed_commands[] = {
    "PSPL_LOAD_MESSAGE",
    "RUN_TEST",
    NULL};


/* Main extension bindings */
pspl_toolchain_extension_t BUILTINS_toolext = {
    .claimed_global_preprocessor_directives = claimed_pp_direc,
    .claimed_global_command_names = claimed_commands,
    .claimed_heading_names = claimed_headings,
    .init_hook = BUILTINS_init,
    .line_preprocessor_hook = BUILTINS_pp_hook,
    .command_call_hook = BUILTINS_command_call_hook};

