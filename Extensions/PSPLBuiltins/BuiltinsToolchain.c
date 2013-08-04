//
//  BuiltinsToolchain.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//


#include <stdio.h>
#include <PSPLExtension.h>

#pragma mark Preprocessor Builtins

static void message(unsigned int directive_argc,
                    const char** directive_argv) {
    int i;
    uint8_t print_space = 0;
    for (i=0 ; i<directive_argc ; ++i) {
        if (print_space)
            fprintf(stderr, " ");
        fprintf(stderr, "%s", directive_argv[i]);
        print_space = 1;
    }
    fprintf(stderr, "\n");
}

#define ERR_BUF_LEN 1024

static void error(unsigned int directive_argc,
                  const char** directive_argv) {
    char err_buf[ERR_BUF_LEN];
    err_buf[0] = '\0';
    int i;
    uint8_t print_space = 0;
    for (i=1 ; i<directive_argc ; ++i) {
        if (print_space)
            strlcat(err_buf, " ", ERR_BUF_LEN);
        strlcat(err_buf, directive_argv[i], ERR_BUF_LEN);
        print_space = 1;
    }
    pspl_error(-1, "User-defined [ERROR] in source", "%s", err_buf);
}

static void warn(unsigned int directive_argc,
                 const char** directive_argv) {
    char err_buf[1024];
    err_buf[0] = '\0';
    int i;
    uint8_t print_space = 0;
    for (i=1 ; i<directive_argc ; ++i) {
        if (print_space)
            strlcat(err_buf, " ", ERR_BUF_LEN);
        strlcat(err_buf, directive_argv[i], ERR_BUF_LEN);
        print_space = 1;
    }
    pspl_warn("User-defined [WARNING] in source", "%s", err_buf);
}

/* Process a hook call from the preprocessor */
static void pp_hook(const pspl_toolchain_context_t* driver_context,
                    const char* directive_name,
                    unsigned int directive_argc,
                    const char** directive_argv) {
    
    if (!strcasecmp(directive_name, "MESSAGE"))
        message(directive_argc, directive_argv);
    else if (!strcasecmp(directive_name, "ERROR"))
        error(directive_argc, directive_argv);
    else if (!strcasecmp(directive_name, "WARN"))
        warn(directive_argc, directive_argv);
    else if (!strcasecmp(directive_name, "WARNING"))
        warn(directive_argc, directive_argv);
}


#pragma mark Compiler Builtins


static void command_call_hook(const pspl_toolchain_context_t* driver_context,
                              const pspl_toolchain_heading_context_t* current_heading,
                              const char* command_name,
                              unsigned int command_argc,
                              const char** command_argv) {
    
    
    
}


#pragma mark Extension Definition


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
    .init_hook = NULL,
    .line_preprocessor_hook = pp_hook,
    .command_call_hook = command_call_hook};

