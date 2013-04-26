//
//  Driver.c
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//
#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <PSPL/PSPL.h>
#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>


// Escape character sequences to control xterm
#define NOBKD "\E[47;49m"
#define BLUE "\E[47;34m"NOBKD
#define MAGENTA "\E[47;35m"NOBKD
#define RED "\E[47;31m"NOBKD
#define GREEN "\E[47;32m"NOBKD
#define CYAN "\E[47;36m"NOBKD
#define NOBOLD "\033[0m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define SGR0 "\E[m\017"


void print_help(const char* prog_name) {
    const char* term_type = getenv("TERM");
    
    if (term_type && strlen(term_type) >= 5 && !strncmp("xterm", term_type, 5)) {
        
        // We have xterm colour support!
        
        fprintf(stderr, BOLD RED "PSPL Toolchain\n"
                        BOLD MAGENTA "Copyright 2013 Jack Andersen "GREEN"<jackoalan@gmail.com>\n"
                        UNDERLINE CYAN "https://github.com/jackoalan/PSPL\n\n" NOBOLD
        
                        BOLD BLUE"Simple Usage:"NOBOLD"\n"BLUE"  %s [-d] [-o <PSPLP_OUT_PATH>] "
                        "<PSPLD_OR_PSPLCD>"SGR0"\n"
                        "  This usage will transform a valid intermediate directory structure ("UNDERLINE"PSPLD"NOBOLD")\n"
                        "  or compiled directory structure ("UNDERLINE"PSPLCD"NOBOLD") into a "UNDERLINE"PSPLP"NOBOLD" flat-file package.\n"
                        "  Adding the "BOLD"-d"NOBOLD" flag will emit a "UNDERLINE"PSPLPD"NOBOLD" directory rather than a "UNDERLINE"PSPLP"NOBOLD" flat-file.\n"
                        "  Omiting the "BOLD"-o"NOBOLD" flag will save the output file in the same directory with\n"
                        "  appropriate file-extension added.\n\n"
        
                        BOLD BLUE "Advanced Usage:"NOBOLD BLUE"\n  %s <VERB> [options]\n"NOBOLD
                        "  Since this "BOLD"pspl"NOBOLD" command contains an entire "UNDERLINE"toolchain"
                        NOBOLD", there are multiple ways\n  to use it. The main functionality is selected using one of the verbs below:\n\n"
                
                        BOLD MAGENTA"Verbs:\n"SGR0
                        "  Supply verbs here\n\n"
                        SGR0, prog_name, prog_name);
        
    } else {
        fprintf(stderr, "No colour version\n");
    }
}

int main(int argc, char** argv) {
    
    // We need at least one argument
    if (!argc || argc == 1) {
        print_help(argv[0]);
        return 0;
    }
    
    // Initial argument pass
    unsigned int def_c = 0;
    
    // Parse arguments
    int i;
    for (i=0;i<argc;++i) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'h') {
                print_help(argv[0]);
                return 0;
            }
            
        }
    }

    
    return 0;
}