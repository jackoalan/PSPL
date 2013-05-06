//
//  PreprocessorBuiltins.c
//  PSPL
//
//  Created by Jack Andersen on 5/3/13.
//
//

#include <string.h>
#include <stdio.h>
#include <PSPL/PSPLExtension.h>


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

/* Process a hook call from the preprocessor */
void BUILTINS_pp_hook(const pspl_toolchain_context_t* driver_context,
                      unsigned int directive_argc,
                      const char** directive_argv) {
    
    if (!strcasecmp(directive_argv[0], "MESSAGE"))
        message(directive_argc, directive_argv);
    
}
