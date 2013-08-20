//
//  Driver.c
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//
#define PSPL_INTERNAL

/* Should an error condition print backtrace? */
#ifndef _WIN32
#define PSPL_ERROR_PRINT_BACKTRACE 1
#endif

/* Should PSPL catch system signals as errors on its own? */
#define PSPL_ERROR_CATCH_SIGNALS 1

#include <stdio.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#if PSPL_ERROR_CATCH_SIGNALS
#include <signal.h>
#endif
#if PSPL_ERROR_PRINT_BACKTRACE
#include <execinfo.h>
#endif

#include <PSPL/PSPLCommon.h>
#include <PSPLExtension.h>
#include <PSPLInternal.h>
#include <PSPL/PSPLHash.h>

#include "Driver.h"
#include "Preprocessor.h"
#include "Compiler.h"
#include "Packager.h"


/* Maximum count of sources */
#define PSPL_MAX_SOURCES 128

/* Maximum count of defs */
#define PSPL_MAX_DEFS 128


/* Global driver state */
pspl_toolchain_driver_state_t driver_state;


/* Current working directory */
static char cwd[MAXPATHLEN];

/* Are we using xterm? */
uint8_t xterm_colour = 0;


#pragma mark Terminal Utils

/* Way to get terminal width (for line wrapping) */
static int term_cols() {
#   if _WIN32
    return 80;
#   else
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return w.ws_col;
#   endif
}

/* Word len (similar to `strlen` but ignores escape sequences) */
static size_t word_len(const char* word) {
    size_t result = 0;;
    uint8_t in_esc = 0;
    --word;
    while ((*(++word))) {
        if (*word == '\E' || *word == '\033')
            in_esc = 1;
        else if (in_esc && (*word == 'm' || *word == '\017'))
            in_esc = 0;
        else if (*word == '\0')
            break;
        else if (!in_esc)
            ++result;
    }
    return result;
}

/* Transform string into wrapped string */
#define truncate_string(str, max_len, to_buf) strncpy(to_buf, str, (max_len)-3); strncpy(to_buf+(max_len)-3, "...", 4);
static char* wrap_string(const char* str_in, int indent) {
    
    // Term col count
    int num_cols = term_cols();
    if (num_cols <= 0)
        num_cols = 80;
    if (num_cols > 6)
        num_cols -= 6;
    
    // Space for strings
    char cpy_str[1024];
    char* result_str = malloc(1024);
    result_str[0] = '\0';
    
    // Copy string
    strlcpy(cpy_str, str_in, 1024);
    
    // Initialise space-tokeniser
    char* save_ptr;
    char* word_str = strtok_r(cpy_str, " ", &save_ptr);
    
    // Enumerate words (and keep track of accumulated characters)
    int cur_line_col = 0;
    int cur_line = 0;
    
    // All-indent
    if (indent == 2) {
        cur_line_col = 6;
        strlcat(result_str, "      ", 1024);
    }
    
    for (;;) {
        if (!word_str) // Done
            break;
        
        // Available space remaining on line
        int avail_line_space = num_cols-cur_line_col;
        
        // Actual word length (0 length is actually a space)
        size_t word_length = word_len(word_str);
        if (!word_length)
            word_length = 1;
        
        // Spill onto next line if needed (with optional indent for readability)
        if (word_length >= avail_line_space) {
            if (indent) {
                cur_line_col = 6;
                strlcat(result_str, "\n      ", 1024);
            } else {
                cur_line_col = 0;
                strlcat(result_str, "\n", 1024);
            }
            avail_line_space = num_cols-cur_line_col;
            //if (!cur_line)
            //    num_cols += 2;
            ++cur_line;
        }
        
        strlcat(result_str, word_str, 1024);
        strlcat(result_str, " ", 1024);
        cur_line_col += word_length+1;
        
        word_str = strtok_r(save_ptr, " ", &save_ptr);
    }
    
    char* last_space = strrchr(result_str, ' ');
    if (last_space)
        *last_space = '\0';
    return result_str;
    
}


#pragma mark Copyright Reproduction

const char* PSPL_MIT_LICENCE =
"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
"of this software and associated documentation files (the \"Software\"), to deal\n"
"in the Software without restriction, including without limitation the rights\n"
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
"copies of the Software, and to permit persons to whom the Software is\n"
"furnished to do so, subject to the following conditions:\n"
"\n"
"The above copyright notice and this permission notice shall be included in\n"
"all copies or substantial portions of the Software.\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
"THE SOFTWARE.)";

const char* PSPL_FREEBSD_LICENCE =
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions\n"
"are met:\n"
"1. Redistributions of source code must retain the above copyright\n"
"notice, this list of conditions and the following disclaimer.\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"notice, this list of conditions and the following disclaimer in the\n"
"documentation and/or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n"
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"
"ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n"
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n"
"OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n"
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"
"LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
"OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
"SUCH DAMAGE.";

void pspl_toolchain_provide_copyright(const char* component_name,
                            const char* copyright,
                            const char* licence) {
    
    if (xterm_colour) {
        fprintf(stdout, BOLD BLUE"%s\n"SGR0, component_name);
        fprintf(stdout, BLUE"%s\n\n"SGR0, copyright);
    } else {
        fprintf(stdout, "%s\n", component_name);
        fprintf(stdout, "%s\n\n", copyright);
    }
    
    fprintf(stdout, "%s\n\n\n", licence);
    
}

static void print_copyrights() {

    // PSPL Core copyrights
    
    // First, PSPL's copyright
    pspl_toolchain_provide_copyright("PSPL (Toolchain and Runtime)", "Copyright (c) 2013 Jack Andersen <jackoalan@gmail.com>",
                           PSPL_MIT_LICENCE);
    
    fprintf(stdout, "\n\n");
    
    
#   ifdef _WIN32
    
    // PSPL Windows Core copyrights
    // MinGW
    pspl_toolchain_provide_copyright("MinGW (WIN32 API)", "Copyright (c) 2012 MinGW.org project",
                           PSPL_MIT_LICENCE);
    
    // FreeBSD
    pspl_toolchain_provide_copyright("FreeBSD (strtok_r, strlcat, strlcpy)",
                           "Copyright (c) 1992-2013 The FreeBSD Project. All rights reserved.\n"
                           "\n"
                           "strtok_r, from Berkeley strtok\n"
                           "Copyright (c) 1998 Softweyr LLC.  All rights reserved.\n"
                           "Oct 13, 1998 by Wes Peters <wes@softweyr.com>\n"
                           "\n"
                           "strlcat and strlcpy\n"
                           "Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>",
                           PSPL_FREEBSD_LICENCE);
    
    fprintf(stdout, "\n\n");
    
#   endif
    
    
    // Platforms
    pspl_platform_t** plat = &pspl_available_platforms[0];
    while (plat[0]) {
        if (plat[0]->toolchain_platform && plat[0]->toolchain_platform->copyright_hook) {
            if (xterm_colour)
                fprintf(stdout, BOLD RED"--- %s Platform ---\n\n\n"SGR0, plat[0]->platform_name);
            else
                fprintf(stdout, "--- %s Platform ---\n\n\n", plat[0]->platform_name);
            plat[0]->toolchain_platform->copyright_hook();
            fprintf(stdout, "\n\n");
        }
        ++plat;
    }
    
    fprintf(stdout, "\n\n");

    
    // Extensions
    pspl_extension_t** ext = &pspl_available_extensions[0];
    while (ext[0]) {
        if (ext[0]->toolchain_extension && ext[0]->toolchain_extension->copyright_hook) {
            if (xterm_colour)
                fprintf(stdout, BOLD RED"--- %s Extension ---\n\n\n"SGR0, ext[0]->extension_name);
            else
                fprintf(stdout, "--- %s Extension ---\n\n\n", ext[0]->extension_name);
            ext[0]->toolchain_extension->copyright_hook();
            fprintf(stdout, "\n\n");
        }
        ++ext;
    }
    
    fprintf(stdout, "\n\n");

}


#pragma mark Built-in Help

void pspl_toolchain_provide_subext(const char* subext_name, const char* subext_desc,
                                   unsigned indent_level) {
    if (!indent_level)
        indent_level = 1;
    int i;
    for (i=0 ; i<indent_level ; ++i)
        fprintf(stdout, "  ");
    if (xterm_colour)
        fprintf(stdout, "  "BOLD"- %s"NORMAL, subext_name);
    else
        fprintf(stdout, "  - %s", subext_name);
    if (subext_desc)
        fprintf(stdout, " - %s", subext_desc);
    fprintf(stdout, "\n");
}

static void print_help(const char* prog_name) {
    
    if (xterm_colour) {
        
        // We have xterm colour support!
        
        fprintf(stdout,
                BOLD RED "PSPL Toolchain\n"
                BOLD MAGENTA "By Jack Andersen "GREEN"<jackoalan@gmail.com>\n"
                UNDERLINE CYAN "https://github.com/jackoalan/PSPL\n" NORMAL
                "\n");
        fprintf(stdout, BLUE"Run with "BOLD"-l"NORMAL BLUE
                " to see licences on contained software components"SGR0);
        fprintf(stdout, "\n\n\n"
                BOLD"Available Extensions:\n"NORMAL);
        pspl_extension_t* ext = NULL;
        int i = 0;
        while ((ext = pspl_available_extensions[i++])) {
            fprintf(stdout, "  "BOLD"* %s"NORMAL" - %s\n",
                    ext->extension_name, ext->extension_desc);
            if (ext->toolchain_extension && ext->toolchain_extension->subext_hook)
                ext->toolchain_extension->subext_hook();
        }
        if (i==1)
            fprintf(stdout, "  "BOLD"-- No extensions available --"NORMAL"\n");
        fprintf(stdout,
                "\n"
                "\n"
                BOLD"Available Target Platforms:\n"NORMAL);
        pspl_platform_t* plat = NULL;
        i = 0;
        while ((plat = pspl_available_platforms[i++])) {
            fprintf(stdout, "  "BOLD"* %s"NORMAL" - %s",
                    plat->platform_name, plat->platform_desc);
            fprintf(stdout, "\n");
        }
        if (i==1)
            fprintf(stdout, "  "BOLD"-- No target platforms available --"NORMAL"\n");
        fprintf(stdout,
                "\n"
                "\n");
        
        // Now print usage info
        fprintf(stdout, BOLD BLUE"Command Synopsis:\n"NORMAL);
        const char* help =
        wrap_string("pspl ["BOLD"-o"NORMAL" "UNDERLINE"out-path"NORMAL"] ["BOLD"-E"NORMAL"|"BOLD"-c"NORMAL"] ["BOLD"-G"NORMAL" "UNDERLINE"reflist-out-path"NORMAL"] ["BOLD"-S"NORMAL" "UNDERLINE"staging-root-path"NORMAL"] ["BOLD"-D"NORMAL" "UNDERLINE"def-name"NORMAL"[="UNDERLINE"def-value"NORMAL"]]... ["BOLD"-T"NORMAL" "UNDERLINE"target-platform"NORMAL"]... ["BOLD"-e"NORMAL" <"UNDERLINE"LITTLE"NORMAL","UNDERLINE"BIG"NORMAL","UNDERLINE"BI"NORMAL">] "UNDERLINE"source1"NORMAL" ["UNDERLINE"source2"NORMAL" ["UNDERLINE"sourceN"NORMAL"]]...", 1);
        fprintf(stdout, "%s\n\n\n", help);
        free((char*)help);
        
    } else {
        
        // No colour support
        
        fprintf(stdout,
                "PSPL Toolchain\n"
                "By Jack Andersen <jackoalan@gmail.com>\n"
                "https://github.com/jackoalan/PSPL\n"
                "\n");
        fprintf(stdout, "Run with `-l`"
                " to see licences on contained software components");
        fprintf(stdout,
                "\n\n\nAvailable Extensions:\n");
        pspl_extension_t* ext = NULL;
        int i = 0;
        while ((ext = pspl_available_extensions[i++])) {
            fprintf(stdout, "  * %s - %s\n",
                    ext->extension_name, ext->extension_desc);
            if (ext->toolchain_extension && ext->toolchain_extension->subext_hook)
                ext->toolchain_extension->subext_hook();
        }
        if (i==1)
            fprintf(stdout, "  -- No extensions available --\n");
        fprintf(stdout,
                "\n"
                "\n"
                "Available Target Platforms:\n");
        pspl_platform_t* plat = NULL;
        i = 0;
        while ((plat = pspl_available_platforms[i++])) {
            fprintf(stdout, "  * %s - %s",
                    plat->platform_name, plat->platform_desc);
            fprintf(stdout, "\n");
        }
        if (i==1)
            fprintf(stdout, "  -- No target platforms available --\n");
        fprintf(stdout,
                "\n"
                "\n");
        
        // Now print usage info
        fprintf(stdout, "Command Synopsis:\n");
        const char* help =
        wrap_string("pspl [-o out-path] [-E|-c] [-G reflist-out-path] [-S staging-root-path] [-D def-name[=def-value]]... [-T target-platform]... [-e <LITTLE,BIG,BI>] source1 [source2 [sourceN]]...", 1);
        fprintf(stdout, "%s\n\n\n", help);
        free((char*)help);
                
    }
}


#pragma mark Error Handling

/* This will print a backtrace for `pspl_error` call */
#if PSPL_ERROR_PRINT_BACKTRACE
#define BACKTRACE_DEPTH 20
static void print_backtrace() {
    void *array[BACKTRACE_DEPTH];
    size_t size;
    char **strings;
    size_t i;
    
    size = backtrace(array, BACKTRACE_DEPTH);
    strings = backtrace_symbols(array, (int)size);
        
    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);
    
    free(strings);
}
#endif

/* Toolchain-wide error handler
 * uses a global context state to print context-sensitive error info */
void pspl_error(int exit_code, const char* brief, const char* msg, ...) {
    brief = wrap_string(brief, 1);
    char* msg_str = NULL;
    if (msg) {
        va_list va;
        va_start(va, msg);
        char msg_str_buf[1024];
        msg_str = msg_str_buf;
        vsprintf(msg_str, msg, va);
        va_end(va);
        char* new_msg = wrap_string(msg_str, 2);
        msg_str = new_msg;
    }
    
    char err_head_buf[256];
    err_head_buf[0] = '\0';
    char* err_head = err_head_buf;
    if (xterm_colour) {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string(BOLD RED"ERROR WHILE "UNDERLINE"INITIALISING"NORMAL BOLD RED" TOOLCHAIN:\n"SGR0, 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, BOLD RED"ERROR WHILE "UNDERLINE"PREPROCESSING"NORMAL BOLD BLUE" `%s`"MAGENTA" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num+1);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, BOLD RED"ERROR WHILE "CYAN"COMPILING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num+1);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, BOLD RED"ERROR WHILE "CYAN"PACKAGING "BLUE"`%s`\n"SGR0,
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "\n%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    } else {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string("ERROR WHILE INITIALISING TOOLCHAIN:\n", 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, "ERROR WHILE PREPROCESSING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num+1);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, "ERROR WHILE COMPILING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num+1);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, "ERROR WHILE PACKAGING `%s`\n",
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "\n%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, "%s:\n", brief);
    }
    free((void*)brief);
    
    fprintf(stderr, "%s\n", msg_str);
    
#   if PSPL_ERROR_PRINT_BACKTRACE
    if (xterm_colour)
        fprintf(stderr, BOLD BLUE"\nBacktrace:\n"SGR0);
    else
        fprintf(stderr, "\nBacktrace:\n");

    print_backtrace();
    fprintf(stderr, "\n");
#   endif // PSPL_ERROR_PRINT_BACKTRACE
    
    if (driver_state.tool_ctx->output_path)
        unlink(driver_state.tool_ctx->output_path);
    exit(exit_code);
}

/* Toolchain-wide warning handler
 * uses a global context state to print context-sensitive error info */
void pspl_warn(const char* brief, const char* msg, ...) {
    brief = wrap_string(brief, 1);
    char* msg_str = NULL;
    if (msg) {
        va_list va;
        va_start(va, msg);
        char msg_str_buf[1024];
        msg_str = msg_str_buf;
        vsprintf(msg_str, msg, va);
        va_end(va);
        char* new_msg = wrap_string(msg_str, 2);
        msg_str = new_msg;
    }
    
    char err_head_buf[256];
    err_head_buf[0] = '\0';
    char* err_head = err_head_buf;
    if (xterm_colour) {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string(BOLD MAGENTA"WARNING WHILE "CYAN"INITIALISING"MAGENTA" TOOLCHAIN:\n"SGR0, 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, BOLD MAGENTA"WARNING WHILE "CYAN"PREPROCESSING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, BOLD MAGENTA"WARNING WHILE "CYAN"COMPILING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, BOLD RED"WARNING WHILE "CYAN"PACKAGING "BLUE"`%s`\n"SGR0,
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "\n%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    } else {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string("WARNING WHILE INITIALISING TOOLCHAIN:\n", 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, "WARNING WHILE PREPROCESSING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, "WARNING WHILE COMPILING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, "WARNING WHILE PACKAGING `%s`\n",
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "\n%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, "%s:\n", brief);
    }
    free((void*)brief);
    
    fprintf(stderr, "%s\n", msg_str);
}

/* Report PSPLC read-in underflow */
void check_psplc_underflow(pspl_toolchain_driver_psplc_t* psplc, const void* cur_ptr) {
    //size_t delta = cur_ptr-(void*)psplc->psplc_data;
    /*
    if (delta > psplc->psplc_data_len)
        pspl_error(-1, "PSPLC underflow detected",
                   "PSPLC `%s` is not long enough to load needed data @0x%zx (%u)",
                   psplc->file_path, delta, (unsigned)delta);*/
}


#pragma mark Driver State Helpers

/* Insert def into driver opts */
static void add_def(pspl_toolchain_driver_opts_t* driver_opts,
                    const char* def) {
    if (driver_opts->def_c >= PSPL_MAX_DEFS) {
        pspl_error(-1, "Definitions exceeded maximum count",
                   "up to %u defs supported", (unsigned int)PSPL_MAX_DEFS);
    }
    
    driver_opts->def_k[driver_opts->def_c] = def;
    
    char* eq_ptr = strchr(def, '=');
    if (eq_ptr) {
        *eq_ptr = '\0';
        ++eq_ptr;
        driver_opts->def_v[driver_opts->def_c] = eq_ptr;
    }
    
    ++driver_opts->def_c;
}

/* Insert target platform into driver opts */
static pspl_platform_t* pspl_platforms[PSPL_MAX_PLATFORMS];
static void add_target_platform(pspl_toolchain_driver_opts_t* driver_opts,
                                const char* name) {
    if (driver_opts->platform_c >= PSPL_MAX_PLATFORMS) {
        pspl_error(-1, "Platforms exceeded maximum count",
                   "up to %u platforms supported", (unsigned int)PSPL_MAX_PLATFORMS);
    }
    
    // Look up platform
    pspl_platform_t* plat = NULL;
    int i = 0;
    while ((plat = pspl_available_platforms[i++])) {
        if (!strcmp(plat->platform_name, name)) {
            pspl_platforms[driver_opts->platform_c] = plat;
            break;
        }
    }
    
    if (!plat)
        pspl_error(-1, "Unable to find requested platform",
                   "requested `%s`", name);
    
    ++driver_opts->platform_c;
}

/* Accept default endianness argument */
static void set_default_endianness(pspl_toolchain_driver_opts_t* driver_opts,
                                   const char* arg) {
    
    if (!strcasecmp(arg, "little"))
        driver_opts->default_endianness = PSPL_LITTLE_ENDIAN;
    else if (!strcasecmp(arg, "big"))
        driver_opts->default_endianness = PSPL_BIG_ENDIAN;
    else if (!strcasecmp(arg, "bi"))
        driver_opts->default_endianness = PSPL_BI_ENDIAN;
    else
        pspl_error(-1, "Invalid default endianness",
                   "'%s' is not a valid endianness; please use one of [LITTLE, BIG, BI]", arg);
    
}

/* Lookup target extension by name */
static pspl_extension_t* lookup_ext(const char* ext_name, unsigned int* idx_out) {
    pspl_extension_t* ext = NULL;
    unsigned int i = 0;
    while ((ext = pspl_available_extensions[i++])) {
        if (!strcmp(ext->extension_name, ext_name)) {
            if (idx_out)
                *idx_out = i-1;
            break;
        }
    }
    return ext;
}

/* Lookup target platform by name */
static pspl_platform_t* lookup_target_platform(const char* plat_name) {
    pspl_platform_t* plat = NULL;
    unsigned int i = 0;
    while ((plat = pspl_available_platforms[i++])) {
        if (!strcmp(plat->platform_name, plat_name))
            break;
    }
    return plat;
}

/* Initialise psplc structure from file */
#define IS_PSPLC_BI pspl_head->endian_flags == PSPL_BI_ENDIAN
#if __LITTLE_ENDIAN__
#define IS_PSPLC_SWAPPED pspl_head->endian_flags == PSPL_BIG_ENDIAN
#elif __BIG_ENDIAN__
#define IS_PSPLC_SWAPPED pspl_head->endian_flags == PSPL_LITTLE_ENDIAN
#endif
static void init_psplc_from_file(pspl_toolchain_driver_psplc_t* psplc, const char* path) {
    int j;
    
    // Populate filename members
    psplc->file_path = path;
    
    // Make path absolute (if it's not already)
    const char* abs_path = psplc->file_path;
    if (abs_path[0] != '/') {
        abs_path = malloc(MAXPATHLEN);
        sprintf((char*)abs_path, "%s/%s", cwd, psplc->file_path);
    }
    
    // Enclosing dir
    char* last_slash = strrchr(abs_path, '/');
    psplc->file_enclosing_dir = malloc(MAXPATHLEN);
    sprintf((char*)psplc->file_enclosing_dir, "%.*s", (int)(last_slash-abs_path+1), abs_path);
    
    // File name
    psplc->file_name = last_slash+1;
    
    
    // Load PSPLC object data
    FILE* psplc_file = fopen(path, "r");
    fseek(psplc_file, 0, SEEK_END);
    size_t psplc_len = ftell(psplc_file);
    fseek(psplc_file, 0, SEEK_SET);
    if (psplc_len > PSPL_MAX_SOURCE_SIZE)
        pspl_error(-1, "PSPLC file exceeded filesize limit",
                   "PSPLC file `%s` is %zu bytes in length; exceeding %u byte limit",
                   path, psplc_len, (unsigned)PSPL_MAX_SOURCE_SIZE);
    uint8_t* psplc_buf = malloc(psplc_len+1);
    if (!psplc_buf)
        pspl_error(-1, "Unable to allocate memory buffer for PSPLC",
                   "errno %d - `%s`", errno, strerror(errno));
    size_t read_len = fread(psplc_buf, 1, psplc_len, psplc_file);
    psplc_buf[psplc_len] = '\0';
    psplc->psplc_data = psplc_buf;
    psplc->psplc_data_len = psplc_len;
    fclose(psplc_file);
    if (read_len != psplc_len)
        pspl_error(-1, "Didn't read expected amount from PSPLC",
                   "expected %lu bytes; read %zu bytes", psplc_len, read_len);
    
    // Verify file size (at least a header in length)
    if (psplc->psplc_data_len < sizeof(pspl_header_t))
        pspl_error(-1, "PSPLC file too short",
                   "`%s` is not a valid PSPLC file", psplc->file_path);
    
    // Set Header and verify magic
    const pspl_header_t* pspl_head = (pspl_header_t*)psplc->psplc_data;
    check_psplc_underflow(psplc, pspl_head+sizeof(pspl_header_t));
    if (memcmp(pspl_head->magic, "PSPL", 4))
        pspl_error(-1, "Invalid PSPLC magic",
                   "`%s` is not a valid PSPLC file", psplc->file_path);
    // Verify version
    if (pspl_head->version != 1)
        pspl_error(-1, "Invalid PSPLC version",
                   "file `%s` is version %u; expected version %u",
                   psplc->file_path, pspl_head->version, PSPL_VERSION);
    // Verify not package
    if (pspl_head->package_flag)
        pspl_error(-1, "Unable to import PSPLP packages",
                   "`%s` is a PSPLP package, not a PSPLC object file", psplc->file_path);
    // Verify endianness
    if (!(pspl_head->endian_flags & 0x3))
        pspl_error(-1, "Invalid endian flag",
                   "`%s` has invalid endian flag", psplc->file_path);
    
    
    // Parse offset header
    pspl_off_header_t* pspl_off_head = NULL;
    if (IS_PSPLC_BI) {
        pspl_off_header_bi_t* bi_head = (pspl_off_header_bi_t*)(psplc->psplc_data + sizeof(pspl_header_t));
        check_psplc_underflow(psplc, bi_head+sizeof(pspl_off_header_bi_t));
        pspl_off_head = &bi_head->native;
    } else {
        pspl_off_head = (pspl_off_header_t*)(psplc->psplc_data + sizeof(pspl_header_t));
        check_psplc_underflow(psplc, pspl_off_head+sizeof(pspl_off_header_t));
        if (IS_PSPLC_SWAPPED) {
            SWAP_PSPL_OFF_HEADER_T(pspl_off_head);
        }
    }
    
    // Populate extension set
    unsigned int psplc_extension_count = pspl_off_head->extension_name_table_c;
    unsigned int psplc_extension_off = pspl_off_head->extension_name_table_off;
    const char* psplc_extension_name = (char*)(psplc->psplc_data + psplc_extension_off);
    psplc->required_extension_set = calloc(psplc_extension_count+1, sizeof(pspl_extension_t*));
    for (j=0 ; j<psplc_extension_count ; ++j) {
        check_psplc_underflow(psplc, psplc_extension_name);
        
        // Lookup
        const pspl_extension_t* ext = lookup_ext(psplc_extension_name, NULL);
        if (!ext)
            pspl_error(-1, "PSPLC-required extension not available",
                       "PSPLC `%s` requested `%s` which is not available in this build of PSPL",
                       psplc->file_path, psplc_extension_name);
        
        // Add to set
        psplc->required_extension_set[j] = ext;
        
        // Advance
        psplc_extension_name += strlen(psplc_extension_name) + 1;
    }
    psplc->required_extension_set[psplc_extension_count] = NULL;
    
    
    // Populate platform set
    unsigned int psplc_platform_count = pspl_off_head->platform_name_table_c;
    unsigned int psplc_platform_off = pspl_off_head->platform_name_table_off;
    const char* psplc_platform_name = (char*)(psplc->psplc_data + psplc_platform_off);
    psplc->required_platform_set = calloc(psplc_platform_count+1, sizeof(pspl_platform_t*));
    for (j=0 ; j<psplc_platform_count ; ++j) {
        check_psplc_underflow(psplc, psplc_platform_name);
        
        // Lookup
        const pspl_platform_t* plat = lookup_target_platform(psplc_platform_name);
        if (!plat)
            pspl_error(-1, "PSPLC-required target platform not available",
                       "PSPLC `%s` requested `%s` which is not available in this build of PSPL",
                       psplc->file_path, psplc_platform_name);
        
        // Add to set
        psplc->required_platform_set[j] = plat;
        
        // Advance
        psplc_platform_name += strlen(psplc_platform_name) + 1;
    }
    psplc->required_platform_set[psplc_platform_count] = NULL;
}


#pragma mark Public Init Other Extension Request

/* Get/set/unset bit state of extension init index */
#define GET_INIT_BIT(idx) ((driver_state.ext_init_bits[(idx)/0x20] >> (idx)%0x20) & 0x1)
#define SET_INIT_BIT(idx) (driver_state.ext_init_bits[(idx)/0x20] |= (0x1 << (idx)%0x20))
#define UNSET_INIT_BIT(idx) (driver_state.ext_init_bits[(idx)/0x20] &= ~(0x1 << (idx)%0x20))

/* Request the immediate initialisation of another extension
 * (only valid within init hook) */
int pspl_toolchain_init_other_extension(const char* ext_name) {
    if (driver_state.pspl_phase != PSPL_PHASE_INIT_EXTENSION)
        return -1;
    
    // Lookup extension
    unsigned int idx;
    const pspl_extension_t* ext = lookup_ext(ext_name, &idx);
    if (!ext) {
        pspl_warn("Unable to locate extension",
                  "extension '%s' requested initialising '%s' early; latter not available in this build of PSPL",
                  driver_state.proc_extension->extension_name, ext_name);
        return -1;
    }
    
    // Run init hook of other extension if its not yet initialised
    if (ext->toolchain_extension && ext->toolchain_extension->init_hook && !GET_INIT_BIT(idx)) {
        const pspl_extension_t* save_ext = driver_state.proc_extension;
        driver_state.proc_extension = ext;
        int err;
        err = ext->toolchain_extension->init_hook(driver_state.tool_ctx);
        if (err)
            pspl_error(-1, "Extension failed to init",
                       "extension '%s' returned %d error code when requested to initialise early by '%s'",
                       ext->extension_name, err, save_ext->extension_name);
        driver_state.proc_extension = save_ext;
        SET_INIT_BIT(idx);
    }
    
    return 0;
}


#if PSPL_ERROR_CATCH_SIGNALS
static void catch_sig(int sig) {
#   ifdef _WIN32
    pspl_error(-1, "Caught signal", "PSPL caught signal %d", sig);
#   else
    if (sig == SIGCHLD || sig == SIGWINCH) // Child exits and window resizes are OK
        return;
    pspl_error(-1, "Caught signal", "PSPL caught signal %d (%s)", sig, sys_siglist[sig]);
#   endif
}
#endif


#pragma mark Main Driver Routine

int main(int argc, char** argv) {
    
#   if PSPL_ERROR_CATCH_SIGNALS
    // Register signal handler (throws pspl error with backtrace)
    int i;
    for (i=1 ; i<NSIG ; ++i)
        signal(i, catch_sig);
#   endif
    
    // Initial driver state
    driver_state.pspl_phase = PSPL_PHASE_INIT;
    
    // Get CWD
    if(!getcwd((char*)cwd, MAXPATHLEN))
        pspl_error(-1, "Unable to get current working directory",
                   "errno %d - `%s`", errno, strerror(errno));
    
    
    // Check for xterm
    const char* term_type = getenv("TERM");
    if (term_type && strlen(term_type) >= 5 && !strncmp("xterm", term_type, 5))
        xterm_colour = 1;
    
    
    // Start recording driver options
    pspl_toolchain_driver_opts_t driver_opts;
    memset(&driver_opts, 0, sizeof(driver_opts));
    driver_opts.source_a = calloc(PSPL_MAX_SOURCES, sizeof(const char*));
    driver_opts.def_k = calloc(PSPL_MAX_DEFS, sizeof(const char*));
    driver_opts.def_v = calloc(PSPL_MAX_DEFS, sizeof(const char*));
    
    // We need at least one argument
    if (!argc || argc == 1) {
        print_help(argv[0]);
        return 0;
    }
    
    // Default endianness
    driver_opts.default_endianness = PSPL_UNSPEC_ENDIAN;
    
    // Initial argument pass
    char expected_arg = 0;
    for (i=1 ; i<argc ; ++i) {
        if (!expected_arg && argv[i][0] == '-') { // Process flag argument
            char token_char = argv[i][1];
            
            if (token_char == 'h') {
                
                print_help(argv[0]);
                return 0;
                
            } else if (token_char == 'L') {
                
                print_copyrights();
                return 0;
                
            } else if (token_char == 'l') {
                
                char execstr[MAXPATHLEN];
                snprintf(execstr, MAXPATHLEN, "%s -L | less -r", argv[0]);
                system(execstr);
                return 0;
                
            } else if (token_char == 'o') {
                
                if (argv[i][2])
                    driver_opts.out_path = &argv[i][2];
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'E') {
                
                expected_arg = 0;
                driver_opts.pspl_mode_opts |= PSPL_MODE_PREPROCESS_ONLY;
                
            } else if (token_char == 'c') {
                
                expected_arg = 0;
                driver_opts.pspl_mode_opts |= PSPL_MODE_COMPILE_ONLY;
                
            } else if (token_char == 'G') {
                
                if (argv[i][2])
                    driver_opts.reflist_out_path = &argv[i][2];
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'S') {
                
                if (argv[i][2])
                    driver_opts.staging_path = &argv[i][2];
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'D') {
                
                if (argv[i][2])
                    add_def(&driver_opts, &argv[i][2]);
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'T') {
                
                if (argv[i][2])
                    add_target_platform(&driver_opts, &argv[i][2]);
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'e') {
                
                if (argv[i][2])
                    set_default_endianness(&driver_opts, &argv[i][2]);
                else
                    expected_arg = token_char;
                
            } else
                pspl_error(-1, "Unrecognised argument flag",
                           "`-%c` flag not recognised by PSPL", token_char);
            
        } else { // Process string argument
            char* str_arg = argv[i];
            
            if (expected_arg) { // Process a mode-string
                
                if (expected_arg == 'o') {
                    
                    // Output path
                    driver_opts.out_path = str_arg;
                    
                } else if (expected_arg == 'G') {
                    
                    // Reference gathering
                    driver_opts.reflist_out_path = str_arg;
                    
                } else if (expected_arg == 'S') {
                    
                    // Staging area
                    driver_opts.staging_path = str_arg;
                    
                } else if (expected_arg == 'D') {
                    
                    // Add definition
                    add_def(&driver_opts, str_arg);
                    
                } else if (expected_arg == 'T') {
                    
                    // Add target platform
                    add_target_platform(&driver_opts, str_arg);
                    
                } else if (expected_arg == 'e') {
                    
                    set_default_endianness(&driver_opts, str_arg);
                    
                }
                
                expected_arg = 0;
                    
                
            } else { // Add a source input
                
                // Add source
                if (driver_opts.def_c >= PSPL_MAX_SOURCES) {
                    pspl_error(-1, "Sources exceeded maximum count",
                               "up to %u sources supported", (unsigned int)PSPL_MAX_SOURCES);
                }
                
                driver_opts.source_a[driver_opts.source_c] = str_arg;
                
                ++driver_opts.source_c;
                
            }
            
        }
    }
    
    // We need at least one source
    if (!driver_opts.source_c) {
        if (xterm_colour)
            fprintf(stderr, BOLD"*** Please provide "UNDERLINE"at least one"NORMAL BOLD" input source ***\n\n"SGR0);
        else
            fprintf(stderr, "*** Please provide at least one input source ***\n\n");
        print_help(argv[0]);
        return -1;
    }
    
    // We can't compile *and* preprocess
    if ((driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) &&
        (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY)) {
        pspl_error(-1, "Impossible task",
                   "PSPL can't preprocess-only (-E) *and* compile-only (-c) simultaneously");
        return -1;
    }
    
    
    // Ensure *all* provided files and paths exist
    
    // Sources
    for (i=0;i<driver_opts.source_c;++i) {
        
        // Verify file extension
        const char* file_ext = strrchr(driver_opts.source_a[i], '.') + 1;
        if (!file_ext || (strcasecmp(file_ext, "pspl") && strcasecmp(file_ext, "psplc")))
            pspl_error(-1, "Source file extension matters!!",
                       "please provide PSPL sources with '.pspl' extension and compiled objects with '.psplc' extension; `%s` does not follow this convention", driver_opts.source_a[i]);
        if (!strcasecmp(file_ext, "psplc") && ((driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) ||
                                               (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY)))
            pspl_error(-1, "Unable to accept PSPLC",
                       "unable to accept `%s` when in compile-only or preprocess-only mode",
                       driver_opts.source_a[i]);
        
        FILE* file;
        if ((file = fopen(driver_opts.source_a[i], "r")))
            fclose(file);
        else
            pspl_error(-1, "Unable to open provided source", "Can't open `%s` for reading",
                       driver_opts.source_a[i]);
    }
    
    // Reflist (just the path)
    if (driver_opts.reflist_out_path) {
        FILE* file;
        if ((file = fopen(driver_opts.reflist_out_path, "r")))
            fclose(file);
        else if ((file = fopen(driver_opts.reflist_out_path, "w")))
            fclose(file);
        else
            pspl_error(-1, "Unable to write reflist", "Can't open `%s` for writing",
                       driver_opts.reflist_out_path);
    }
    
    // Output file
    //driver_state.out_path = driver_opts.out_path;
    if (driver_opts.out_path) {
        if (driver_opts.out_path[0] != '-') {
            FILE* file;
            if ((file = fopen(driver_opts.out_path, "w")))
                fclose(file);
            else
                pspl_error(-1, "Unable to write output", "Can't open `%s` for writing",
                           driver_opts.out_path);
        }
    }
    
    // Staging area path (make cwd if not specified)
    if (!driver_opts.staging_path)
        driver_opts.staging_path = cwd;
    
    // Stat path to ensure it's a directory
    struct stat s;
    if (stat(driver_opts.staging_path, &s))
        pspl_error(-1, "Unable to `stat` requested staging path",
                   "`%s`; errno %d - `%s`", driver_opts.staging_path,
                   errno, strerror(errno));
    else if (!(s.st_mode & S_IFDIR)) {
        pspl_error(-1, "Unable to use requested staging path",
                   "`%s`; path not a directory", driver_opts.staging_path);
    }
    snprintf(driver_state.staging_path, MAXPATHLEN, "%s/PSPLFiles/", driver_opts.staging_path);
    
    // Set target platform array ref
    driver_opts.platform_a = (const pspl_platform_t* const *)pspl_platforms;
    
    // Populate toolchain context
    pspl_toolchain_context_t tool_ctx = {
        .target_runtime_platforms_c = driver_opts.platform_c,
        .target_runtime_platforms = driver_opts.platform_a,
        .target_endianness = driver_opts.default_endianness,
        .def_c = driver_opts.def_c,
        .def_k = driver_opts.def_k,
        .def_v = driver_opts.def_v,
        .output_path = driver_opts.out_path
    };
    driver_state.tool_ctx = &tool_ctx;
    
    // Determine endianness
    for (i=0 ; i<driver_opts.platform_c ; ++i)
        tool_ctx.target_endianness |= driver_opts.platform_a[i]->byte_order;
    tool_ctx.target_endianness &= PSPL_BI_ENDIAN;
    if (!tool_ctx.target_endianness) {
#       if __LITTLE_ENDIAN__
        tool_ctx.target_endianness = PSPL_LITTLE_ENDIAN;
#       elif __BIG_ENDIAN__
        tool_ctx.target_endianness = PSPL_BIG_ENDIAN;
#       endif
    }
    
    
    // Reference gathering context (if requested)
    driver_state.gather_ctx = NULL;
    if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) &&
        driver_opts.reflist_out_path) {
        pspl_gatherer_context_t gatherer;
        pspl_gather_load_cmake(&gatherer, driver_opts.reflist_out_path);
        driver_state.gather_ctx = &gatherer;
    }
    
    
#   pragma mark Available Extension Count
    
    // Extension count
    driver_state.ext_count = 0;
    pspl_extension_t** ext_counter = pspl_available_extensions;
    while (*(ext_counter++))
        ++driver_state.ext_count;
    
    // Allocate extension init bits
    size_t ext_init_bits_words = driver_state.ext_count / 0x20 + (driver_state.ext_count&0x1f)?1:0;
    driver_state.ext_init_bits = calloc(ext_init_bits_words, sizeof(uint32_t));
    
    
#   pragma mark Begin Toolchain Process
    
    // Make packager context (if packaging)
    pspl_packager_context_t packager_ctx;
    if (!(driver_opts.pspl_mode_opts & (PSPL_MODE_PREPROCESS_ONLY|PSPL_MODE_COMPILE_ONLY)))
        pspl_packager_init(&packager_ctx, driver_state.ext_count, driver_opts.platform_c, driver_opts.source_c);
    
    // Initialise object indexer for each input file
    pspl_indexer_context_t* indexers = calloc(driver_opts.source_c, sizeof(pspl_indexer_context_t));
    
    // Arguments valid, now time to compile each source and/or load PSPLCs for packaging!!
    unsigned int sources_c = 0;
    pspl_toolchain_driver_source_t sources[PSPL_MAX_SOURCES];
    unsigned int psplcs_c = 0;
    pspl_toolchain_driver_psplc_t psplcs[PSPL_MAX_SOURCES];
    
    
    // Run through sources and objects
    for (i=0 ; i<driver_opts.source_c ; ++i) {
        pspl_indexer_context_t* indexer = &indexers[i];
        driver_state.indexer_ctx = NULL;
        if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)) {
            pspl_indexer_init(indexer, driver_state.ext_count, driver_opts.platform_c);
            driver_state.indexer_ctx = indexer;
        }
        int j;
        
        const char* file_ext = strrchr(driver_opts.source_a[i], '.') + 1;
        if (!strcasecmp(file_ext, "pspl")) {
            driver_state.pspl_phase = PSPL_PHASE_PREPARE;
            
#           pragma mark Process PSPL Source
            pspl_toolchain_driver_source_t* source = &sources[sources_c++];
            driver_state.source = source;
            source->parent_source = NULL;
            
            // Populate filename members
            source->file_path = driver_opts.source_a[i];
            
            // Generate default PSPLC hash
            if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)) {
                pspl_hash_ctx_t hash_ctx;
                pspl_hash_init(&hash_ctx);
                pspl_hash_write(&hash_ctx, source->file_path, strlen(source->file_path));
                pspl_hash* result_hash;
                pspl_hash_result(&hash_ctx, result_hash);
                pspl_hash_cpy(&driver_state.indexer_ctx->psplc_hash, result_hash);
            }
            
            // Make path absolute (if it's not already)
            const char* abs_path = source->file_path;
            if (abs_path[0] != '/') {
                abs_path = malloc(MAXPATHLEN);
                sprintf((char*)abs_path, "%s/%s", cwd, source->file_path);
            }
            
            // Enclosing dir
            char* last_slash = strrchr(abs_path, '/');
            source->file_enclosing_dir = malloc(MAXPATHLEN);
            sprintf((char*)source->file_enclosing_dir, "%.*s", (int)(last_slash-abs_path+1), abs_path);
            
            // File name
            source->file_name = last_slash+1;
            
            // Allocate required extension set
            source->required_extension_set = calloc(driver_state.ext_count+1, sizeof(pspl_extension_t*));
            
            // Set error handling state for source file
            driver_state.file_name = source->file_name;
            driver_state.line_num = 0;
            
            // Toolchain context info for this source
            tool_ctx.pspl_name = source->file_name;
            tool_ctx.pspl_enclosing_dir = source->file_enclosing_dir;
            
            // Initialise each extension
            driver_state.pspl_phase = PSPL_PHASE_INIT_EXTENSION;
            pspl_extension_t* ext;
            j = 0;
            while ((ext = pspl_available_extensions[j++])) {
                driver_state.proc_extension = ext;
                if (ext->toolchain_extension && ext->toolchain_extension->init_hook && !GET_INIT_BIT(j-1)) {
                    int err;
                    if ((err = ext->toolchain_extension->init_hook(&tool_ctx)))
                        pspl_error(-1, "Extension failed to init",
                                   "extension '%s' returned %d error code", ext->extension_name, err);
                    SET_INIT_BIT(j-1);
                }
            }
            
            // Prepare to read in file
            driver_state.pspl_phase = PSPL_PHASE_PREPARE;
            
            // Load original source
            FILE* source_file = fopen(driver_opts.source_a[i], "r");
            if (!source_file)
                pspl_error(-1, "Unable to open PSPL source",
                           "`%s` is unavailable for reading; errno %d (%s)",
                           driver_opts.source_a[i], errno, strerror(errno));
            fseek(source_file, 0, SEEK_END);
            size_t source_len = ftell(source_file);
            fseek(source_file, 0, SEEK_SET);
            if (source_len > PSPL_MAX_SOURCE_SIZE)
                pspl_error(-1, "PSPL Source file exceeded filesize limit",
                           "source file `%s` is %zu bytes in length; exceeding %u byte limit",
                           driver_opts.source_a[i], source_len, (unsigned)PSPL_MAX_SOURCE_SIZE);
            char* source_buf = malloc(source_len+1);
            if (!source_buf)
                pspl_error(-1, "Unable to allocate memory buffer for PSPL source",
                           "errno %d - `%s`", errno, strerror(errno));
            size_t read_len = fread(source_buf, 1, source_len, source_file);
            source_buf[source_len] = '\0';
            source->original_source = source_buf;
            fclose(source_file);
            if (read_len != source_len)
                pspl_error(-1, "Didn't read expected amount from PSPL source",
                           "expected %lu bytes; read %zu bytes", source_len, read_len);
            
            
            
            // Now run preprocessor
            pspl_run_preprocessor(source, &tool_ctx, &driver_opts, 1);
            
            
            // Now run compiler (if not in preprocess-only mode)
            if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)) {
                driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                driver_state.file_name = source->file_name;
                driver_state.line_num = 0;
                pspl_run_compiler(source, &tool_ctx, &driver_opts);
            }
            
            // Finish each extension
            driver_state.pspl_phase = PSPL_PHASE_FINISH_EXTENSION;
            j = 0;
            while ((ext = pspl_available_extensions[j++])) {
                driver_state.proc_extension = ext;
                if (ext->toolchain_extension && ext->toolchain_extension->finish_hook)
                    ext->toolchain_extension->finish_hook(&tool_ctx);
                UNSET_INIT_BIT(j-1);
            }
            
            // Only do first source if in preprocess or compile only modes
            if (driver_opts.pspl_mode_opts & (PSPL_MODE_PREPROCESS_ONLY|PSPL_MODE_COMPILE_ONLY))
                break;
            
            
        } else if (!strcasecmp(file_ext, "psplc")) {
            driver_state.pspl_phase = PSPL_PHASE_PREPARE;
            
#           pragma mark Process PSPLC Object
            pspl_toolchain_driver_psplc_t* psplc = &psplcs[psplcs_c++];
            init_psplc_from_file(psplc, driver_opts.source_a[i]);
            if (driver_state.indexer_ctx)
                pspl_indexer_psplc_stub_augment(driver_state.indexer_ctx, psplc);
            
        }
        
        // Add Indexer to packager (if packaging)
        if (!(driver_opts.pspl_mode_opts & (PSPL_MODE_PREPROCESS_ONLY|PSPL_MODE_COMPILE_ONLY)))
            pspl_packager_indexer_augment(&packager_ctx, indexer);
        
    }
    
    
#   pragma mark Output Stage
    
    // Open output file
    FILE* out_file;
    if (driver_opts.out_path) {
        if (driver_opts.out_path[0] == '-')
            out_file = stdout;
        out_file = fopen(driver_opts.out_path, "w");
    } else {
        if (driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)
            out_file = fopen("a.out.pspl", "w");
        else if (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY)
            out_file = fopen("a.out.psplc", "w");
        else
            out_file = fopen("a.out.psplp", "w");
    }
    
    // Output selected data
    if (driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) {
        
        // Preprocessor only
        size_t len = strlen(sources[0].preprocessed_source);
        fwrite(sources[0].preprocessed_source, 1, len, out_file);
        
    } else if (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY) {
        
        // Compile only
        pspl_indexer_write_psplc(driver_state.indexer_ctx, driver_opts.default_endianness, out_file);
        
    } else {
        
        // Full Package
        driver_state.pspl_phase = PSPL_PHASE_PACKAGE;
        pspl_packager_write_psplp(&packager_ctx, driver_opts.default_endianness, out_file);
        
    }
    
    if (!driver_opts.out_path || (driver_opts.out_path && driver_opts.out_path[0] != '-'))
        fclose(out_file);
    
    
    // Same for reflist
    if (driver_state.gather_ctx)
        pspl_gather_finish(driver_state.gather_ctx, driver_opts.out_path);
    
    
    return 0;
    
}




